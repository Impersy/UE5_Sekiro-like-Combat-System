#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunMikiriCounterTargetInterface.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "System/JunCombatVFXSubsystem.h"
#include "EngineUtils.h"

#pragma region Mikiri Counter
// ============================================================================
// Mikiri Counter
// ============================================================================

bool AJunPlayer::TryOpenMikiriCounterWindow(bool bRequireForwardInput)
{
	if (bMikiriCounterWindowOpen || MikiriCounterReadyRemainTime > 0.f || CurrentMikiriParryReadyMontage)
	{
		return true;
	}

	if (DeathState != EJunPlayerDeathState::Alive ||
		Is_Dead() ||
		IsExecuting() ||
		IsInDeathSequence() ||
		CurrentHitState != EJunPlayerHitState::None ||
		(GetCharacterMovement() && GetCharacterMovement()->IsFalling()))
	{
		return false;
	}

	if (bRequireForwardInput &&
		(HasGameplayTag(JunGameplayTags::State_Block_Dodge) ||
			HasGameplayTag(JunGameplayTags::State_Action_Dodge)))
	{
		return false;
	}

	const float ForwardInput = FMath::Max(DesiredMoveForward, PendingMoveForward);
	if (bRequireForwardInput && ForwardInput < MikiriCounterForwardInputThreshold)
	{
		return false;
	}

	AActor* MikiriReferenceActor = FindBestMikiriCounterTarget();
	if (!MikiriReferenceActor)
	{
		return false;
	}

	if (!bRequireForwardInput && HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (AnimInstance && CurrentDodgeMontage)
		{
			AnimInstance->Montage_Stop(FMath::Max(0.f, MikiriCounterDodgeBlendOutTime), CurrentDodgeMontage);
		}
		FinishDodgeState();
		RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	}

	CurrentMikiriParryReadyMontage = GetMikiriParryReadyMontage(MikiriReferenceActor);
	const float ReadyDuration = CurrentMikiriParryReadyMontage
		? FMath::Max(MikiriCounterReadyMinDuration, CurrentMikiriParryReadyMontage->GetPlayLength())
		: MikiriCounterReadyMinDuration;

	bMikiriCounterWindowOpen = true;
	MikiriCounterWindowRemainTime = FMath::Max(MikiriCounterWindowDuration, KINDA_SMALL_NUMBER);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	ConsumeMovementInputVector();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	if (CurrentMikiriParryReadyMontage)
	{
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.f, CurrentMikiriParryReadyMontage);
		}

		PlayAnimMontage(CurrentMikiriParryReadyMontage);
	}
	MikiriCounterReadyRemainTime = ReadyDuration;

	return true;
}

bool AJunPlayer::IsMikiriCounterThreatAvailable() const
{
	return FindBestMikiriCounterTarget() != nullptr;
}

AActor* AJunPlayer::GetMikiriCounterReferenceActor() const
{
	return FindBestMikiriCounterTarget();
}

AActor* AJunPlayer::FindBestMikiriCounterTarget() const
{
	const IJunMikiriCounterTargetInterface* LockOnMikiriTarget = Cast<IJunMikiriCounterTargetInterface>(LockOnTarget.Get());
	if (LockOnMikiriTarget &&
		LockOnMikiriTarget->IsMikiriCounterThreatActiveFor(this) &&
		LockOnMikiriTarget->CanBeMikiriCounteredBy(this))
	{
		return LockOnTarget.Get();
	}

	const UWorld* World = GetWorld();
	if (!World || MikiriCounterThreatSearchRadius <= 0.f)
	{
		return nullptr;
	}

	const float SearchRadiusSq = FMath::Square(MikiriCounterThreatSearchRadius);
	AActor* BestActor = nullptr;
	float BestDistanceSq = SearchRadiusSq;
	for (TActorIterator<AJunCharacter> CharacterIt(World); CharacterIt; ++CharacterIt)
	{
		AJunCharacter* OtherCharacter = *CharacterIt;
		const IJunMikiriCounterTargetInterface* MikiriTarget = Cast<IJunMikiriCounterTargetInterface>(OtherCharacter);
		if (!OtherCharacter ||
			OtherCharacter == this ||
			!MikiriTarget ||
			!MikiriTarget->IsMikiriCounterThreatActiveFor(this) ||
			!MikiriTarget->CanBeMikiriCounteredBy(this))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), OtherCharacter->GetActorLocation());
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestActor = OtherCharacter;
		}
	}

	return BestActor;
}

