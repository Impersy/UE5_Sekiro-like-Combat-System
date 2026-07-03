#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

void AJunBoss::EnterApproachState()
{
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bRunLocomotionRequested = false;
}

void AJunBoss::EnterIdleState()
{
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bRunLocomotionRequested = false;
	bUseIdleHoldForYawMismatch = false;

	if (!HasQueuedReactiveAction())
	{
		RefreshPlannedCombatPlan();
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::Attack &&
		HasValidPlannedAttack() &&
		!IsTargetTooFarForPlannedAttack() &&
		!IsTargetTooCloseForPlannedAttack())
	{
		const float TargetYawAbsDelta = GetTargetYawAbsDelta();
		if (TargetYawAbsDelta > IdleEntryYawTolerance && TargetYawAbsDelta < MoveTurnStartAngle)
		{
			bUseIdleHoldForYawMismatch = FMath::FRand() > YawMismatchRepositionChance;
		}
	}
}

void AJunBoss::EnterHitState()
{
	StartPlayerPressure();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();
	ResetPlannedCombatPlan();
}

void AJunBoss::EnterAttackState()
{
	ResetReactiveEvadePressure();
	const FMonsterAttackSelection AttackSelection = ChooseNextAttackSelection();
	const FBossNormalAttackData* CurrentNormalAttackData =
		(CurrentCombatPlan.AttackType == EBossPlannedAttackType::NormalAttack)
			? GetPlannedNormalAttackData()
			: nullptr;
	const float CurrentNormalAttackPlayRate = CurrentNormalAttackData
		? FMath::Max(CurrentNormalAttackData->PlayRate, KINDA_SMALL_NUMBER)
		: 1.f;

	ResetAttackEntryRuntimeState();

	if (TryPrepareNormalAttackEntry(AttackSelection, CurrentNormalAttackData))
	{
		return;
	}

	PrepareComboAttackEntry(AttackSelection);
	QueueCodeMoveForAttackEntryIfNeeded(CurrentNormalAttackData, CurrentNormalAttackPlayRate);
	ApplyAttackEntrySuperArmorIfNeeded(AttackSelection, CurrentNormalAttackData);

	FBossAttackExecutionRequest ExecutionRequest;
	ExecutionRequest.Selection = AttackSelection;
	ExecutionRequest.RuntimeState = CurrentAttackRuntime;
	ExecutionRequest.InitiativeState = InitiativeState;
	if (!ProcessBossAttackExecution(ExecutionRequest))
	{
		FinishAttack();
		return;
	}
	ConfigureAttackEntryAfterMontageStart();
}

void AJunBoss::ResetAttackEntryRuntimeState()
{
	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();
}

bool AJunBoss::TryPrepareNormalAttackEntry(
	const FMonsterAttackSelection& AttackSelection,
	const FBossNormalAttackData* CurrentNormalAttackData)
{
	if (CurrentCombatPlan.AttackType != EBossPlannedAttackType::NormalAttack || AttackSelection.Montage == nullptr)
	{
		return false;
	}

	CurrentAttackRuntime.ActionType = EBossActionType::NormalAttack;
	CurrentAttackRuntime.NormalAttackType = CurrentCombatPlan.NormalAttackType;
	LastStartedNormalAttackType = CurrentAttackRuntime.NormalAttackType;
	InitiativeState = EBossInitiativeState::BossPattern;

	if (IsBowNormalAttackType(CurrentAttackRuntime.NormalAttackType))
	{
		BeginBowAttackPresentation();
		bPostBowCloseRangeApproachInProgress = bForceApproachAfterBowAttack;
		PostBowCloseRangeForceElapsedTime = 0.f;
	}
	else
	{
		bPostBowCloseRangeApproachInProgress = false;
	}

	if (CurrentNormalAttackData && TryStartVariantNormalAttack(CurrentAttackRuntime.NormalAttackType, *CurrentNormalAttackData))
	{
		return true;
	}

	return false;
}

