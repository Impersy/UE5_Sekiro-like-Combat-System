#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"

FMonsterAttackSelection AJunBoss::ChooseNextAttackSelection() const
{
	FMonsterAttackSelection Selection = Super::ChooseNextAttackSelection();

	if (CurrentCombatPlan.AttackType == EBossPlannedAttackType::NormalAttack)
	{
		if (const FBossNormalAttackData* NormalAttackData = GetPlannedNormalAttackData())
		{
			if (NormalAttackData->Montage)
			{
				Selection.Montage = NormalAttackData->Montage;
				Selection.AttackRange = NormalAttackData->MaxRange;
				Selection.bFaceTargetDuringAttack = NormalAttackData->bFaceTargetDuringAttack;
				Selection.FacingDuration = NormalAttackData->bUseAnimNotifyTiming ? 0.f : NormalAttackData->FacingDuration;
				Selection.FacingInterpSpeed = NormalAttackData->FacingInterpSpeed;
				Selection.PlayRate = NormalAttackData->PlayRate;
				Selection.bTryTurnAfterAttack = NormalAttackData->bTryTurnAfterAttack;
				Selection.PostAttackTurnStartAngle = NormalAttackData->PostAttackTurnStartAngle;
				return Selection;
			}
		}
	}

	if (CurrentCombatPlan.AttackType == EBossPlannedAttackType::ComboAttack)
	{
		if (UAnimMontage* PlannedComboMontage = GetPlannedComboMontage())
		{
			const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentCombatPlan.ComboSet);
			Selection.Montage = PlannedComboMontage;
			Selection.AttackRange = ComboAttackData ? ComboAttackData->AttackRange : 250.f;
			Selection.bFaceTargetDuringAttack = true;
			Selection.FacingDuration = ComboAttackData && ComboAttackData->bUseAnimNotifyTiming
				? 0.f
				: GetPlannedComboFacingDuration();
			Selection.FacingInterpSpeed = AttackFacingInterpSpeed;
			Selection.PlayRate = ComboAttackData ? ComboAttackData->PlayRate : 1.f;
			Selection.bTryTurnAfterAttack = true;
			Selection.PostAttackTurnStartAngle = TurnStartAngle;
			return Selection;
		}
	}

	return Selection;
}

bool AJunBoss::HasQueuedReactiveAction() const
{
	return ReactiveActionState.QueuedAction != EBossReactiveActionType::None;
}

bool AJunBoss::TryConsumeReactiveBossActionFromIdle()
{
	if (!HasQueuedReactiveAction())
	{
		return false;
	}

	if (!CanStartMobilityMontageSafely())
	{
		SetDesiredMoveAxes(0.f, 0.f);
		return true;
	}

	return TryConsumeReactiveAction();
}

bool AJunBoss::EvaluateNextBossActionFromIdle()
{
	EnsureCombatPlan();
	return TryExecutePlannedBossActionFromIdle();
}

bool AJunBoss::TryExecutePlannedBossActionFromIdle()
{
	if (bNoAttackCandidateApproachInProgress)
	{
		TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::TargetRangeChanged);
		return true;
	}

	switch (CurrentCombatPlan.ActionType)
	{
	case EBossPlannedActionType::Attack:
		return TryExecutePlannedAttackFromIdle();
	case EBossPlannedActionType::NonAttack:
		return TryExecutePlannedMovementFromIdle();
	case EBossPlannedActionType::None:
	default:
		return false;
	}
}

bool AJunBoss::TryExecutePlannedAttackFromIdle()
{
	if (bUseIdleHoldForYawMismatch && CombatSubStateElapsedTime < YawMismatchIdleHoldDuration)
	{
		SetDesiredMoveAxes(0.f, 0.f);
		return true;
	}

	if (!HasValidPlannedAttack())
	{
		ResetPlannedCombatPlan();
		return true;
	}

	const float TargetYawAbsDelta = GetTargetYawAbsDelta();
	if (TargetYawAbsDelta > IdleEntryYawTolerance)
	{
		if (CanStartBossTurn())
		{
			TransitionBossCombatState(EBossCombatState::Turn, EBossCombatStateChangeReason::TurnStarted);
			return true;
		}

		if (!IsTargetTooFarForPlannedAttack() && !IsTargetTooCloseForPlannedAttack())
		{
			TransitionBossCombatState(EBossCombatState::Reposition, EBossCombatStateChangeReason::TargetRangeChanged);
			return true;
		}

		TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::TargetRangeChanged);
		return true;
	}

	if (IsTargetTooFarForPlannedAttack())
	{
		TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::TargetRangeChanged);
		return true;
	}

	if (IsTargetTooCloseForPlannedAttack())
	{
		TransitionBossCombatState(EBossCombatState::Reposition, EBossCombatStateChangeReason::TargetRangeChanged);
		return true;
	}

	if (CanAttackTarget())
	{
		TransitionBossCombatState(EBossCombatState::Attack, EBossCombatStateChangeReason::AttackStarted);
		return true;
	}

	return true;
}

