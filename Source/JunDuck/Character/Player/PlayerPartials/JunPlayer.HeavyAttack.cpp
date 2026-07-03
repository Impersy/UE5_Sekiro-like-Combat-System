#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

#pragma region Heavy Attack Runtime
// ============================================================================
// Heavy Attack Runtime
// ============================================================================

void AJunPlayer::UpdateHeavyAttackInput(float DeltaTime)
{
	if (!bHeavyAttackInputHeld && !bIsHeavyAttacking)
	{
		return;
	}

	const float HeavyAttackTimelineDeltaTime = DeltaTime * GetCurrentHeavyAttackTimelineRate();

	if (bIsHeavyAttacking &&
		bHeavyAttackInputHeld &&
		HeavyAttackState == EJunHeavyAttackState::Tap)
	{
		HeavyAttackInputHoldTime += DeltaTime;
		if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold &&
			CanCancelHeavyAttackIntoHeavyAttack())
		{
			CancelHeavyAttackForRecoveryTransition(HeavyAttackRestartBlendOutTime);
			StartHeavyAttackCharge();
			return;
		}
	}

	if (!bIsHeavyAttacking)
	{
		if (!CanStartHeavyAttack())
		{
			if (bHeavyAttackInputHeld && bIsJigenAttacking)
			{
				HeavyAttackInputHoldTime += DeltaTime;
				return;
			}

			if (bHeavyAttackInputHeld && CanBufferDefenseTransitionCancel())
			{
				HeavyAttackInputHoldTime += DeltaTime;
				return;
			}

			ResetHeavyAttackChargeInput();
			return;
		}

		HeavyAttackInputHoldTime += DeltaTime;
		if (!bHeavyAttackChargeStartPlayed && HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
		{
			StartHeavyAttackCharge();
		}

		return;
	}

	if (HeavyAttackState == EJunHeavyAttackState::ChargeLoop)
	{
		HeavyAttackChargeLoopElapsedTime += HeavyAttackTimelineDeltaTime;
		HeavyAttackSectionElapsedTime += HeavyAttackTimelineDeltaTime;
		if (bHeavyAttackChargeEndRequested &&
			AnimInstance &&
			AnimInstance->Montage_GetCurrentSection(HeavyAttackChargeMontage) == HeavyAttackChargeEndSectionName)
		{
			HeavyAttackState = EJunHeavyAttackState::ChargeEnd;
			HeavyAttackSectionElapsedTime = 0.f;
			ProcessHeavyAttackChargeEndDash();
			ResetHeavyAttackChargeInput();
			return;
		}

		if (!bHeavyAttackChargeEndRequested &&
			HeavyAttackChargeLoopElapsedTime >= HeavyAttackChargeLoopMaxDuration)
		{
			bHeavyAttackInputHeld = false;
			bHeavyAttackChargeEndRequested = true;

			if (AnimInstance && HeavyAttackChargeMontage)
			{
				AnimInstance->Montage_SetNextSection(
					HeavyAttackChargeLoopSectionName,
					HeavyAttackChargeEndSectionName,
					HeavyAttackChargeMontage
				);
			}
		}
	}
	else if (HeavyAttackState == EJunHeavyAttackState::ChargeStart)
	{
		HeavyAttackSectionElapsedTime += HeavyAttackTimelineDeltaTime;
		HeavyAttackChargeStartRemainTime = FMath::Max(0.f, HeavyAttackChargeStartRemainTime - HeavyAttackTimelineDeltaTime);
		if (HeavyAttackChargeStartRemainTime <= 0.f)
		{
			if (bHeavyAttackInputHeld && !bHeavyAttackChargeEndRequested)
			{
				HeavyAttackState = EJunHeavyAttackState::ChargeLoop;
				HeavyAttackChargeStartRemainTime = 0.f;
				HeavyAttackChargeLoopElapsedTime = 0.f;
				HeavyAttackSectionElapsedTime = 0.f;
			}
			else
			{
				StartHeavyAttackChargeEnd();
			}
		}
	}
	else if (HeavyAttackState == EJunHeavyAttackState::Tap ||
		HeavyAttackState == EJunHeavyAttackState::ChargeEnd)
	{
		HeavyAttackSectionElapsedTime += HeavyAttackTimelineDeltaTime;
	}

	TryProcessBufferedHeavyAttackRecoveryAction();
}

