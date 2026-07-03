#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "System/JunTimeEffectSubsystem.h"

namespace
{
	bool DoesMontageUseRootMotion(const UAnimMontage* Montage)
	{
		return Montage && Montage->HasRootMotion();
	}
}

#pragma region Hit React Runtime
// ============================================================================
// Hit React Runtime
// ============================================================================

void AJunPlayer::StartParrySuccess(bool bPerfectParry)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->StartParrySuccess(bPerfectParry);
	}
}

void AJunPlayer::StartGuardBlock()
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->StartGuardBlock();
	}
}

void AJunPlayer::StartGuardBreak()
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->StartGuardBreak();
	}
}

void AJunPlayer::StartHitReact(EHitReactType HitType, ECharacterHitReactDirection HitDirection)
{
	const bool bRestartingHitReact = CurrentHitState == EJunPlayerHitState::HitReact;

	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterStompOpportunityPending = false;
	JumpCounterStompTarget = nullptr;
	JumpCounterStompPendingTarget = nullptr;

	InterruptActionsForHitReaction();
	ResetAirHeavyHitReactState();

	const EJunAirHitReactType AirHitReactType = ResolveAirHitReactType();
	const bool bUseAirHitReact = AirHitReactType != EJunAirHitReactType::None;

	if (!TryApplyForcedActionRequest(MakeForcedActionRequest(
		EJunPlayerActionState::HitReact,
		EJunActionTransitionReason::ForcedByDamage)))
	{
		return;
	}

	CurrentHitState = EJunPlayerHitState::HitReact;
	CurrentHitReactType = HitType;
	CurrentHitReactDirection = HitDirection;
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = bUseAirHitReact;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	bFreeRunSlideActive = false;
	FreeRunSlideSpeed = 0.f;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (!bUseAirHitReact)
		{
			FVector NewVelocity = MovementComponent->Velocity;
			NewVelocity.X = 0.f;
			NewVelocity.Y = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}
	ConsumeMovementInputVector();

	UAnimMontage* HitReactMontage = bUseAirHitReact ? GetAirHitReactMontage(AirHitReactType) : GetHitReactMontage(HitType, HitDirection);
	CurrentHitReactMontage = HitReactMontage;
	if (AirHitReactType == EJunAirHitReactType::Light)
	{
		PlayerHitStateRemainTime = HitReactMontage ? FMath::Min(HitReactMontage->GetPlayLength(), AirLightHitReactDuration) : AirLightHitReactDuration;
		PlayerHitControlLockRemainTime = AirLightHitControlLockDuration;
		ApplyAirLightHitFallTuning();
	}
	else if (AirHitReactType == EJunAirHitReactType::Heavy)
	{
		AirHeavyHitStage = EJunAirHeavyHitStage::Launch;
		const float LaunchMontageDuration = HitReactMontage ? HitReactMontage->GetPlayLength() : AirHeavyHitReactDuration;
		AirHeavyHitStageRemainTime = AirHeavyHitLaunchStageMaxDuration > 0.f
			? FMath::Min(LaunchMontageDuration, AirHeavyHitLaunchStageMaxDuration)
			: LaunchMontageDuration;
		PlayerHitStateRemainTime = FMath::Max(AirHeavyHitSequenceMaxDuration, AirHeavyHitStageRemainTime);
		PlayerHitControlLockRemainTime = FMath::Max(AirHeavyHitControlLockDuration, PlayerHitStateRemainTime);
		ApplyAirHeavyHitFallTuning();
	}
	else if (HitType == EHitReactType::LightHit)
	{
		PlayerHitStateRemainTime = LightHitReactDuration;
		PlayerHitControlLockRemainTime = LightHitControlLockDuration;
	}
	else if (HitType == EHitReactType::HeavyHit_A ||
		HitType == EHitReactType::HeavyHit_B ||
		HitType == EHitReactType::HeavyHit_C)
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : HeavyHitReactDuration;
		switch (HitType)
		{
		case EHitReactType::HeavyHit_A:
			PlayerHitControlLockRemainTime = HeavyHitAControlLockDuration;
			break;
		case EHitReactType::HeavyHit_B:
			PlayerHitControlLockRemainTime = HeavyHitBControlLockDuration;
			break;
		case EHitReactType::HeavyHit_C:
			PlayerHitControlLockRemainTime = HeavyHitCControlLockDuration;
			break;
		default:
			PlayerHitControlLockRemainTime = HeavyHitAControlLockDuration;
			break;
		}
	}
	else if (HitType == EHitReactType::LargeHit_Short)
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : LargeHitReactDuration;
		PlayerHitControlLockRemainTime = LargeHitControlLockDuration;
	}
	else if (HitType == EHitReactType::LargeHit_Long)
	{
		PlayerHitStateRemainTime = LargeHitLongReactDuration;
		PlayerHitControlLockRemainTime = LargeHitLongControlLockDuration;
	}
	else if (HitType == EHitReactType::Lighting_Shock)
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : LightingShockDuration;
		PlayerHitControlLockRemainTime = LightingShockControlLockDuration;
	}
	else
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : HitReactDuration;
		PlayerHitControlLockRemainTime = HitReactDuration;
	}

	if (GuardBreakVulnerableRemainTime > 0.f)
	{
		PlayerHitControlLockRemainTime = FMath::Max(PlayerHitControlLockRemainTime, GuardBreakVulnerableRemainTime);
	}

	if (bUseAirHitReact)
	{
		ApplyAirHitKnockback(AirHitReactType);
	}
	else if (!DoesMontageUseRootMotion(HitReactMontage))
	{
		ApplyCommonKnockback(
			LastIncomingKnockbackDirection,
			LastIncomingDefenseKnockbackData.HitReact.Strength,
			LastIncomingDefenseKnockbackData.HitReact.BrakingDeceleration,
			LastIncomingDefenseKnockbackData.HitReact.GroundFriction,
			LastIncomingDefenseKnockbackData.HitReact.BrakingFrictionFactor,
			LastIncomingDefenseKnockbackData.HitReact.OverrideDuration
		);
	}

	if (!bUseAirHitReact)
	{
		AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	}
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	if (AirHitReactType == EJunAirHitReactType::Heavy)
	{
		AddGameplayTag(JunGameplayTags::State_Block_Move);
		AddGameplayTag(JunGameplayTags::State_Block_Jump);
		AddGameplayTag(JunGameplayTags::State_Block_Attack);
		AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	}

	if (HitReactMontage)
	{
		RequestPlayerHitReactHitStop();
		PlayHitReactMontageWithBlend(HitReactMontage, bRestartingHitReact);
	}
}