bool AJunBoss::TryExecutePlannedMovementFromIdle()
{
	if (!HasValidPlannedMovement())
	{
		ResetPlannedCombatPlan();
		return true;
	}

	if (CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Hold)
	{
		BeginNoAttackCandidateApproach();
		TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::TargetRangeChanged);
		return true;
	}

	if (IsMobilityNonAttackType(CurrentCombatPlan.NonAttackType) && !CanStartMobilityMontageSafely())
	{
		SetDesiredMoveAxes(0.f, 0.f);
		return true;
	}

	if (GetTargetYawAbsDelta() >= MoveTurnStartAngle && CanStartBossTurn())
	{
		TransitionBossCombatState(EBossCombatState::Turn, EBossCombatStateChangeReason::TurnStarted);
		return true;
	}

	if (CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe)
	{
		if (CurrentTarget && GetTargetDistance2D() < StrafeMinDistanceToTarget)
		{
			if (DashBackwardMontage && CanTriggerCloseRangeBackDash() && CanStartMobilityMontageSafely())
			{
				StartCloseRangeBackDashCooldown();
				CurrentCombatPlan.NonAttackType = EBossPlannedNonAttackType::Dash;
				CurrentCombatPlan.MovementDirection = EBossMovementDirection::Backward;
				TransitionBossCombatState(EBossCombatState::Evade, EBossCombatStateChangeReason::MobilityStarted);
				return true;
			}

			CurrentCombatPlan.MovementDirection = EBossMovementDirection::Backward;
		}

		TransitionBossCombatState(EBossCombatState::Reposition, EBossCombatStateChangeReason::TargetRangeChanged);
		return true;
	}

	TransitionBossCombatState(IsMobilityNonAttackType(CurrentCombatPlan.NonAttackType)
		? EBossCombatState::Evade
		: EBossCombatState::NonAttackAction,
		EBossCombatStateChangeReason::MobilityStarted);
	return true;
}

bool AJunBoss::CanStartMobilityMontageSafely() const
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	return !AnimInstance || AnimInstance->GetCurrentActiveMontage() == nullptr;
}

void AJunBoss::ResetPlannedCombatPlan()
{
	ClearDelayedNormalAttackRangeLink();
	CurrentCombatPlan.Reset();
}

void AJunBoss::BeginNoAttackCandidateApproach()
{
	if (bNoAttackCandidateApproachInProgress)
	{
		return;
	}

	bNoAttackCandidateApproachInProgress = true;
	NoAttackCandidateApproachRemainTime = NoAttackCandidateApproachDelay;
}

void AJunBoss::ClearNoAttackCandidateApproach()
{
	bNoAttackCandidateApproachInProgress = false;
	NoAttackCandidateApproachRemainTime = 0.f;
}

bool AJunBoss::TryQueueReactiveAction(EBossReactiveActionType ReactiveActionType)
{
	if (ReactiveActionType == EBossReactiveActionType::None)
	{
		return false;
	}

	ReactiveActionState.QueuedAction = ReactiveActionType;
	return true;
}

bool AJunBoss::TryReactToHitDirection(ECharacterHitReactDirection HitDirection)
{
	return ApplyReactiveActionDecision(EvaluateReactiveActionDecision(HitDirection));
}