void AJunBoss::PrepareComboAttackEntry(const FMonsterAttackSelection& AttackSelection)
{
	if (AttackSelection.Montage == GetPlannedComboMontage())
	{
		CurrentAttackRuntime.ActionType = EBossActionType::ComboAttack;
		CurrentAttackRuntime.ComboSet = CurrentCombatPlan.ComboSet;
		CurrentAttackRuntime.ComboLength = 1;
		CurrentCombatPlan.ComboLength = 1;
		InitiativeState = EBossInitiativeState::BossPattern;
		bPostBowCloseRangeApproachInProgress = false;
	}
}

void AJunBoss::QueueCodeMoveForAttackEntryIfNeeded(
	const FBossNormalAttackData* CurrentNormalAttackData,
	float CurrentNormalAttackPlayRate)
{
	const bool bUseNormalAttackAnimNotifyTiming =
		CurrentAttackRuntime.ActionType == EBossActionType::NormalAttack &&
		CurrentNormalAttackData &&
		CurrentNormalAttackData->bUseAnimNotifyTiming;

	if (!bUseNormalAttackAnimNotifyTiming &&
		CurrentAttackRuntime.ActionType == EBossActionType::NormalAttack &&
		CurrentNormalAttackData &&
		CurrentNormalAttackData->MoveSpeed > 0.f)
	{
		const float MoveStartTime = CurrentNormalAttackData->MoveStartTimeAtPlayRateOne;
		QueueCodeDrivenAttackMove(CurrentAttackRuntime.NormalAttackType, MoveStartTime / CurrentNormalAttackPlayRate);
	}
}

void AJunBoss::ApplyAttackEntrySuperArmorIfNeeded(
	const FMonsterAttackSelection& AttackSelection,
	const FBossNormalAttackData* CurrentNormalAttackData)
{
	const bool bUseNormalAttackAnimNotifyTiming =
		CurrentAttackRuntime.ActionType == EBossActionType::NormalAttack &&
		CurrentNormalAttackData &&
		CurrentNormalAttackData->bUseAnimNotifyTiming;
	const FBossComboAttackData* CurrentComboAttackData =
		(CurrentAttackRuntime.ActionType == EBossActionType::ComboAttack)
			? GetComboAttackData(CurrentAttackRuntime.ComboSet)
			: nullptr;
	const bool bUseComboAttackAnimNotifyTiming = CurrentComboAttackData && CurrentComboAttackData->bUseAnimNotifyTiming;
	const float AttackMontageDuration = AttackSelection.Montage
		? AttackSelection.Montage->GetPlayLength() / FMath::Max(AttackSelection.PlayRate, KINDA_SMALL_NUMBER)
		: 0.f;
	if (!bUseNormalAttackAnimNotifyTiming && !bUseComboAttackAnimNotifyTiming)
	{
		ApplyTimedMontageSuperArmor(AttackMontageDuration);
	}
}

void AJunBoss::ConfigureAttackEntryAfterMontageStart()
{
	if (CurrentAttackRuntime.ActionType == EBossActionType::ComboAttack)
	{
		ConfigurePlannedComboMontageSections();
	}
}

void AJunBoss::EnterRecoveryState()
{
	ResetReactiveEvadePressure();
	ClearTimedMontageSuperArmor();
	ResetActiveReactiveActionState();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	RestoreAttackGroundMotionOverride();

	if (!bAttackInterruptedByHitReact && CurrentAttackRuntime.ActionType != EBossActionType::None)
	{
		RecordAction(CurrentAttackRuntime.ActionType, CurrentAttackRuntime.ComboSet, CurrentAttackRuntime.ComboLength, CurrentAttackRuntime.NormalAttackType);
	}

	ResetPlannedCombatPlan();

	ResetCurrentAttackRuntimeState();
	bAttackInterruptedByHitReact = false;
}

void AJunBoss::EnterParrySuccessState()
{
	ResetReactiveEvadePressure();
	ClearTimedMontageSuperArmor();
	ResetActiveReactiveActionState();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bRunLocomotionRequested = false;
}

