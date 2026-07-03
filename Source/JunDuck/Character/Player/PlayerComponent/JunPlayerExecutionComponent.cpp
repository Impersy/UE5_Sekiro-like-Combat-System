#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"

#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Character/JunCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunExecutionTargetInterface.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "System/JunBGMManager.h"
#include "System/JunTimeEffectSubsystem.h"

UJunPlayerExecutionComponent::UJunPlayerExecutionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJunPlayerExecutionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayer = Cast<AJunPlayer>(GetOwner());
}

bool UJunPlayerExecutionComponent::TryStartExecution()
{
	if (!OwnerPlayer || bIsExecuting)
	{
		return false;
	}

	AActor* ExecutionTarget = OwnerPlayer->LockOnTarget.Get();
	if (!CanStartExecution(ExecutionTarget))
	{
		ExecutionTarget = OwnerPlayer->FindBestAttackTarget();
	}

	if (!CanStartExecution(ExecutionTarget))
	{
		return false;
	}

	StartExecution(ExecutionTarget);
	return true;
}

bool UJunPlayerExecutionComponent::CanStartExecution(const AActor* ExecutionTargetActor) const
{
	if (!OwnerPlayer)
	{
		return false;
	}

	if (OwnerPlayer->GetCharacterMovement() && OwnerPlayer->GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	const IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionTargetActor);
	return ExecutionTargetActor
		&& ExecutionTarget
		&& GetExecutionMontageForTarget(ExecutionTarget)
		&& OwnerPlayer->AnimInstance
		&& ExecutionTarget->CanBeExecutedBy(OwnerPlayer);
}

void UJunPlayerExecutionComponent::StartExecution(AActor* ExecutionTargetActor)
{
	if (!OwnerPlayer || !CanStartExecution(ExecutionTargetActor))
	{
		return;
	}

	IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionTargetActor);
	if (!ExecutionTarget || !ExecutionTarget->TryBeginExecutionBy(OwnerPlayer))
	{
		return;
	}

	OwnerPlayer->InterruptActionsForExecution();
	OwnerPlayer->FinishPlayerHitState();

	bIsExecuting = true;
	OwnerPlayer->SetActionState(EJunPlayerActionState::Execution, EJunActionTransitionReason::ForcedByExecution);
	CurrentExecutionTarget = ExecutionTargetActor;
	OwnerPlayer->bCurrentExecutionIsFinish = ExecutionTarget->IsFinalExecution();
	CurrentExecutionMontage = GetExecutionMontageForTarget(ExecutionTarget);
	OwnerPlayer->TargetActor = Cast<AJunCharacter>(ExecutionTargetActor);
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(OwnerPlayer))
	{
		BGMManager->SetExecutionBGMDucked(true);
	}
	if (OwnerPlayer->ExecutionStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			OwnerPlayer,
			OwnerPlayer->ExecutionStartSound,
			OwnerPlayer->GetActorLocation(),
			OwnerPlayer->ExecutionStartSoundVolume);
	}
	if (OwnerPlayer->GetWorld())
	{
		const bool bUseFinishSlowMotion =
			OwnerPlayer->bCurrentExecutionIsFinish && OwnerPlayer->bEnableExecutionFinishStartSlowMotion;
		const bool bUseNormalSlowMotion =
			!OwnerPlayer->bCurrentExecutionIsFinish && OwnerPlayer->bEnableExecutionStartSlowMotion;
		if (bUseFinishSlowMotion || bUseNormalSlowMotion)
		{
			const float SlowMotionDuration = bUseFinishSlowMotion
				? OwnerPlayer->ExecutionFinishStartSlowMotionDuration
				: OwnerPlayer->ExecutionStartSlowMotionDuration;
			const float SlowMotionTimeScale = bUseFinishSlowMotion
				? OwnerPlayer->ExecutionFinishStartSlowMotionTimeScale
				: OwnerPlayer->ExecutionStartSlowMotionTimeScale;
			const float SlowMotionBlendInTime = bUseFinishSlowMotion
				? OwnerPlayer->ExecutionFinishStartSlowMotionBlendInTime
				: OwnerPlayer->ExecutionStartSlowMotionBlendInTime;
			const float SlowMotionBlendOutTime = bUseFinishSlowMotion
				? OwnerPlayer->ExecutionFinishStartSlowMotionBlendOutTime
				: OwnerPlayer->ExecutionStartSlowMotionBlendOutTime;

			if (UJunTimeEffectSubsystem* TimeEffectSubsystem = OwnerPlayer->GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
			{
				TimeEffectSubsystem->RequestSlowMotion(
					SlowMotionDuration,
					SlowMotionTimeScale,
					SlowMotionBlendInTime,
					SlowMotionBlendOutTime,
					OwnerPlayer->ExecutionStartSlowMotionPriority);
			}
		}
	}
	SetExecutionCapsuleCollisionIgnore(ExecutionTargetActor, true);
	ReduceExecutionCapsuleRadius(ExecutionTargetActor);
	OwnerPlayer->StartExecutionCamera(ExecutionTargetActor);

	const FVector ToTarget = ExecutionTargetActor->GetActorLocation() - OwnerPlayer->GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		OwnerPlayer->SetActorRotation(ToTarget.ToOrientationRotator());
	}

	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Action_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(this, &UJunPlayerExecutionComponent::OnExecutionMontageEnded);
	OwnerPlayer->AnimInstance->OnMontageEnded.AddDynamic(this, &UJunPlayerExecutionComponent::OnExecutionMontageEnded);
	const FMontageBlendSettings ExecutionBlendInSettings(FMath::Max(0.f, OwnerPlayer->ExecutionMontageBlendInTime));
	const float PlayResult = OwnerPlayer->AnimInstance->Montage_PlayWithBlendSettings(
		CurrentExecutionMontage,
		ExecutionBlendInSettings);
	if (PlayResult <= 0.f)
	{
		UE_LOG(LogJun, Warning,
			TEXT("Execution montage play failed. Player=%s Target=%s Montage=%s"),
			*GetNameSafe(OwnerPlayer),
			*GetNameSafe(ExecutionTargetActor),
			*GetNameSafe(CurrentExecutionMontage));
		FinishExecution(true);
	}
}