bool AJunPlayer::CanStartHeavyAttack() const
{
	if (!AnimInstance)
	{
		return false;
	}

	if ((GetCharacterMovement() && GetCharacterMovement()->IsFalling()) ||
		HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		HasGameplayTag(JunGameplayTags::State_Block_Attack) ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	if (bIsBasicAttacking || bIsHeavyAttacking)
	{
		return false;
	}

	return GetDefenseState() == EJunDefenseState::None;
}

void AJunPlayer::BeginHeavyAttackHoldInput()
{
	if (bHeavyAttackInputHeld)
	{
		return;
	}

	bHeavyAttackInputHeld = true;
	HeavyAttackInputHoldTime = FMath::Max(HeavyAttackInputHoldTime, 0.f);
	HeavyAttackChargeLoopElapsedTime = 0.f;
}

void AJunPlayer::ResetHeavyAttackChargeInput()
{
	bHeavyAttackInputHeld = false;
	bHeavyAttackChargeStartPlayed = false;
	bHeavyAttackChargeEndRequested = false;
	HeavyAttackInputHoldTime = 0.f;
	HeavyAttackChargeStartRemainTime = 0.f;
}

void AJunPlayer::StartHeavyAttackTap()
{
	if (!AnimInstance || !HeavyAttackTapMontage)
	{
		ResetHeavyAttackChargeInput();
		return;
	}

	if (!TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::HeavyAttack),
		[this]()
		{
			ProcessHeavyAttackTapStart();
		}))
	{
		ResetHeavyAttackChargeInput();
		return;
	}
}

void AJunPlayer::ProcessHeavyAttackTapStart()
{
	if (!AnimInstance || !HeavyAttackTapMontage)
	{
		ResetHeavyAttackChargeInput();
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsHeavyAttacking = true;
	HeavyAttackState = EJunHeavyAttackState::Tap;
	CurrentHeavyAttackMontage = HeavyAttackTapMontage;
	CurrentHeavyAttackPlayRate = FMath::Max(HeavyAttackTapPlayRate, KINDA_SMALL_NUMBER);
	HeavyAttackChargeLoopElapsedTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	PendingHeavyAttackRecoveryActionRequest.Reset();
	HeavyAttackComboIndex = 1;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->Montage_PlayWithBlendIn(
		HeavyAttackTapMontage,
		FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackStartBlendInTime)),
		CurrentHeavyAttackPlayRate);
	AnimInstance->Montage_JumpToSection(HeavyAttackTapFirstSectionName, HeavyAttackTapMontage);

	ResetHeavyAttackChargeInput();
}

void AJunPlayer::StartHeavyAttackCharge()
{
	if (bHeavyAttackChargeStartPlayed)
	{
		return;
	}

	if (!AnimInstance || !HeavyAttackChargeMontage)
	{
		StartHeavyAttackTap();
		return;
	}

	if (!TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::HeavyAttack),
		[this]()
		{
			ProcessHeavyAttackChargeStart();
		}))
	{
		ResetHeavyAttackChargeInput();
		return;
	}
}

void AJunPlayer::ProcessHeavyAttackChargeStart()
{
	if (!AnimInstance || !HeavyAttackChargeMontage)
	{
		ResetHeavyAttackChargeInput();
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsHeavyAttacking = true;
	bHeavyAttackChargeStartPlayed = true;
	bHeavyAttackChargeEndRequested = false;
	HeavyAttackState = EJunHeavyAttackState::ChargeStart;
	CurrentHeavyAttackMontage = HeavyAttackChargeMontage;
	CurrentHeavyAttackPlayRate = FMath::Max(HeavyAttackChargePlayRate, KINDA_SMALL_NUMBER);
	const int32 StartSectionIndex = HeavyAttackChargeMontage->GetSectionIndex(HeavyAttackChargeStartSectionName);
	HeavyAttackChargeStartRemainTime = StartSectionIndex != INDEX_NONE
		? HeavyAttackChargeMontage->GetSectionLength(StartSectionIndex)
		: HeavyAttackChargeMontage->GetPlayLength();
	HeavyAttackChargeLoopElapsedTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	bHeavyAttackChargeEndDashProcessed = false;
	PendingHeavyAttackRecoveryActionRequest.Reset();
	HeavyAttackComboIndex = 0;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->Montage_PlayWithBlendIn(
		HeavyAttackChargeMontage,
		FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackStartBlendInTime)),
		CurrentHeavyAttackPlayRate);
	AnimInstance->Montage_SetNextSection(
		HeavyAttackChargeLoopSectionName,
		HeavyAttackChargeLoopSectionName,
		HeavyAttackChargeMontage
	);
	AnimInstance->Montage_JumpToSection(HeavyAttackChargeStartSectionName, HeavyAttackChargeMontage);
}