void AJunPlayer::RequestPlayerHitReactHitStop()
{
	if (!bEnablePlayerHitReactHitStop || !GetWorld() || PlayerHitReactHitStopDuration <= 0.f)
	{
		return;
	}

	AActor* AttackerActor = HitReactFacingTarget.Get();
	if (!IsValid(AttackerActor) || AttackerActor == this)
	{
		return;
	}

	if (UJunTimeEffectSubsystem* TimeEffectSubsystem = GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
	{
		TimeEffectSubsystem->RequestHitStopForActors(
			{ this, AttackerActor },
			PlayerHitReactHitStopDuration,
			PlayerHitReactHitStopTimeScale,
			PlayerHitReactHitStopPriority);
	}
}

void AJunPlayer::PlayHitReactMontageWithBlend(UAnimMontage* HitReactMontage, bool bRestartingHitReact)
{
	if (!HitReactMontage)
	{
		return;
	}

	UAnimInstance* PlayerAnimInstance = AnimInstance ? AnimInstance.Get() : GetPlayerAnimInstance();
	if (!PlayerAnimInstance)
	{
		PlayAnimMontage(HitReactMontage);
		return;
	}

	const float BlendInTime = bRestartingHitReact ? HitReactRestartBlendInTime : HitReactBlendInTime;
	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, BlendInTime));
	if (PlayerAnimInstance->Montage_PlayWithBlendSettings(HitReactMontage, BlendInSettings) <= 0.f)
	{
		PlayAnimMontage(HitReactMontage);
	}
}