FBossReactiveActionDecision AJunBoss::EvaluateReactiveActionDecision(
	ECharacterHitReactDirection HitDirection) const
{
	FBossReactiveActionDecision Decision;

	switch (HitDirection)
	{
	case ECharacterHitReactDirection::Left_L:
	case ECharacterHitReactDirection::Right_R:
	case ECharacterHitReactDirection::Back_B:
		Decision.Result = EBossReactiveActionDecisionResult::ResetPressure;
		return Decision;
	case ECharacterHitReactDirection::Front_F:
	case ECharacterHitReactDirection::Front_FL:
	case ECharacterHitReactDirection::Front_FR:
		break;
	default:
		return Decision;
	}

	const FBossReactiveActionTuningData* ReactiveEvadeTuningData =
		GetReactiveActionTuningData(EBossReactiveActionType::EvadeBackward);

	if (!CurrentTarget || !ReactiveEvadeTuningData)
	{
		Decision.Result = EBossReactiveActionDecisionResult::ResetPressure;
		return Decision;
	}

	if (GetTargetDistance2D() > ReactiveEvadeTuningData->TriggerRange)
	{
		Decision.Result = EBossReactiveActionDecisionResult::ResetPressure;
		return Decision;
	}

	Decision.CloseHitCount = ReactiveActionState.BackwardEvadeCloseHitCount + 1;
	const int32 AdditionalCloseHitCount = FMath::Max(0, Decision.CloseHitCount - 1);
	const float RawTriggerChance =
		ReactiveEvadeTuningData->BaseTriggerChance +
		(AdditionalCloseHitCount * ReactiveEvadeTuningData->TriggerChancePerHitCount) +
		ReactiveEvadeTuningData->TriggerChanceBonus;

	Decision.TriggerChance = FMath::Clamp(
		RawTriggerChance,
		0.f,
		ReactiveEvadeTuningData->MaxTriggerChance
	);
	Decision.Result = EBossReactiveActionDecisionResult::AccumulatePressure;

	if (FMath::FRand() > Decision.TriggerChance || (!DashBackwardMontage && !DodgeBackwardMontage))
	{
		return Decision;
	}

	Decision.Result = EBossReactiveActionDecisionResult::QueueAction;
	Decision.ActionType = EBossReactiveActionType::EvadeBackward;
	return Decision;
}

bool AJunBoss::ApplyReactiveActionDecision(const FBossReactiveActionDecision& Decision)
{
	switch (Decision.Result)
	{
	case EBossReactiveActionDecisionResult::ResetPressure:
		ResetReactiveEvadePressure();
		return false;
	case EBossReactiveActionDecisionResult::AccumulatePressure:
		ReactiveActionState.BackwardEvadeCloseHitCount = Decision.CloseHitCount;
		ReactiveActionState.BackwardEvadeTriggerChance = Decision.TriggerChance;
		return false;
	case EBossReactiveActionDecisionResult::QueueAction:
		ReactiveActionState.BackwardEvadeCloseHitCount = Decision.CloseHitCount;
		ReactiveActionState.BackwardEvadeTriggerChance = Decision.TriggerChance;
		if (!TryQueueReactiveAction(Decision.ActionType))
		{
			return false;
		}
		ResetReactiveEvadePressure();
		return true;
	case EBossReactiveActionDecisionResult::None:
	default:
		return false;
	}
}

bool AJunBoss::EvaluateReactiveActionExecution(
	EBossReactiveActionType ReactiveActionType,
	FBossReactiveActionExecutionData& OutExecutionData) const
{
	OutExecutionData = FBossReactiveActionExecutionData{};

	switch (ReactiveActionType)
	{
	case EBossReactiveActionType::EvadeBackward:
		return EvaluateReactiveBackwardEvade(OutExecutionData);
	case EBossReactiveActionType::None:
	default:
		return false;
	}
}

void AJunBoss::ApplyReactiveActionExecution(const FBossReactiveActionExecutionData& ExecutionData)
{
	ReactiveActionState.ActiveAction = ExecutionData;
}

bool AJunBoss::TryConsumeReactiveAction()
{
	if (!HasQueuedReactiveAction())
	{
		return false;
	}

	const EBossReactiveActionType ReactiveActionToConsume = ReactiveActionState.QueuedAction;
	ResetReactiveActionState();
	ResetPlannedCombatPlan();

	FBossReactiveActionExecutionData ExecutionData;
	if (!EvaluateReactiveActionExecution(ReactiveActionToConsume, ExecutionData))
	{
		return false;
	}
	ApplyReactiveActionExecution(ExecutionData);

	TransitionBossCombatState(GetReactiveActionTargetState(ReactiveActionToConsume), EBossCombatStateChangeReason::ReactiveAction);
	return true;
}

void AJunBoss::EnsureCombatPlan()
{
	if (bNoAttackCandidateApproachInProgress)
	{
		return;
	}

	if (!IsCurrentCombatPlanValid())
	{
		RefreshPlannedCombatPlan();
	}
}