void UJunPlayerExecutionComponent::TriggerExecutionTargetMontage()
{
	if (!OwnerPlayer || !bIsExecuting || !CurrentExecutionTarget)
	{
		return;
	}

	if (IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(CurrentExecutionTarget.Get()))
	{
		ExecutionTarget->TriggerPendingExecutionMontage(!OwnerPlayer->bCurrentExecutionIsFinish);
	}
}

void UJunPlayerExecutionComponent::FinishExecution(bool bInterrupted)
{
	if (!OwnerPlayer || !bIsExecuting)
	{
		return;
	}

	if (OwnerPlayer->AnimInstance)
	{
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(this, &UJunPlayerExecutionComponent::OnExecutionMontageEnded);
	}

	bIsExecuting = false;
	OwnerPlayer->EndAttackFacingWindow();
	OwnerPlayer->EndExecutionCamera();
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(OwnerPlayer))
	{
		BGMManager->SetExecutionBGMDucked(false);
	}
	const bool bWasFinishExecution = OwnerPlayer->bCurrentExecutionIsFinish;
	IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(CurrentExecutionTarget.Get());
	if (ExecutionTarget && !ExecutionTarget->HasExecutionResultApplied())
	{
		if (bInterrupted)
		{
			ExecutionTarget->CancelPendingExecution();
		}
		else
		{
			UE_LOG(LogJun, Warning,
				TEXT("Execution finished before target result was applied. Forcing result. Player=%s Target=%s Montage=%s"),
				*GetNameSafe(OwnerPlayer),
				*GetNameSafe(CurrentExecutionTarget),
				*GetNameSafe(CurrentExecutionMontage));
			ExecutionTarget->TriggerPendingExecutionMontage(!bWasFinishExecution);
			if (!ExecutionTarget->HasExecutionResultApplied())
			{
				ExecutionTarget->ApplyPendingExecutionResult();
			}
		}
	}
	OwnerPlayer->bCurrentExecutionIsFinish = false;
	SetExecutionCapsuleCollisionIgnore(CurrentExecutionTarget.Get(), false);
	RestoreExecutionCapsuleRadius(CurrentExecutionTarget.Get());
	CurrentExecutionTarget = nullptr;
	CurrentExecutionMontage = nullptr;

	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	OwnerPlayer->ClearActionStateIf(EJunPlayerActionState::Execution, EJunActionTransitionReason::System);
}