void AJunBoss::EnterRepositionState()
{
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::NonAttack &&
		CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe &&
		CurrentCombatPlan.MovementDirection == EBossMovementDirection::None)
	{
		if (CurrentTarget && GetTargetDistance2D() < StrafeMinDistanceToTarget)
		{
			CurrentCombatPlan.MovementDirection = EBossMovementDirection::Backward;
		}
		else
		{
			CurrentCombatPlan.MovementDirection = FMath::RandBool() ? EBossMovementDirection::Left : EBossMovementDirection::Right;
		}
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::NonAttack &&
		CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe &&
		CurrentCombatPlan.MovementDuration <= 0.f)
	{
		CurrentCombatPlan.MovementDuration = StrafeMinDuration;
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::NonAttack &&
		CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe)
	{
		switch (CurrentCombatPlan.MovementDirection)
		{
		case EBossMovementDirection::Left:
			CurrentRepositionDirection = -1.f;
			break;
		case EBossMovementDirection::Backward:
			CurrentRepositionDirection = 0.f;
			break;
		case EBossMovementDirection::Right:
		default:
			CurrentRepositionDirection = 1.f;
			break;
		}
		return;
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::Attack &&
		CurrentTarget &&
		!IsTargetTooCloseForPlannedAttack())
	{
		const FVector ToTarget = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float SideSign = FVector::DotProduct(GetActorRightVector(), ToTarget);
		CurrentRepositionDirection = SideSign >= 0.f ? 1.f : -1.f;
		return;
	}

	CurrentRepositionDirection = 1.f;
}

void AJunBoss::EnterNonAttackActionState()
{
	ResetReactiveEvadePressure();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bRunLocomotionRequested = false;

	if (UAnimMontage* NonAttackMontage = GetPlannedNonAttackMontage())
	{
		PlayAnimMontage(NonAttackMontage);
		CurrentCombatPlan.MovementDuration = NonAttackMontage->GetPlayLength();
	}
	else if (CurrentCombatPlan.MovementDuration <= 0.f)
	{
		CurrentCombatPlan.MovementDuration = NoAttackHoldDuration;
	}
}

void AJunBoss::EnterEvadeState()
{
	ResetReactiveEvadePressure();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;
	ClearTimedMontageSuperArmor();
	const bool bUsingReactiveEvade = ReactiveActionState.ActiveAction.Type != EBossReactiveActionType::None;

	if (UAnimMontage* NonAttackMontage = bUsingReactiveEvade ? GetReactiveActionMontage() : GetPlannedNonAttackMontage())
	{
		float PlayedMontageLength = 0.f;
		if (bUsingReactiveEvade && ReactiveEvadeBlendInTime > 0.f)
		{
			if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				const FMontageBlendSettings BlendInSettings(ReactiveEvadeBlendInTime);
				PlayedMontageLength = AnimInstance->Montage_PlayWithBlendSettings(NonAttackMontage, BlendInSettings);
			}
			else
			{
				PlayedMontageLength = PlayAnimMontage(NonAttackMontage);
			}
		}
		else
		{
			PlayedMontageLength = PlayAnimMontage(NonAttackMontage);
		}

		if (PlayedMontageLength > 0.f)
		{
			ApplyTimedMontageSuperArmor(PlayedMontageLength, 1.f);
			if (bUsingReactiveEvade)
			{
				ReactiveActionState.ActiveAction.Duration = PlayedMontageLength;
			}
			else
			{
				CurrentCombatPlan.MovementDuration = PlayedMontageLength;
			}
		}
		else
		{
			ApplyTimedMontageSuperArmor(NonAttackMontage->GetPlayLength(), 1.f);
			if (bUsingReactiveEvade)
			{
				ReactiveActionState.ActiveAction.Duration = NonAttackMontage->GetPlayLength();
			}
			else
			{
				CurrentCombatPlan.MovementDuration = NonAttackMontage->GetPlayLength();
			}
		}
	}
	else if (bUsingReactiveEvade)
	{
		if (ReactiveActionState.ActiveAction.Duration <= 0.f)
		{
			ReactiveActionState.ActiveAction.Duration = EvadeFallbackDuration;
		}
	}
	else if (CurrentCombatPlan.MovementDuration <= 0.f)
	{
		CurrentCombatPlan.MovementDuration = EvadeFallbackDuration;
	}
}