bool AJunBoss::IsCurrentCombatPlanValid() const
{
	switch (CurrentCombatPlan.ActionType)
	{
	case EBossPlannedActionType::Attack:
		return HasValidPlannedAttack();
	case EBossPlannedActionType::NonAttack:
		return HasValidPlannedMovement();
	case EBossPlannedActionType::None:
	default:
		return false;
	}
}

void AJunBoss::RefreshPlannedAttackType()
{
	if (!CurrentTarget)
	{
		ResetPlannedAttackChoice();
		return;
	}

	const float TargetDistance = GetTargetDistance2D();
	TArray<FBossWeightedNormalAttackCandidate> NormalAttackCandidates;
	BuildNormalAttackCandidatesForPlan(TargetDistance, NormalAttackCandidates);

	TArray<EBossPlannedAttackType> AttackCandidates;
	CollectPlannedAttackTypeCandidates(NormalAttackCandidates, AttackCandidates);
	if (AttackCandidates.Num() <= 0)
	{
		ResetPlannedCombatPlan();
		return;
	}

	if (!ApplyPlannedAttackTypeSelection(AttackCandidates, NormalAttackCandidates))
	{
		ResetPlannedCombatPlan();
	}
}

void AJunBoss::ResetPlannedAttackChoice()
{
	CurrentCombatPlan.ResetAttackChoice();
}

bool AJunBoss::BuildNormalAttackCandidatesForPlan(float TargetDistance, TArray<FBossWeightedNormalAttackCandidate>& OutNormalAttackCandidates) const
{
	OutNormalAttackCandidates.Reset();
	CollectNormalAttackCandidates(TargetDistance, FBossNormalAttackCandidatePolicy{}, OutNormalAttackCandidates);

	if (OutNormalAttackCandidates.Num() > 0)
	{
		return true;
	}

	FBossNormalAttackCandidatePolicy RepeatFallbackPolicy;
	RepeatFallbackPolicy.bIgnoreRepeat = true;
	CollectNormalAttackCandidates(TargetDistance, RepeatFallbackPolicy, OutNormalAttackCandidates);
	return OutNormalAttackCandidates.Num() > 0;
}

void AJunBoss::CollectPlannedAttackTypeCandidates(
	const TArray<FBossWeightedNormalAttackCandidate>& NormalAttackCandidates,
	TArray<EBossPlannedAttackType>& OutAttackCandidates) const
{
	OutAttackCandidates.Reset();

	auto AddWeightedAttackCandidate = [&OutAttackCandidates](EBossPlannedAttackType AttackType, int32 Weight)
	{
		const int32 ClampedWeight = FMath::Max(1, Weight);
		for (int32 Index = 0; Index < ClampedWeight; ++Index)
		{
			OutAttackCandidates.Add(AttackType);
		}
	};

	if (NormalAttackCandidates.Num() > 0)
	{
		int32 TotalNormalAttackWeight = 0;
		for (const FBossWeightedNormalAttackCandidate& Candidate : NormalAttackCandidates)
		{
			TotalNormalAttackWeight += FMath::Max(1, Candidate.Weight);
		}
		AddWeightedAttackCandidate(EBossPlannedAttackType::NormalAttack, TotalNormalAttackWeight);
	}
}

bool AJunBoss::ApplyPlannedAttackTypeSelection(
	const TArray<EBossPlannedAttackType>& AttackCandidates,
	const TArray<FBossWeightedNormalAttackCandidate>& NormalAttackCandidates)
{
	if (AttackCandidates.Num() <= 0)
	{
		ResetPlannedAttackChoice();
		return false;
	}

	CurrentCombatPlan.AttackType = AttackCandidates[FMath::RandRange(0, AttackCandidates.Num() - 1)];
	CurrentCombatPlan.NormalAttackType = EBossNormalAttackType::None;
	CurrentCombatPlan.ComboSet = EBossComboSet::None;
	CurrentCombatPlan.ComboLength = 0;

	if (CurrentCombatPlan.AttackType == EBossPlannedAttackType::NormalAttack)
	{
		if (NormalAttackCandidates.Num() <= 0)
		{
			CurrentCombatPlan.AttackType = EBossPlannedAttackType::None;
			return false;
		}

		CurrentCombatPlan.NormalAttackType = SelectNormalAttackCandidate(NormalAttackCandidates);
		return CurrentCombatPlan.NormalAttackType != EBossNormalAttackType::None;
	}

	return CurrentCombatPlan.AttackType == EBossPlannedAttackType::ComboAttack;
}