void AJunPlayer::ApplyCommonKnockback(
	ECharacterKnockbackDirection KnockbackDirectionType,
	float Strength,
	float BrakingDecelerationOverride,
	float GroundFrictionOverride,
	float BrakingFrictionFactorOverride,
	float OverrideDuration
)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || Strength <= 0.f)
	{
		return;
	}

	FVector KnockbackDirection = FVector::ZeroVector;
	switch (KnockbackDirectionType)
	{
	case ECharacterKnockbackDirection::Forward:
		KnockbackDirection = GetActorForwardVector();
		break;
	case ECharacterKnockbackDirection::Backward:
		KnockbackDirection = -GetActorForwardVector();
		break;
	case ECharacterKnockbackDirection::Left:
		KnockbackDirection = -GetActorRightVector();
		break;
	case ECharacterKnockbackDirection::Right:
		KnockbackDirection = GetActorRightVector();
		break;
	default:
		KnockbackDirection = -GetActorForwardVector();
		break;
	}

	KnockbackDirection.Z = 0.f;
	KnockbackDirection = KnockbackDirection.GetSafeNormal();
	if (KnockbackDirection.IsNearlyZero())
	{
		return;
	}

	KnockbackBrakingDecelerationOverride = BrakingDecelerationOverride;
	KnockbackGroundFrictionOverride = GroundFrictionOverride;
	KnockbackBrakingFrictionFactorOverride = BrakingFrictionFactorOverride;
	KnockbackBrakingOverrideRemainTime = OverrideDuration;

	MovementComponent->AddImpulse(KnockbackDirection * Strength, true);
}