void AJunBoss::UpdateApproachState(float DeltaTime)
{
	EnsureCombatPlan();

	if (IsPlayerPressureActive())
	{
		StopAIMovement();
		SetDesiredMoveAxes(0.f, 0.f);
		bRunLocomotionRequested = false;
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, ApproachFacingInterpSpeed);
		return;
	}

	if (bNoAttackCandidateApproachInProgress)
	{
		const bool bPostBowStillNeedsApproach =
			ShouldForceCloseAfterBowAttack() && !HasAnyNonBowAttackCandidateForCurrentDistance();
		const bool bFartherThanAllCandidates = IsFartherThanAllAttackCandidates();

		if (HasAnyAttackCandidateForCurrentDistance() && !bPostBowStillNeedsApproach && !bFartherThanAllCandidates)
		{
			ClearNoAttackCandidateApproach();
			bUseNonAttackFallbackUntilAttackCandidateAppears = false;
			bShouldStartNoAttackFallbackWithStrafe = true;
			ResetPlannedCombatPlan();
			TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlanReady);
			return;
		}

		if (TryStartOrWaitForBossTurn(MoveTurnStartAngle))
		{
			return;
		}

		NoAttackCandidateApproachRemainTime = FMath::Max(0.f, NoAttackCandidateApproachRemainTime - DeltaTime);
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, ApproachFacingInterpSpeed);
		bRunLocomotionRequested = false;

		if (CurrentTarget)
		{
			MoveToTarget(CurrentTarget);
		}

		if (NoAttackCandidateApproachRemainTime <= 0.f)
		{
			if (bPostBowStillNeedsApproach || bFartherThanAllCandidates)
			{
				return;
			}

			ClearNoAttackCandidateApproach();
			bUseNonAttackFallbackUntilAttackCandidateAppears = true;
			ResetPlannedCombatPlan();
			TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlanInvalid);
		}

		return;
	}

	if (CurrentCombatPlan.ActionType != EBossPlannedActionType::Attack || !HasValidPlannedAttack())
	{
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlanInvalid);
		return;
	}

	if (TryStartOrWaitForBossTurn(MoveTurnStartAngle))
	{
		return;
	}

	UpdateFacingTowardsTargetWithoutTurn(DeltaTime, ApproachFacingInterpSpeed);

	if (IsTargetTooFarForPlannedAttack())
	{
		bRunLocomotionRequested = false;
	}
	else
	{
		bRunLocomotionRequested = false;
	}

	if (IsTargetTooCloseForPlannedAttack())
	{
		TransitionBossCombatState(EBossCombatState::Reposition, EBossCombatStateChangeReason::TargetRangeChanged);
		return;
	}

	if (!IsTargetTooFarForPlannedAttack())
	{
		const float TargetYawAbsDelta = GetTargetYawAbsDelta();

		if (TargetYawAbsDelta <= IdleEntryYawTolerance)
		{
			StopAIMovement();
			if (CanAttackTarget())
			{
				TransitionBossCombatState(EBossCombatState::Attack, EBossCombatStateChangeReason::AttackStarted);
			}
			else
			{
				TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlanInvalid);
			}
			return;
		}

		TransitionBossCombatState(EBossCombatState::Reposition, EBossCombatStateChangeReason::TargetRangeChanged);
		return;
	}

	if (CurrentTarget)
	{
		MoveToTarget(CurrentTarget);
	}
}

void AJunBoss::UpdateIdleState(float DeltaTime)
{
	if (ParryExchangeState.SuccessExitActionLockRemainTime > 0.f)
	{
		StopAIMovement();
		SetDesiredMoveAxes(0.f, 0.f);
		bRunLocomotionRequested = false;
		return;
	}

	if (IsPlayerPressureActive())
	{
		StopAIMovement();
		SetDesiredMoveAxes(0.f, 0.f);
		bRunLocomotionRequested = false;
		TryStartOrWaitForBossTurn(TurnStartAngle);
		return;
	}

	if (TryConsumeReactiveBossActionFromIdle())
	{
		return;
	}

	EvaluateNextBossActionFromIdle();
}