void AJunPlayer::StartHeavyAttackChargeEnd()
{
	if (!AnimInstance || !HeavyAttackChargeMontage)
	{
		FinishHeavyAttack();
		return;
	}

	if (!TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::HeavyAttack),
		[this]()
		{
			ProcessHeavyAttackChargeEndStart();
		}))
	{
		FinishHeavyAttack();
		return;
	}
}

void AJunPlayer::ProcessHeavyAttackChargeEndStart()
{
	if (!AnimInstance || !HeavyAttackChargeMontage)
	{
		FinishHeavyAttack();
		return;
	}

	if (!bIsHeavyAttacking)
	{
		CancelLockOnTurn();
		AddGameplayTag(JunGameplayTags::State_Action_Attack);
		AddGameplayTag(JunGameplayTags::State_Block_Move);
		AddGameplayTag(JunGameplayTags::State_Block_Jump);
		AddGameplayTag(JunGameplayTags::State_Block_Attack);
		AddGameplayTag(JunGameplayTags::State_Block_Dodge);
		bIsHeavyAttacking = true;
	}

	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	HeavyAttackState = EJunHeavyAttackState::ChargeEnd;
	CurrentHeavyAttackMontage = HeavyAttackChargeMontage;
	CurrentHeavyAttackPlayRate = FMath::Max(HeavyAttackChargePlayRate, KINDA_SMALL_NUMBER);
	HeavyAttackChargeStartRemainTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	HeavyAttackComboIndex = 0;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	if (!AnimInstance->Montage_IsPlaying(HeavyAttackChargeMontage))
	{
		AnimInstance->Montage_PlayWithBlendIn(
			HeavyAttackChargeMontage,
			FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackStartBlendInTime)),
			CurrentHeavyAttackPlayRate);
	}
	AnimInstance->Montage_JumpToSection(HeavyAttackChargeEndSectionName, HeavyAttackChargeMontage);
	ProcessHeavyAttackChargeEndDash();
	ResetHeavyAttackChargeInput();
}

void AJunPlayer::ProcessHeavyAttackChargeEndDash()
{
	if (bHeavyAttackChargeEndDashProcessed)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	bHeavyAttackChargeEndDashProcessed = true;

	const float FullDashThreshold =
		HeavyAttackChargeLoopMaxDuration * FMath::Clamp(HeavyAttackChargeFullDashThresholdRatio, 0.f, 1.f);
	const bool bUseFullDash = HeavyAttackChargeLoopElapsedTime >= FullDashThreshold;
	const float DashSpeed = bUseFullDash
		? HeavyAttackChargeEndDashMaxSpeed
		: HeavyAttackChargeEndDashMaxSpeed * HeavyAttackChargeEndDashMinSpeedRatio;

	FVector DashDirection = GetActorForwardVector();
	DashDirection.Z = 0.f;
	DashDirection = DashDirection.GetSafeNormal();
	if (DashDirection.IsNearlyZero())
	{
		return;
	}

	const float CurrentVerticalSpeed = MovementComponent->Velocity.Z;
	MovementComponent->Velocity = FVector(
		DashDirection.X * DashSpeed,
		DashDirection.Y * DashSpeed,
		CurrentVerticalSpeed
	);

	KnockbackBrakingOverrideRemainTime = HeavyAttackChargeEndDashGroundMotionDuration;
	KnockbackBrakingDecelerationOverride = 0.f;
	KnockbackGroundFrictionOverride = 0.f;
	KnockbackBrakingFrictionFactorOverride = 0.f;
}

void AJunPlayer::FinishHeavyAttack()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsHeavyAttacking = false;
	HeavyAttackState = EJunHeavyAttackState::None;
	CurrentHeavyAttackMontage = nullptr;
	CurrentHeavyAttackPlayRate = 1.f;
	HeavyAttackChargeStartRemainTime = 0.f;
	HeavyAttackChargeLoopElapsedTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	bHeavyAttackChargeEndDashProcessed = false;
	PendingHeavyAttackRecoveryActionRequest.Reset();
	HeavyAttackComboIndex = 0;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	ResetHeavyAttackChargeInput();
	ClearActionStateIf(EJunPlayerActionState::HeavyAttack, EJunActionTransitionReason::System);
}