void AJunPlayer::InterruptActionsForHitReaction(float BlendOutTime)
{
	const float ResolvedBlendOutTime = FMath::Max(0.f, BlendOutTime);
	FinishDrinkPotion(true);

	if (bMikiriCounterWindowOpen ||
		MikiriCounterReadyRemainTime > 0.f ||
		CurrentMikiriParryReadyMontage)
	{
		if (AnimInstance && CurrentMikiriParryReadyMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, CurrentMikiriParryReadyMontage);
		}

		FinishMikiriCounterReady();
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (AnimInstance && CurrentDodgeMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, CurrentDodgeMontage);
		}

		FinishDodgeState();
	}

	if (bIsBasicAttacking)
	{
		CancelBasicAttackForRecoveryTransition(ResolvedBlendOutTime);
	}

	if (bIsHeavyAttacking)
	{
		if (AnimInstance && CurrentHeavyAttackMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, CurrentHeavyAttackMontage);
		}

		FinishHeavyAttack();
	}

	if (bIsJigenAttacking)
	{
		if (AnimInstance && JigenMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, JigenMontage);
		}

		FinishJigen();
	}

	if (bIsJumpAttacking)
	{
		if (AnimInstance && JumpAttackMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, JumpAttackMontage);
		}

		FinishJumpAttack();
	}

	if (bIsDodgeAttacking)
	{
		if (AnimInstance && CurrentDodgeAttackMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, CurrentDodgeAttackMontage);
		}

		FinishDodgeAttack();
	}

	if (GetDefenseState() != EJunDefenseState::None)
	{
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

			if (GuardStartMontage)
			{
				AnimInstance->Montage_Stop(ResolvedBlendOutTime, GuardStartMontage);
			}

			if (GuardEndMontage)
			{
				AnimInstance->Montage_Stop(ResolvedBlendOutTime, GuardEndMontage);
			}

			if (AirGuardStartMontage)
			{
				AnimInstance->Montage_Stop(ResolvedBlendOutTime, AirGuardStartMontage);
			}

			if (AirGuardEndMontage)
			{
				AnimInstance->Montage_Stop(ResolvedBlendOutTime, AirGuardEndMontage);
			}
		}

		FinishDefenseForCancel();
	}

	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::UpdatePlayerHitState(float DeltaTime)
{
	if (ChainParryWindowRemainTime > 0.f)
	{
		ChainParryWindowRemainTime = FMath::Max(0.f, ChainParryWindowRemainTime - DeltaTime);
	}

	if (CurrentHitState == EJunPlayerHitState::None)
	{
		return;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		ParrySuccessElapsedTime += DeltaTime;
		if (bParrySuccessHeavyAttackInputHeld && bHeavyAttackInputHeld)
		{
			HeavyAttackInputHoldTime += DeltaTime;
		}

		const bool bDefenseHeld = DefenseComponent
			? DefenseComponent->IsDefenseButtonHeld()
			: false;
		if (bDefenseHeld)
		{
			ParrySuccessDefenseHoldElapsedTime += DeltaTime;
			if (ParrySuccessDefenseHoldElapsedTime >= ParrySuccessDefenseHoldConfirmTime &&
				!PendingParrySuccessActionRequest.IsSet())
			{
				BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::Defense);
			}
		}
		else
		{
			ParrySuccessDefenseHoldElapsedTime = 0.f;
		}

		if (PendingParrySuccessActionRequest.IsSet())
		{
			TryProcessBufferedParrySuccessCancelAction();
			if (CurrentHitState == EJunPlayerHitState::None)
			{
				return;
			}
		}
	}

	if (PlayerHitControlLockRemainTime > 0.f)
	{
		PlayerHitControlLockRemainTime = FMath::Max(0.f, PlayerHitControlLockRemainTime - DeltaTime);
		if (PlayerHitControlLockRemainTime <= 0.f)
		{
			ReleaseHitReactControlLock();
		}
	}

	UpdateAirHeavyHitReact(DeltaTime);

	PlayerHitStateRemainTime = FMath::Max(0.f, PlayerHitStateRemainTime - DeltaTime);

	if (PlayerHitStateRemainTime <= 0.f)
	{
		FinishPlayerHitState();
	}
}

void AJunPlayer::UpdateGuardBreakVulnerability(float DeltaTime)
{
	if (GuardBreakVulnerableRemainTime <= 0.f)
	{
		return;
	}

	GuardBreakVulnerableRemainTime = FMath::Max(0.f, GuardBreakVulnerableRemainTime - DeltaTime);
}

void AJunPlayer::ReleaseHitReactControlLock()
{
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);

	if (CurrentHitState == EJunPlayerHitState::HitReact)
	{
		// Move and defense have explicit hit-react cancel paths. Jump and dodge
		// remain blocked until HitReact itself ends.
		RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
		TryCancelHitReactIntoMove();
		return;
	}

	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
}

void AJunPlayer::StopCurrentHitReactMontage(float BlendOutTime)
{
	if (!AnimInstance)
	{
		return;
	}

	if (UAnimMontage* HitReactMontage = CurrentHitReactMontage.Get())
	{
		AnimInstance->Montage_Stop(BlendOutTime, HitReactMontage);
	}
}

void AJunPlayer::ClearHitReactCancelState(bool bClearControlLock)
{
	CurrentHitState = EJunPlayerHitState::None;
	PlayerHitStateRemainTime = 0.f;
	PlayerHitControlLockRemainTime = 0.f;
	ChainParryWindowRemainTime = 0.f;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = false;
	bAirGuardBreakLandMontagePending = false;
	ResetAirHeavyHitReactState();
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();

	if (GetCharacterMovement())
	{
		RestoreDefaultMovementBrakingSettings();
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	if (bClearControlLock)
	{
		RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	}
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	ClearActionStateIf(EJunPlayerActionState::HitReact, EJunActionTransitionReason::System);
}

bool AJunPlayer::TryCancelHitReactIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::HitReact,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	const bool bHasMoveInput =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);
	if (!bHasMoveInput)
	{
		return false;
	}

	StopCurrentHitReactMontage(CancelRule.BlendOutTime);
	ClearHitReactCancelState(false);

	return true;
}