void AJunBoss::UpdateAttackState(float DeltaTime)
{
	if (!bIsAttacking)
	{
		if (InitiativeState == EBossInitiativeState::PlayerPressure)
		{
			PlayerPressureRemainTime = FMath::Max(PlayerPressureRemainTime, PlayerPressureHoldDuration);
		}

		if (InitiativeState == EBossInitiativeState::BossPattern ||
			InitiativeState == EBossInitiativeState::BossParryCounter)
		{
			InitiativeState = EBossInitiativeState::Neutral;
		}

		ResetPendingAttackMotionState();
		RestoreAttackGroundMotionOverride();
		TransitionBossCombatState(EBossCombatState::Recovery, EBossCombatStateChangeReason::AttackEnded);
	}
}

void AJunBoss::UpdateHitState(float DeltaTime)
{
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (!IsInHitReact())
	{
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::HitReactEnded);
	}
}

bool AJunBoss::IsPlayerPressureActive() const
{
	return InitiativeState == EBossInitiativeState::PlayerPressure && PlayerPressureRemainTime > 0.f;
}

void AJunBoss::StartPlayerPressure()
{
	InitiativeState = EBossInitiativeState::PlayerPressure;
	PlayerPressureRemainTime = FMath::Max(PlayerPressureRemainTime, PlayerPressureHoldDuration);
}

void AJunBoss::ClearPlayerPressure()
{
	InitiativeState = EBossInitiativeState::Neutral;
	PlayerPressureRemainTime = 0.f;
}

void AJunBoss::UpdatePlayerPressure(float DeltaTime)
{
	if (InitiativeState != EBossInitiativeState::PlayerPressure || PlayerPressureRemainTime <= 0.f)
	{
		return;
	}

	if (bIsAttacking ||
		IsInHitReact() ||
		CurrentCombatSubState == EBossCombatState::ParrySuccess)
	{
		return;
	}

	PlayerPressureRemainTime = FMath::Max(0.f, PlayerPressureRemainTime - DeltaTime);
	if (PlayerPressureRemainTime <= 0.f)
	{
		InitiativeState = EBossInitiativeState::Neutral;
	}
}

void AJunBoss::UpdateParrySuccessState(float DeltaTime)
{
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (CombatSubStateElapsedTime <= ParrySuccessFacingDuration)
	{
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, ParrySuccessFacingInterpSpeed);
	}

	if (ParryExchangeState.SuccessFallbackRemainTime > 0.f)
	{
		ParryExchangeState.SuccessFallbackRemainTime = FMath::Max(0.f, ParryExchangeState.SuccessFallbackRemainTime - DeltaTime);
		if (ParryExchangeState.SuccessFallbackRemainTime <= 0.f)
		{
			FinishParrySuccessState();
		}
	}
}

void AJunBoss::UpdateRecoveryState(float DeltaTime)
{
	if (CombatSubStateElapsedTime < RecoveryDuration)
	{
		return;
	}

	TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::RecoveryEnded);
}