float AJunPlayer::GetCurrentHeavyAttackTimelineRate() const
{
	return FMath::Max(CurrentHeavyAttackPlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction Action) const
{
	const FJunPendingActionRequest Request = MakeRecoveryPendingActionRequest(
		EJunPendingActionRequestSource::HeavyAttackRecovery,
		Action);
	FJunPlayerCancelRule CancelRule;
	return TryGetRecoveryCancelRule(Request, CancelRule) &&
		IsPendingActionCancelRuleOpen(Request, CancelRule) &&
		CanRequestResolvedBufferedAction(CancelRule.ToAction);
}

void AJunPlayer::BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction Action)
{
	QueuePendingActionRequest(MakeRecoveryPendingActionRequest(
		EJunPendingActionRequestSource::HeavyAttackRecovery,
		Action));
}

void AJunPlayer::TryProcessBufferedHeavyAttackRecoveryAction()
{
	TryProcessPendingRecoveryActionRequest(PendingHeavyAttackRecoveryActionRequest);
}

void AJunPlayer::CancelHeavyAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsHeavyAttacking)
	{
		return;
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	if (AnimInstance && CurrentHeavyAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, CurrentHeavyAttackMontage);
	}

	FinishHeavyAttack();
}

bool AJunPlayer::CanCancelHeavyAttackIntoMove() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::None,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule);
}

bool AJunPlayer::TryCancelHeavyAttackIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	CancelHeavyAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelHeavyAttackIntoBasicAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::BasicAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelHeavyAttackIntoBasicAttack()
{
	if (!CanCancelHeavyAttackIntoBasicAttack())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::BasicAttack,
		CancelRule);
	CancelHeavyAttackForRecoveryTransition(CancelRule.BlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::CanCancelHeavyAttackIntoHeavyAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelHeavyAttackIntoHeavyAttack()
{
	if (!CanCancelHeavyAttackIntoHeavyAttack())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule);
	CancelHeavyAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelHeavyAttackIntoJigen() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::Jigen,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelHeavyAttackIntoJigen()
{
	if (!CanCancelHeavyAttackIntoJigen())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::HeavyAttack,
		EJunPlayerActionState::Jigen,
		CancelRule);
	CancelHeavyAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

void AJunPlayer::OnHeavyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentHeavyAttackMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	switch (HeavyAttackState)
	{
	case EJunHeavyAttackState::ChargeStart:
		// ChargeStart transition is driven by UpdateHeavyAttackInput so blend-out timing
		// cannot skip or duplicate the startup pose.
		break;
	case EJunHeavyAttackState::ChargeLoop:
		// Loop exits through input release or HeavyAttackChargeLoopMaxDuration.
		break;
	case EJunHeavyAttackState::Tap:
	case EJunHeavyAttackState::ChargeEnd:
	default:
		FinishHeavyAttack();
		break;
	}
}