void AJunBoss::RefreshPlannedCombatPlan()
{
	ResetPlannedCombatPlan();

	const bool bShouldCloseAfterBow = ShouldForceCloseAfterBowAttack();

	if (TryPlanForcedApproachAfterBowAttack(bShouldCloseAfterBow))
	{
		return;
	}

	if (TryPlanApproachUntilAttackCandidate())
	{
		return;
	}

	if (TryPlanNoAttackFallback())
	{
		return;
	}

	ClearNoAttackCandidateApproach();
	bUseNonAttackFallbackUntilAttackCandidateAppears = false;
	bShouldStartNoAttackFallbackWithStrafe = true;

	if (TryPlanAttackCombatPlan(bShouldCloseAfterBow))
	{
		return;
	}

	PlanNonAttackCombatPlan();
}

bool AJunBoss::TryPlanForcedApproachAfterBowAttack(bool bShouldCloseAfterBow)
{
	if (!bShouldCloseAfterBow || HasAnyNonBowAttackCandidateForCurrentDistance())
	{
		return false;
	}

	bPostBowCloseRangeApproachInProgress = true;
	BeginNoAttackCandidateApproach();
	CurrentCombatPlan.ActionType = EBossPlannedActionType::None;
	return true;
}

bool AJunBoss::TryPlanApproachUntilAttackCandidate()
{
	if (!IsFartherThanAllAttackCandidates())
	{
		return false;
	}

	BeginNoAttackCandidateApproach();
	CurrentCombatPlan.ActionType = EBossPlannedActionType::None;
	return true;
}

bool AJunBoss::TryPlanNoAttackFallback()
{
	if (HasAnyAttackCandidateForCurrentDistance())
	{
		return false;
	}

	if (bUseNonAttackFallbackUntilAttackCandidateAppears)
	{
		CurrentCombatPlan.ActionType = EBossPlannedActionType::NonAttack;
		RefreshNoAttackFallbackMovementType();
		return true;
	}

	BeginNoAttackCandidateApproach();
	CurrentCombatPlan.ActionType = EBossPlannedActionType::None;
	return true;
}

bool AJunBoss::TryPlanAttackCombatPlan(bool bShouldCloseAfterBow)
{
	if (bShouldCloseAfterBow)
	{
		bPostBowCloseRangeApproachInProgress = true;
		CurrentCombatPlan.ActionType = EBossPlannedActionType::Attack;
		RefreshPlannedAttackType();
		if (CurrentCombatPlan.AttackType == EBossPlannedAttackType::None)
		{
			BeginNoAttackCandidateApproach();
			CurrentCombatPlan.ActionType = EBossPlannedActionType::None;
		}
		return true;
	}

	if (FMath::FRand() > AttackPlanWeight)
	{
		return false;
	}

	CurrentCombatPlan.ActionType = EBossPlannedActionType::Attack;
	RefreshPlannedAttackType();
	if (CurrentCombatPlan.AttackType == EBossPlannedAttackType::None)
	{
		CurrentCombatPlan.ActionType = EBossPlannedActionType::NonAttack;
		RefreshNoAttackFallbackMovementType();
	}
	return true;
}

void AJunBoss::PlanNonAttackCombatPlan()
{
	CurrentCombatPlan.ActionType = EBossPlannedActionType::NonAttack;
	RefreshPlannedNonAttackType();
}

void AJunBoss::RefreshPlannedNonAttackType()
{
	CurrentCombatPlan.MovementDirection = ChooseStrafeDirectionAvoidingImmediateReverse();
	CurrentCombatPlan.NonAttackType = EBossPlannedNonAttackType::Strafe;
	CurrentCombatPlan.MovementDuration = StrafeMinDuration;
}

void AJunBoss::RefreshNoAttackFallbackMovementType()
{
	if (bShouldStartNoAttackFallbackWithStrafe)
	{
		bShouldStartNoAttackFallbackWithStrafe = false;
		CurrentCombatPlan.NonAttackType = EBossPlannedNonAttackType::Strafe;
		CurrentCombatPlan.MovementDirection = ChooseStrafeDirectionAvoidingImmediateReverse();
		CurrentCombatPlan.MovementDuration = StrafeMinDuration;
		return;
	}

	TArray<EBossPlannedNonAttackType> NonAttackCandidates;
	NonAttackCandidates.Add(EBossPlannedNonAttackType::Strafe);

	if (NonAttackCandidates.Num() <= 0)
	{
		BeginNoAttackCandidateApproach();
		CurrentCombatPlan.NonAttackType = EBossPlannedNonAttackType::None;
		CurrentCombatPlan.MovementDuration = 0.f;
		return;
	}

	CurrentCombatPlan.NonAttackType = NonAttackCandidates[FMath::RandRange(0, NonAttackCandidates.Num() - 1)];
	CurrentCombatPlan.MovementDirection = ChooseStrafeDirectionAvoidingImmediateReverse();

	if (CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe)
	{
		CurrentCombatPlan.MovementDuration = StrafeMinDuration;
		return;
	}

	if (UAnimMontage* NonAttackMontage = GetPlannedNonAttackMontage())
	{
		CurrentCombatPlan.MovementDuration = NonAttackMontage->GetPlayLength();
		return;
	}

	BeginNoAttackCandidateApproach();
	CurrentCombatPlan.NonAttackType = EBossPlannedNonAttackType::None;
	CurrentCombatPlan.MovementDuration = 0.f;
}