void AJunBoss::UpdateRepositionState(float DeltaTime)
{
	EnsureCombatPlan();
	const float ResolvedStrafeAnimInputStrength = FMath::Clamp(StrafeAnimInputStrength, 0.f, 0.4f);
	const float ResolvedStrafeMoveInputStrength = StrafeMoveInputStrength * StrafeMoveSpeedScale;

	if (IsPlayerPressureActive())
	{
		StopAIMovement();
		SetDesiredMoveAxes(0.f, 0.f);
		bRunLocomotionRequested = false;
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, CombatFacingInterpSpeed);
		return;
	}

	if (TryStartOrWaitForBossTurn(MoveTurnStartAngle))
	{
		return;
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::Attack && IsTargetTooFarForPlannedAttack())
	{
		TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::TargetRangeChanged);
		return;
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::NonAttack &&
		CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe)
	{
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, CombatFacingInterpSpeed);
		if (CurrentTarget && GetTargetDistance2D() < StrafeMinDistanceToTarget)
		{
			if (DashBackwardMontage && CanTriggerCloseRangeBackDash() && CanStartMobilityMontageSafely())
			{
				StartCloseRangeBackDashCooldown();
				CurrentCombatPlan.NonAttackType = EBossPlannedNonAttackType::Dash;
				CurrentCombatPlan.MovementDirection = EBossMovementDirection::Backward;
				TransitionBossCombatState(EBossCombatState::Evade, EBossCombatStateChangeReason::MobilityStarted);
				return;
			}

			CurrentCombatPlan.MovementDirection = EBossMovementDirection::Backward;
		}

		if (CurrentCombatPlan.MovementDirection == EBossMovementDirection::Backward)
		{
			AddMovementInput(-GetActorForwardVector(), BackStrafeMoveInputStrength);
			SetDesiredMoveAxes(-StrafeAnimInputStrength, 0.f);
		}
		else
		{
			const FVector RightDirection = GetActorRightVector();
			AddMovementInput(RightDirection, CurrentRepositionDirection * ResolvedStrafeMoveInputStrength);
			SetDesiredMoveAxes(0.f, CurrentRepositionDirection * ResolvedStrafeAnimInputStrength);
		}

		if (CombatSubStateElapsedTime >= CurrentCombatPlan.MovementDuration)
		{
			CombatMoveInput = FVector2D::ZeroVector;
			SetDesiredMoveAxes(0.f, 0.f);
			LastCompletedNonAttackType = EBossPlannedNonAttackType::Strafe;
			LastCompletedStrafeDirection = CurrentCombatPlan.MovementDirection;
			ResetPlannedCombatPlan();
			TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::MobilityEnded);
		}

		return;
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::Attack && IsTargetTooCloseForPlannedAttack())
	{
		ResetPlannedCombatPlan();
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::TargetRangeChanged);
		return;
	}

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::Attack)
	{
		const float TargetYawDelta = GetTargetYawAbsDelta();
		if (TargetYawDelta > IdleEntryYawTolerance)
		{
			UpdateFacingTowardsTargetWithoutTurn(DeltaTime, CombatFacingInterpSpeed);
			const FVector RightDirection = GetActorRightVector();
			AddMovementInput(RightDirection, CurrentRepositionDirection * ResolvedStrafeMoveInputStrength);
			SetDesiredMoveAxes(0.f, CurrentRepositionDirection * ResolvedStrafeAnimInputStrength);
			return;
		}

		if (CanAttackTarget())
		{
			TransitionBossCombatState(EBossCombatState::Attack, EBossCombatStateChangeReason::AttackStarted);
			return;
		}

		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlanInvalid);
		return;
	}

	SetDesiredMoveAxes(0.f, 0.f);
	TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlanInvalid);
}

void AJunBoss::UpdateNonAttackActionState(float DeltaTime)
{
	if (CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Hold)
	{
		BeginNoAttackCandidateApproach();
		TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::PlanInvalid);
		return;
	}

	StopAIMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (CombatSubStateElapsedTime < CurrentCombatPlan.MovementDuration)
	{
		return;
	}

	LastCompletedNonAttackType = CurrentCombatPlan.NonAttackType;
	ResetPlannedCombatPlan();
	TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::MobilityEnded);
}

void AJunBoss::UpdateEvadeState(float DeltaTime)
{
	const bool bUsingReactiveEvade = ReactiveActionState.ActiveAction.Type != EBossReactiveActionType::None;
	const EBossPlannedNonAttackType ResolvedEvadeType =
		bUsingReactiveEvade ? ReactiveActionState.ActiveAction.NonAttackType : CurrentCombatPlan.NonAttackType;
	const float ResolvedEvadeDuration =
		bUsingReactiveEvade ? ReactiveActionState.ActiveAction.Duration : CurrentCombatPlan.MovementDuration;

	if (CurrentTarget)
	{
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, EvadeFacingInterpSpeed);
	}

	if (CombatSubStateElapsedTime < ResolvedEvadeDuration)
	{
		return;
	}

	switch (ResolvedEvadeType)
	{
	case EBossPlannedNonAttackType::Dash:
		RecordAction(EBossActionType::Dash);
		break;
	case EBossPlannedNonAttackType::Dodge:
		RecordAction(EBossActionType::Dodge);
		break;
	default:
		break;
	}

	if (bUsingReactiveEvade)
	{
		ResetActiveReactiveActionState();
	}

	ResetPlannedCombatPlan();

	TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::MobilityEnded);
}