void AJunPlayer::OnHeavyAttackStarted()
{
	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		BeginHeavyAttackHoldInput();
		BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::HeavyAttack);
		return;
	}

	if (CanBufferDefenseTransitionCancel())
	{
		BeginHeavyAttackHoldInput();
		BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::HeavyAttack);
		return;
	}

	if (bIsDodgeAttacking)
	{
		if (!TryCancelDodgeAttackIntoHeavyAttack())
		{
			return;
		}

		BeginHeavyAttackHoldInput();
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (!TryCancelDodgeIntoHeavyAttack())
		{
			return;
		}
	}

	if (bIsJumpAttacking)
	{
		if (!TryCancelJumpAttackEndIntoHeavyAttack())
		{
			return;
		}

		BeginHeavyAttackHoldInput();
		return;
	}

	if (bIsBasicAttacking)
	{
		if (!TryCancelBasicAttackIntoHeavyAttack())
		{
			return;
		}

		BeginHeavyAttackHoldInput();
		return;
	}

	if (bIsHeavyAttacking)
	{
		if (HeavyAttackState == EJunHeavyAttackState::Tap)
		{
			if (bHeavyAttackComboAdvanceStateActive)
			{
				bBufferedHeavyAttackComboInput = true;
				TryAdvanceHeavyAttackCombo();
				return;
			}

			if (bCanBufferHeavyAttackComboInput)
			{
				bBufferedHeavyAttackComboInput = true;
				return;
			}

			if (bCanRestartHeavyAttackAfterComboAdvance)
			{
				CancelHeavyAttackForRecoveryTransition(HeavyAttackRestartBlendOutTime);
				BeginHeavyAttackHoldInput();
				return;
			}

			BeginHeavyAttackHoldInput();
			bBufferedHeavyAttackInput = true;
			return;
		}

		if (TryCancelHeavyAttackIntoHeavyAttack())
		{
			BeginHeavyAttackHoldInput();
		}
		return;
	}

	if (bIsJigenAttacking)
	{
		if (!CanCancelJigenIntoHeavyAttack())
		{
			return;
		}

		FJunPlayerCancelRule CancelRule;
		TryGetDirectActionCancelRule(
			EJunPlayerActionState::Jigen,
			EJunPlayerActionState::HeavyAttack,
			CancelRule);
		CancelJigenForRecoveryTransition(CancelRule.BlendOutTime);
		bHeavyAttackInputHeld = true;
		HeavyAttackInputHoldTime = FMath::Max(HeavyAttackInputHoldTime, 0.f);
		HeavyAttackChargeLoopElapsedTime = 0.f;
		if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
		{
			StartHeavyAttackCharge();
		}
		return;
	}

	BeginHeavyAttackHoldInput();
}

void AJunPlayer::OnHeavyAttackReleased()
{
	bParrySuccessHeavyAttackInputHeld = false;

	if (!bHeavyAttackInputHeld && !bIsHeavyAttacking)
	{
		return;
	}

	bHeavyAttackInputHeld = false;

	if (bIsHeavyAttacking)
	{
		if (HeavyAttackState == EJunHeavyAttackState::ChargeStart)
		{
			bHeavyAttackChargeEndRequested = true;
		}
		else if (HeavyAttackState == EJunHeavyAttackState::ChargeLoop)
		{
			StartHeavyAttackChargeEnd();
		}
		return;
	}

	if (!CanStartHeavyAttack())
	{
		ResetHeavyAttackChargeInput();
		return;
	}

	if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
	{
		StartHeavyAttackChargeEnd();
		return;
	}

	StartHeavyAttackTap();
}


void AJunPlayer::TryAdvanceHeavyAttackCombo()
{
	if (!bIsHeavyAttacking ||
		!bBufferedHeavyAttackComboInput ||
		!HeavyAttackTapMontage ||
		HeavyAttackState != EJunHeavyAttackState::Tap)
	{
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (HeavyAttackComboIndex <= 1)
	{
		bBufferedHeavyAttackComboInput = false;
		bCanBufferHeavyAttackComboInput = false;
		bHeavyAttackComboAdvanceStateActive = false;
		bCanRestartHeavyAttackAfterComboAdvance = false;
		HeavyAttackComboIndex = 2;
		HeavyAttackSectionElapsedTime = 0.f;

		TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

		const int32 SecondSectionIndex = HeavyAttackTapMontage->GetSectionIndex(HeavyAttackTapSecondSectionName);
		if (SecondSectionIndex != INDEX_NONE)
		{
			float SectionStartTime = 0.f;
			float SectionEndTime = 0.f;
			HeavyAttackTapMontage->GetSectionStartAndEndTime(SecondSectionIndex, SectionStartTime, SectionEndTime);

			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
			const float MontagePlayResult = AnimInstance->Montage_PlayWithBlendIn(
				HeavyAttackTapMontage,
				FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackComboBlendInTime)),
				CurrentHeavyAttackPlayRate,
				EMontagePlayReturnType::MontageLength,
				SectionStartTime,
				true
			);
			AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);

			if (MontagePlayResult > 0.f)
			{
				return;
			}
		}

		AnimInstance->Montage_JumpToSection(HeavyAttackTapSecondSectionName, HeavyAttackTapMontage);
		return;
	}

	bBufferedHeavyAttackComboInput = false;
	bCanBufferHeavyAttackComboInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	CancelHeavyAttackForRecoveryTransition(HeavyAttackRestartBlendOutTime);
	StartHeavyAttackTap();
}


#pragma endregion