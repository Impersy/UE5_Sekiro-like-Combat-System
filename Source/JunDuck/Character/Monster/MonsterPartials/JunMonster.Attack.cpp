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

namespace
{
	const TCHAR* GetMonsterStateDebugText(EMonsterState State)
	{
		switch (State)
		{
		case EMonsterState::CutsceneWait:
			return TEXT("CutsceneWait");
		case EMonsterState::Idle:
			return TEXT("Idle");
		case EMonsterState::Patrol:
			return TEXT("Patrol");
		case EMonsterState::Chase:
			return TEXT("Chase");
		case EMonsterState::Return:
			return TEXT("Return");
		case EMonsterState::BattleStart:
			return TEXT("BattleStart");
		case EMonsterState::Combat:
			return TEXT("Combat");
		case EMonsterState::Dead:
			return TEXT("Dead");
		default:
			return TEXT("Unknown");
		}
	}
}

void AJunMonster::BeginAttackFacingWindow(float InterpSpeed)
{
	bAttackFacingWindowActive = true;
	AttackFacingWindowInterpSpeed = FMath::Max(InterpSpeed, 0.f);
}

void AJunMonster::EndAttackFacingWindow()
{
	bAttackFacingWindowActive = false;
	AttackFacingWindowInterpSpeed = 0.f;
}

void AJunMonster::BeginHitReactFacingWindow(float InterpSpeed)
{
	if (!HitReactFacingTarget)
	{
		bHitReactFacingWindowActive = false;
		HitReactFacingWindowInterpSpeed = 0.f;
		return;
	}

	bHitReactFacingWindowActive = true;
	HitReactFacingWindowInterpSpeed = FMath::Max(InterpSpeed, 0.f);
}

void AJunMonster::EndHitReactFacingWindow()
{
	bHitReactFacingWindowActive = false;
	HitReactFacingWindowInterpSpeed = 0.f;
}

void AJunMonster::TryAttack()
{
	if (!CanAttackTarget())
	{
		return;
	}

	const FMonsterAttackSelection AttackSelection = ChooseNextAttackSelection();
	UAnimMontage* AttackMontageToPlay = AttackSelection.Montage.Get();
	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	CurrentAttackSelection = AttackSelection;
	CurrentAttackMontage = AttackMontageToPlay;
	const float AttackPlayRate = FMath::Max(AttackSelection.PlayRate, KINDA_SMALL_NUMBER);
	AttackTime = AttackMontageToPlay ? (AttackMontageToPlay->GetPlayLength() / AttackPlayRate) : DefaultAttackDuration;
	AttackFacingRemainTime = AttackSelection.FacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (AttackMontageToPlay)
	{
		PlayAnimMontage(AttackMontageToPlay, AttackPlayRate);
	}
}

void AJunMonster::UpdateAttack(float DeltaTime)
{
	if (!bIsAttacking)
	{
		return;
	}

	if (IsInHitReact())
	{
		return;
	}

	if (IsCombatTurnPlaying())
	{
		return;
	}

	if (AttackFacingRemainTime > 0.f)
	{
		AttackFacingRemainTime = FMath::Max(0.f, AttackFacingRemainTime - DeltaTime);
	}

	const bool bCanFaceDuringAttack =
		CurrentAttackSelection.bFaceTargetDuringAttack &&
		(bAttackFacingWindowActive || AttackFacingRemainTime > 0.f || CurrentAttackSelection.FacingDuration < 0.f);

	if (bCanFaceDuringAttack && CurrentTarget)
	{
		FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator CurrentRotation = GetActorRotation();
			const FRotator TargetRotation = ToTarget.Rotation();
			const float FacingInterpSpeed = bAttackFacingWindowActive && AttackFacingWindowInterpSpeed > 0.f
				? AttackFacingWindowInterpSpeed
				: CurrentAttackSelection.FacingInterpSpeed;
			const FRotator NewRotation = FMath::RInterpTo(
				CurrentRotation,
				TargetRotation,
				DeltaTime,
				FacingInterpSpeed
			);

			SetActorRotation(NewRotation);
		}
	}

	OnAttackTick(DeltaTime);

	AttackTime -= DeltaTime;
	if (AttackTime <= 0.f)
	{
		FinishAttack();
	}
}

void AJunMonster::FinishAttack()
{
	if (!bIsAttacking)
	{
		return;
	}

	StopMonsterCodeMove(true);
	bIsAttacking = false;
	AttackTime = 0.f;
	AttackFacingRemainTime = 0.f;

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	ResetCurrentAttackRuntimeState();
}

FMonsterAttackSelection AJunMonster::ChooseNextAttackSelection() const
{
	FMonsterAttackSelection Selection;
	Selection.Montage = AttackMontage;
	Selection.AttackRange = DefaultAttackRange;
	Selection.bFaceTargetDuringAttack = true;
	Selection.FacingDuration = -1.f;
	Selection.FacingInterpSpeed = AttackFacingInterpSpeed;
	Selection.PlayRate = 1.f;
	Selection.bTryTurnAfterAttack = false;
	Selection.PostAttackTurnStartAngle = CombatTurnStartAngle;
	return Selection;
}

bool AJunMonster::TryStartPostAttackTurn()
{
	return TryStartCombatTurnWithThreshold(CurrentAttackSelection.PostAttackTurnStartAngle);
}

void AJunMonster::OnAttackTick(float DeltaTime)
{
}

FString AJunMonster::GetMonsterDebugExtraText() const
{
	return FString();
}

UAnimMontage* AJunMonster::GetCurrentAttackMontage() const
{
	return CurrentAttackMontage.Get();
}

const FMonsterAttackSelection& AJunMonster::GetCurrentAttackSelection() const
{
	return CurrentAttackSelection;
}

void AJunMonster::OnCombatTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentCombatTurnMontage)
	{
		if (bDebugCombatTurnYaw)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][EndIgnored] Ended=%s Current=%s Interrupted=%s Yaw=%.2f"),
				*GetNameSafe(Montage),
				*GetNameSafe(CurrentCombatTurnMontage),
				bInterrupted ? TEXT("true") : TEXT("false"),
				GetActorRotation().Yaw);
		}
		return;
	}

	LastCombatTurnEndYaw = GetActorRotation().Yaw;
	if (bDebugCombatTurnYaw)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][End] Montage=%s Interrupted=%s Start=%.2f AfterPlay=%.2f End=%.2f TotalDelta=%.2f Velocity=%.2f State=%s"),
			*GetNameSafe(Montage),
			bInterrupted ? TEXT("true") : TEXT("false"),
			LastCombatTurnStartYaw,
			LastCombatTurnPostPlayYaw,
			LastCombatTurnEndYaw,
			FMath::FindDeltaAngleDegrees(LastCombatTurnStartYaw, LastCombatTurnEndYaw),
			GetVelocity().Size2D(),
			GetMonsterStateDebugText(CurrentState));
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	}

	bCombatTurnInProgress = false;
	CurrentCombatTurnMontage = nullptr;

	if (bHasPendingStateAfterCombatTurn)
	{
		const EMonsterState NextState = PendingStateAfterCombatTurn;
		bHasPendingStateAfterCombatTurn = false;
		PendingStateAfterCombatTurn = EMonsterState::Idle;
		SetMonsterState(NextState);
	}
}

// Weapon