bool AJunBoss::EvaluateReactiveBackwardEvade(FBossReactiveActionExecutionData& OutExecutionData) const
{
	const FBossReactiveActionTuningData* ReactiveEvadeTuningData =
		GetReactiveActionTuningData(EBossReactiveActionType::EvadeBackward);
	if (!ReactiveEvadeTuningData)
	{
		return false;
	}

	const bool bUsedDashRecently = WasRecentlyUsed(EBossActionType::Dash, 1);
	const bool bUsedDodgeRecently = WasRecentlyUsed(EBossActionType::Dodge, 1);

	const bool bCanUseDash = DashBackwardMontage && !bUsedDashRecently;
	const bool bCanUseDodge = DodgeBackwardMontage && !bUsedDodgeRecently;
	const bool bCanFallbackDash = DashBackwardMontage != nullptr;
	const bool bCanFallbackDodge = DodgeBackwardMontage != nullptr;

	EBossPlannedNonAttackType SelectedEvadeType = EBossPlannedNonAttackType::None;
	const EBossPlannedNonAttackType PrimaryNonAttackType = ReactiveEvadeTuningData->PrimaryNonAttackType;
	const EBossPlannedNonAttackType SecondaryNonAttackType = ReactiveEvadeTuningData->SecondaryNonAttackType;
	const bool bPreferPrimaryType = FMath::FRand() <= ReactiveEvadeTuningData->PrimaryNonAttackTypeChance;
	const EBossPlannedNonAttackType PreferredType = bPreferPrimaryType ? PrimaryNonAttackType : SecondaryNonAttackType;
	const EBossPlannedNonAttackType FallbackType = bPreferPrimaryType ? SecondaryNonAttackType : PrimaryNonAttackType;

	auto CanUseNonAttackType = [bCanUseDash, bCanUseDodge](EBossPlannedNonAttackType NonAttackType)
	{
		switch (NonAttackType)
		{
		case EBossPlannedNonAttackType::Dash:
			return bCanUseDash;
		case EBossPlannedNonAttackType::Dodge:
			return bCanUseDodge;
		default:
			return false;
		}
	};

	auto CanFallbackNonAttackType = [bCanFallbackDash, bCanFallbackDodge](EBossPlannedNonAttackType NonAttackType)
	{
		switch (NonAttackType)
		{
		case EBossPlannedNonAttackType::Dash:
			return bCanFallbackDash;
		case EBossPlannedNonAttackType::Dodge:
			return bCanFallbackDodge;
		default:
			return false;
		}
	};

	if (bCanUseDash && bCanUseDodge)
	{
		SelectedEvadeType = PreferredType;
	}
	else if (CanUseNonAttackType(PreferredType))
	{
		SelectedEvadeType = PreferredType;
	}
	else if (CanUseNonAttackType(FallbackType))
	{
		SelectedEvadeType = FallbackType;
	}
	else if (bCanFallbackDash && bCanFallbackDodge)
	{
		SelectedEvadeType = PreferredType;
	}
	else if (CanFallbackNonAttackType(PreferredType))
	{
		SelectedEvadeType = PreferredType;
	}
	else if (CanFallbackNonAttackType(FallbackType))
	{
		SelectedEvadeType = FallbackType;
	}

	if (SelectedEvadeType == EBossPlannedNonAttackType::None)
	{
		return false;
	}

	OutExecutionData.Type = EBossReactiveActionType::EvadeBackward;
	OutExecutionData.NonAttackType = SelectedEvadeType;
	OutExecutionData.MovementDirection = ReactiveEvadeTuningData->MovementDirection;
	OutExecutionData.Duration = 0.f;
	return true;
}
