#include "Character/Monster/JunMonster.h"

#include "AI/JunAIController.h"
#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunBGMManager.h"
#include "System/JunTimeEffectSubsystem.h"
#include "UI/JunMonsterHUDWidget.h"
#include "Weapon/ArrowProjectile.h"
#include "Weapon/WeaponActor.h"

bool AJunMonster::IsExecutionReady() const
{
	return bExecutionReady && !bBeingExecuted && CurrentState != EMonsterState::Dead;
}

bool AJunMonster::CanBeExecutedBy(const AJunPlayer* Player) const
{
	if (!Player || !IsExecutionReady())
	{
		return false;
	}

	if (const UWorld* World = GetWorld();
		World && World->GetRealTimeSeconds() < ExecutionInputUnlockRealTime)
	{
		return false;
	}

	const float Range = FMath::Max(ExecutionInteractRange, 0.f);
	if (FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(Range))
	{
		return false;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	if (ToPlayer.IsNearlyZero())
	{
		return true;
	}

	const float FrontDot = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToPlayer.GetSafeNormal());
	const float MinFrontDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(ExecutionFrontAngle, 0.f, 180.f)));
	return FrontDot >= MinFrontDot;
}

bool AJunMonster::TryBeginExecutionBy(AJunPlayer* Player)
{
	if (!CanBeExecutedBy(Player))
	{
		return false;
	}

	bExecutionReady = false;
	bBeingExecuted = true;
	bExecutionResultApplied = false;
	CurrentExecutionInstigator = Player;
	CurrentExecutionMontage = nullptr;
	ExecutionReadyRemainTime = 0.f;
	ExecutionInputUnlockRealTime = 0.f;
	CurrentPosture = 0.f;
	PostureRecoveryDelayRemainTime = 0.f;
	GetWorldTimerManager().ClearTimer(ExecutionRecoveryTimerHandle);
	if (AJunPlayerController* JunPlayerController = Player ? Cast<AJunPlayerController>(Player->GetController()) : nullptr)
	{
		JunPlayerController->HideBossPostureImmediately();
	}

	StopAIMovement();
	StopAllAttackTraces();
	CancelCombatTurn();
	ResetCombatTurnState();
	if (bIsAttacking)
	{
		FinishAttack();
	}
	EndHitReact();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->StopMovementImmediately();
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	OnExecutionReadyEnded(false);

	return true;
}

void AJunMonster::TriggerPendingExecutionMontage(bool bApplyResultImmediately)
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	const bool bFinalExecution = CurrentLifeCount <= 1;
	UAnimMontage* MontageToPlay = bFinalExecution
		? (ExecutedFinishMontage ? ExecutedFinishMontage.Get() : ExecutedDeathMontage.Get())
		: ExecutedMontage.Get();

	if (MontageToPlay)
	{
		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
			if (!bFinalExecution)
			{
				MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnExecutionMontageEnded);
				CurrentExecutionMontage = MontageToPlay;
			}
		}

		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, ExecutedMontageBlendInTime));
			MonsterAnimInstance->Montage_PlayWithBlendSettings(MontageToPlay, BlendInSettings);
		}
	}

	if (bApplyResultImmediately)
	{
		ApplyPendingExecutionResult();
	}

	if (!MontageToPlay && !bFinalExecution)
	{
		FinishExecutionRecovery();
	}
}

void AJunMonster::ApplyPendingExecutionResult()
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	bExecutionResultApplied = true;
	CurrentLifeCount = FMath::Max(0, CurrentLifeCount - 1);
	CurrentPosture = 0.f;
	Hp = 0;

	if (CurrentLifeCount <= 0)
	{
		EndAttackFacingWindow();
		AddGameplayTag(JunGameplayTags::State_Condition_Dead);
		SetMonsterState(EMonsterState::Dead);
	}
}

void AJunMonster::CancelPendingExecution()
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	RestoreExecutionCapsuleCollisionIgnore();
	RestoreExecutionCapsuleRadius();
	bBeingExecuted = false;
	ExecutionReadyRemainTime = 0.f;
	CurrentPosture = 0.f;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();

	if (Hp <= 0 && CurrentLifeCount > 0)
	{
		Hp = 1;
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
}

void AJunMonster::RestoreExecutionCapsuleCollisionIgnore()
{
	AJunPlayer* ExecutionInstigator = CurrentExecutionInstigator.Get();
	if (!ExecutionInstigator)
	{
		return;
	}

	if (UCapsuleComponent* MonsterCapsule = GetCapsuleComponent())
	{
		MonsterCapsule->IgnoreActorWhenMoving(ExecutionInstigator, false);
	}

	if (UCapsuleComponent* PlayerCapsule = ExecutionInstigator->GetCapsuleComponent())
	{
		PlayerCapsule->IgnoreActorWhenMoving(this, false);
	}
}