void AJunPlayer::UpdateMikiriCounterWindow(float DeltaTime)
{
	if (MikiriCounterReadyRemainTime > 0.f)
	{
		MikiriCounterReadyRemainTime = FMath::Max(0.f, MikiriCounterReadyRemainTime - DeltaTime);
		if (MikiriCounterReadyRemainTime <= 0.f)
		{
			FinishMikiriCounterReady();
			return;
		}
	}

	if (!bMikiriCounterWindowOpen)
	{
		return;
	}

	MikiriCounterWindowRemainTime = FMath::Max(0.f, MikiriCounterWindowRemainTime - DeltaTime);
	if (MikiriCounterWindowRemainTime <= 0.f)
	{
		FinishMikiriCounterReady();
	}
}

bool AJunPlayer::TryHandleMikiriCounter(AActor* DamageCauser)
{
	if (!bMikiriCounterWindowOpen)
	{
		return false;
	}

	if (!IsDamageCauserInDefenseAngle(DamageCauser))
	{
		FinishMikiriCounterReady();
		return false;
	}

	IJunMikiriCounterTargetInterface* MikiriTarget = Cast<IJunMikiriCounterTargetInterface>(DamageCauser);
	if (!MikiriTarget ||
		!MikiriTarget->CanBeMikiriCounteredBy(this) ||
		!MikiriTarget->NotifyMikiriCounteredBy(this))
	{
		FinishMikiriCounterReady();
		return false;
	}

	bMikiriCounterWindowOpen = false;
	MikiriCounterWindowRemainTime = 0.f;
	PlayDefenseSound(EJunDefenseSoundType::PerfectParry);
	PlayDefenseCameraShake(MikiriParryCameraShakeScale);
	StartMikiriCounterSuccess(DamageCauser);
	return true;
}

void AJunPlayer::StartMikiriCounterSuccess(AActor* DamageCauser)
{
	ResetParrySpamPenalty();

	InterruptActionsForHitReaction();
	PlayDefenseEffect(
		EJunCombatDefenseVFX::PerfectParry,
		CachedParryEffectLocationComponent,
		ParryEffectLocationComponentName);
	if (AnimInstance && CurrentMikiriParryReadyMontage)
	{
		AnimInstance->Montage_Stop(0.05f, CurrentMikiriParryReadyMontage);
	}
	CurrentMikiriParryReadyMontage = nullptr;
	MikiriCounterReadyRemainTime = 0.f;
	bMikiriCounterWindowOpen = false;
	MikiriCounterWindowRemainTime = 0.f;

	CurrentHitState = EJunPlayerHitState::ParrySuccess;
	ChainParryWindowRemainTime = ChainParryWindowDuration;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	PendingParrySuccessActionRequest.Reset();
	bParrySuccessHeavyAttackInputHeld = false;
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	CurrentParrySuccessMontage = GetMikiriParryMontage(DamageCauser);
	PlayerHitStateRemainTime = CurrentParrySuccessMontage
		? CurrentParrySuccessMontage->GetPlayLength()
		: ParrySuccessDuration;

	if (CurrentParrySuccessMontage)
	{
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.f, CurrentParrySuccessMontage);
		}

		PlayAnimMontage(CurrentParrySuccessMontage);
	}
}

void AJunPlayer::FinishMikiriCounterReady()
{
	bMikiriCounterWindowOpen = false;
	MikiriCounterWindowRemainTime = 0.f;
	MikiriCounterReadyRemainTime = 0.f;
	CurrentMikiriParryReadyMontage = nullptr;
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
}

UAnimMontage* AJunPlayer::GetMikiriParryMontage(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return MikiriParryMontage.Get();
	}

	const FVector ToPlayer = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
	if (ToPlayer.IsNearlyZero())
	{
		return MikiriParryMontage.Get();
	}

	const float RightDot = FVector::DotProduct(DamageCauser->GetActorRightVector().GetSafeNormal2D(), ToPlayer);
	if (RightDot > 0.f && MikiriParryRMontage)
	{
		return MikiriParryRMontage.Get();
	}

	return MikiriParryMontage.Get();
}

UAnimMontage* AJunPlayer::GetMikiriParryReadyMontage(const AActor* ReferenceActor) const
{
	if (!ReferenceActor)
	{
		return MikiriParryReadyMontage.Get();
	}

	const FVector ToPlayer = (GetActorLocation() - ReferenceActor->GetActorLocation()).GetSafeNormal2D();
	if (ToPlayer.IsNearlyZero())
	{
		return MikiriParryReadyMontage.Get();
	}

	const float RightDot = FVector::DotProduct(ReferenceActor->GetActorRightVector().GetSafeNormal2D(), ToPlayer);
	if (RightDot > 0.f && MikiriParryReadyRMontage)
	{
		return MikiriParryReadyRMontage.Get();
	}

	return MikiriParryReadyMontage.Get();
}

#pragma endregion