bool AJunPlayer::TryCancelHitReactIntoJump()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::HitReact,
		EJunPlayerActionState::Jump,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	StopCurrentHitReactMontage(CancelRule.BlendOutTime);
	ClearHitReactCancelState(true);

	return true;
}

bool AJunPlayer::TryCancelHitReactIntoDefense()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::HitReact,
		EJunPlayerActionState::Defense,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	StopCurrentHitReactMontage(CancelRule.BlendOutTime);

	FinishPlayerHitState();

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		StartAirParrySequence();
	}
	else
	{
		StartDefenseSequence();
	}

	return true;
}

void AJunPlayer::FinishPlayerHitState()
{
	GetWorldTimerManager().ClearTimer(GuardBlockReleaseIntoGuardEndTimerHandle);
	const bool bWasGuardBlock = CurrentHitState == EJunPlayerHitState::GuardBlock;
	const bool bWasGuardBreak = CurrentHitState == EJunPlayerHitState::GuardBreak;
	const bool bDefenseHeld = DefenseComponent
		? DefenseComponent->IsDefenseButtonHeld()
		: false;
	const bool bShouldEndGuardAfterGuardBlock = bWasGuardBlock && !bDefenseHeld;

	if (bWasGuardBreak)
	{
		CurrentPosture = 0.f;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		ClearActionStateIf(EJunPlayerActionState::ParrySuccess, EJunActionTransitionReason::System);
	}
	else if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		ClearActionStateIf(EJunPlayerActionState::GuardBlock, EJunActionTransitionReason::System);
	}
	else if (CurrentHitState == EJunPlayerHitState::GuardBreak)
	{
		ClearActionStateIf(EJunPlayerActionState::GuardBreak, EJunActionTransitionReason::System);
	}
	else if (CurrentHitState == EJunPlayerHitState::HitReact)
	{
		ClearActionStateIf(EJunPlayerActionState::HitReact, EJunActionTransitionReason::System);
	}

	CurrentHitState = EJunPlayerHitState::None;
	PlayerHitStateRemainTime = 0.f;
	PlayerHitControlLockRemainTime = 0.f;
	ChainParryWindowRemainTime = 0.f;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = false;
	bAirGuardBreakLandMontagePending = false;
	ResetAirHeavyHitReactState();
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();
	PendingParrySuccessActionRequest.Reset();
	bParrySuccessHeavyAttackInputHeld = false;
	CurrentParrySuccessMontage = nullptr;
	bGuardBlockReleasePending = false;

	if (GetCharacterMovement())
	{
		RestoreDefaultMovementBrakingSettings();
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	ReleaseHitReactControlLock();

	const EJunDefenseState CurrentDefenseState = GetDefenseState();
	if (CurrentDefenseState == EJunDefenseState::Guarding)
	{
		ApplyGuardOnBlockTags();
	}
	else if (CurrentDefenseState == EJunDefenseState::Starting || CurrentDefenseState == EJunDefenseState::Ending)
	{
		ApplyDefenseStartBlockTags();
	}

	if (bWasGuardBlock)
	{
		if (bDefenseHeld && CurrentDefenseState == EJunDefenseState::Ending)
		{
			EnterGuardLoop();
		}
		else if (bShouldEndGuardAfterGuardBlock && CurrentDefenseState == EJunDefenseState::Guarding)
		{
			BeginGuardEnd();
		}
	}
}


#pragma endregion