void AJunMonster::RestoreExecutionCapsuleRadius()
{
	if (UCapsuleComponent* MonsterCapsule = GetCapsuleComponent())
	{
		if (bExecutionCapsuleRadiusReduced && SavedExecutionCapsuleRadius > 0.f)
		{
			MonsterCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius, true);
		}
	}

	SavedExecutionCapsuleRadius = 0.f;
	bExecutionCapsuleRadiusReduced = false;
}

void AJunMonster::ReduceExecutionCapsuleRadius()
{
	if (UCapsuleComponent* MonsterCapsule = GetCapsuleComponent())
	{
		if (!bExecutionCapsuleRadiusReduced)
		{
			SavedExecutionCapsuleRadius = MonsterCapsule->GetUnscaledCapsuleRadius();
			bExecutionCapsuleRadiusReduced = true;
		}

		MonsterCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius / 3.f, true);
	}
}

bool AJunMonster::HasExecutionResultApplied() const
{
	return bExecutionResultApplied;
}


void AJunMonster::StartExecutionReady()
{
	if (CurrentState == EMonsterState::Dead || bExecutionReady || bBeingExecuted)
	{
		return;
	}

	bExecutionReady = true;
	ExecutionReadyRemainTime = FMath::Max(0.1f, ExecutionReadyDuration);
	ExecutionInputUnlockRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	CurrentPosture = MaxPosture;
	PostureRecoveryDelayRemainTime = ExecutionReadyRemainTime;

	StopAIMovement();
	StopAllAttackTraces();
	CancelCombatTurn();
	ResetCombatTurnState();
	if (bIsAttacking)
	{
		FinishAttack();
	}
	EndHitReact();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->StopMovementImmediately();
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	OnExecutionReadyStarted();

	if (ReadyExecutedMontage)
	{
		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, ExecutionReadyMontageBlendInTime));
			MonsterAnimInstance->Montage_PlayWithBlendSettings(ReadyExecutedMontage, BlendInSettings);
		}
	}

	if (bEnableExecutionReadySlowMotion && GetWorld())
	{
		if (UJunTimeEffectSubsystem* TimeEffectSubsystem = GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
		{
			const bool bSlowMotionStarted = TimeEffectSubsystem->RequestSlowMotion(
				ExecutionReadySlowMotionDuration,
				ExecutionReadySlowMotionTimeScale,
				ExecutionReadySlowMotionBlendInTime,
				ExecutionReadySlowMotionBlendOutTime,
				ExecutionReadySlowMotionPriority);
			if (bSlowMotionStarted)
			{
				ExecutionInputUnlockRealTime = GetWorld()->GetRealTimeSeconds() +
					FMath::Max(0.f, ExecutionReadySlowMotionDuration);
			}
		}
	}
}

void AJunMonster::EndExecutionReady()
{
	if (!bExecutionReady || bBeingExecuted)
	{
		return;
	}

	bExecutionReady = false;
	ExecutionReadyRemainTime = 0.f;
	ExecutionInputUnlockRealTime = 0.f;
	CurrentPosture = MaxPosture > 0.f
		? FMath::Clamp(MaxPosture * MissedExecutionPostureRatio, 0.f, MaxPosture - KINDA_SMALL_NUMBER)
		: 0.f;
	PostureRecoveryDelayRemainTime = PostureRecoveryDelay;

	// HP 0으로 처형 대기가 끝나면 몬스터가 즉시 Dead 태그를 얻지 않도록 최소 생존 HP를 보장한다.
	if (Hp <= 0 && CurrentLifeCount > 0)
	{
		Hp = 1;
	}

	if (ReadyExecutedMontage)
	{
		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			MonsterAnimInstance->Montage_Stop(0.15f, ReadyExecutedMontage);
		}
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	OnExecutionReadyEnded(true);
}

void AJunMonster::OnExecutionReadyStarted()
{
}

void AJunMonster::OnExecutionReadyEnded(bool bMissedExecution)
{
}

void AJunMonster::FinishExecutionRecovery()
{
	if (!bBeingExecuted || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	RestoreExecutionCapsuleCollisionIgnore();
	RestoreExecutionCapsuleRadius();
	Hp = MaxHp;
	bBeingExecuted = false;
	bExecutionResultApplied = false;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();
	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
	}
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	if (CurrentTarget)
	{
		SetMonsterState(EMonsterState::Combat);
	}
	else
	{
		SetMonsterState(EMonsterState::Idle);
	}
}

void AJunMonster::OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentExecutionMontage)
	{
		return;
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
	}

	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();

	if (!bBeingExecuted || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	FinishExecutionRecovery();
}

float AJunMonster::GetExecutionMontageDuration(const UAnimMontage* Montage, float FallbackDuration) const
{
	return Montage ? FMath::Max(Montage->GetPlayLength(), 0.01f) : FallbackDuration;
}