void UJunPlayerExecutionComponent::SetExecutionCapsuleCollisionIgnore(AActor* ExecutionTargetActor, bool bIgnore)
{
	if (!OwnerPlayer || !ExecutionTargetActor)
	{
		return;
	}

	const IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionTargetActor);
	if (bIgnore && (!ExecutionTarget || !ExecutionTarget->ShouldIgnoreExecutorCapsuleCollisionDuringExecution()))
	{
		return;
	}

	if (UCapsuleComponent* PlayerCapsule = OwnerPlayer->GetCapsuleComponent())
	{
		PlayerCapsule->IgnoreActorWhenMoving(ExecutionTargetActor, bIgnore);
	}

	if (ACharacter* ExecutionTargetCharacter = Cast<ACharacter>(ExecutionTargetActor))
	{
		if (UCapsuleComponent* TargetCapsule = ExecutionTargetCharacter->GetCapsuleComponent())
		{
			TargetCapsule->IgnoreActorWhenMoving(OwnerPlayer, bIgnore);
		}
	}
}

void UJunPlayerExecutionComponent::ReduceExecutionCapsuleRadius(AActor* ExecutionTargetActor)
{
	if (!OwnerPlayer)
	{
		return;
	}

	if (UCapsuleComponent* PlayerCapsule = OwnerPlayer->GetCapsuleComponent())
	{
		if (!bExecutionCapsuleRadiusReduced)
		{
			SavedExecutionCapsuleRadius = PlayerCapsule->GetUnscaledCapsuleRadius();
			bExecutionCapsuleRadiusReduced = true;
		}

		PlayerCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius / 3.f, true);
	}

	if (IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionTargetActor))
	{
		ExecutionTarget->ReduceExecutionCapsuleRadius();
	}
}

void UJunPlayerExecutionComponent::RestoreExecutionCapsuleRadius(AActor* ExecutionTargetActor)
{
	if (OwnerPlayer)
	{
		if (UCapsuleComponent* PlayerCapsule = OwnerPlayer->GetCapsuleComponent())
		{
			if (bExecutionCapsuleRadiusReduced && SavedExecutionCapsuleRadius > 0.f)
			{
				PlayerCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius, true);
			}
		}
	}

	SavedExecutionCapsuleRadius = 0.f;
	bExecutionCapsuleRadiusReduced = false;

	if (ExecutionTargetActor)
	{
		if (IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionTargetActor))
		{
			ExecutionTarget->RestoreExecutionCapsuleRadius();
		}
	}
}

UAnimMontage* UJunPlayerExecutionComponent::GetExecutionMontageForTarget(
	const IJunExecutionTargetInterface* ExecutionTarget) const
{
	if (!OwnerPlayer)
	{
		return nullptr;
	}

	if (ExecutionTarget && ExecutionTarget->IsFinalExecution())
	{
		return OwnerPlayer->ExecutionFinishMontage
			? OwnerPlayer->ExecutionFinishMontage.Get()
			: OwnerPlayer->ExecutionMontage.Get();
	}

	return OwnerPlayer->ExecutionMontage.Get();
}

void UJunPlayerExecutionComponent::OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentExecutionMontage)
	{
		return;
	}

	FinishExecution(bInterrupted);
}
