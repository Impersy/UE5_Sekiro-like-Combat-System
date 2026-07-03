#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"

namespace
{
	const TCHAR* GetBossCombatStateDebugText(EBossCombatState State)
	{
		switch (State)
		{
		case EBossCombatState::Approach:
			return TEXT("Approach");
		case EBossCombatState::Idle:
			return TEXT("Idle");
		case EBossCombatState::Hit:
			return TEXT("Hit");
		case EBossCombatState::Attack:
			return TEXT("Attack");
		case EBossCombatState::ParrySuccess:
			return TEXT("ParrySuccess");
		case EBossCombatState::Recovery:
			return TEXT("Recovery");
		case EBossCombatState::Reposition:
			return TEXT("Reposition");
		case EBossCombatState::NonAttackAction:
			return TEXT("NonAttackAction");
		case EBossCombatState::Evade:
			return TEXT("Evade");
		case EBossCombatState::Turn:
			return TEXT("Turn");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* GetBossPlannedAttackDebugText(EBossPlannedAttackType AttackType)
	{
		switch (AttackType)
		{
		case EBossPlannedAttackType::ComboAttack:
			return TEXT("ComboAttack");
		case EBossPlannedAttackType::NormalAttack:
			return TEXT("NormalAttack");
		case EBossPlannedAttackType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetBossPlannedActionDebugText(EBossPlannedActionType ActionType)
	{
		switch (ActionType)
		{
		case EBossPlannedActionType::Attack:
			return TEXT("Attack");
		case EBossPlannedActionType::NonAttack:
			return TEXT("NonAttack");
		case EBossPlannedActionType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetBossPlannedNonAttackDebugText(EBossPlannedNonAttackType NonAttackType)
	{
		switch (NonAttackType)
		{
		case EBossPlannedNonAttackType::Strafe:
			return TEXT("Strafe");
		case EBossPlannedNonAttackType::Dash:
			return TEXT("Dash");
		case EBossPlannedNonAttackType::Dodge:
			return TEXT("Dodge");
		case EBossPlannedNonAttackType::Hold:
			return TEXT("Hold");
		case EBossPlannedNonAttackType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetBossComboSetDebugText(EBossComboSet ComboSet)
	{
		switch (ComboSet)
		{
		case EBossComboSet::ComboA:
			return TEXT("ComboA");
		case EBossComboSet::ComboB:
			return TEXT("ComboB");
		case EBossComboSet::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetBossNormalAttackDebugText(EBossNormalAttackType AttackType)
	{
		switch (AttackType)
		{
		case EBossNormalAttackType::JumpAttack:
			return TEXT("JumpAttack");
		case EBossNormalAttackType::ChargeAttack:
			return TEXT("ChargeAttack");
		case EBossNormalAttackType::DodgeAttack:
			return TEXT("DodgeAttack");
		case EBossNormalAttackType::NinjaA:
			return TEXT("NinjaA");
		case EBossNormalAttackType::NinjaB:
			return TEXT("NinjaB");
		case EBossNormalAttackType::Execution:
			return TEXT("Execution");
		case EBossNormalAttackType::Bow4Combo:
			return TEXT("Bow4Combo");
		case EBossNormalAttackType::BowBackDashAttack:
			return TEXT("BowBackDashAttack");
		case EBossNormalAttackType::BowHeavyAttack:
			return TEXT("BowHeavyAttack");
		case EBossNormalAttackType::FastComeSlash:
			return TEXT("FastComeSlash");
		case EBossNormalAttackType::SlowComeSlash:
			return TEXT("SlowComeSlash");
		case EBossNormalAttackType::FakeDownSlash:
			return TEXT("FakeDownSlash");
		case EBossNormalAttackType::LionSlash:
			return TEXT("LionSlash");
		case EBossNormalAttackType::SpinSlash:
			return TEXT("SpinSlash");
		case EBossNormalAttackType::LightningSword:
			return TEXT("LightningSword");
		case EBossNormalAttackType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetBossInitiativeDebugText(EBossInitiativeState Initiative)
	{
		switch (Initiative)
		{
		case EBossInitiativeState::PlayerPressure:
			return TEXT("PlayerPressure");
		case EBossInitiativeState::BossPattern:
			return TEXT("BossPattern");
		case EBossInitiativeState::BossParryCounter:
			return TEXT("BossParryCounter");
		case EBossInitiativeState::Neutral:
		default:
			return TEXT("Neutral");
		}
	}

	const TCHAR* GetBossCombatStateChangeReasonDebugText(EBossCombatStateChangeReason Reason)
	{
		switch (Reason)
		{
		case EBossCombatStateChangeReason::CombatEntered:
			return TEXT("CombatEntered");
		case EBossCombatStateChangeReason::PlayerDeathWait:
			return TEXT("PlayerDeathWait");
		case EBossCombatStateChangeReason::PlayerDeathReset:
			return TEXT("PlayerDeathReset");
		case EBossCombatStateChangeReason::PlanReady:
			return TEXT("PlanReady");
		case EBossCombatStateChangeReason::PlanInvalid:
			return TEXT("PlanInvalid");
		case EBossCombatStateChangeReason::TargetRangeChanged:
			return TEXT("TargetRangeChanged");
		case EBossCombatStateChangeReason::AttackStarted:
			return TEXT("AttackStarted");
		case EBossCombatStateChangeReason::AttackEnded:
			return TEXT("AttackEnded");
		case EBossCombatStateChangeReason::HitReactStarted:
			return TEXT("HitReactStarted");
		case EBossCombatStateChangeReason::HitReactEnded:
			return TEXT("HitReactEnded");
		case EBossCombatStateChangeReason::ParrySucceeded:
			return TEXT("ParrySucceeded");
		case EBossCombatStateChangeReason::ParryEnded:
			return TEXT("ParryEnded");
		case EBossCombatStateChangeReason::RecoveryEnded:
			return TEXT("RecoveryEnded");
		case EBossCombatStateChangeReason::MobilityStarted:
			return TEXT("MobilityStarted");
		case EBossCombatStateChangeReason::MobilityEnded:
			return TEXT("MobilityEnded");
		case EBossCombatStateChangeReason::TurnStarted:
			return TEXT("TurnStarted");
		case EBossCombatStateChangeReason::TurnEnded:
			return TEXT("TurnEnded");
		case EBossCombatStateChangeReason::ExecutionReadyStarted:
			return TEXT("ExecutionReadyStarted");
		case EBossCombatStateChangeReason::ExecutionReadyEnded:
			return TEXT("ExecutionReadyEnded");
		case EBossCombatStateChangeReason::ReactiveAction:
			return TEXT("ReactiveAction");
		case EBossCombatStateChangeReason::Unspecified:
		default:
			return TEXT("Unspecified");
		}
	}
}

void AJunBoss::HandleHitTurnDecisionPoint()
{
	TryStartHitReactTurnTransition(true);
}

void AJunBoss::UpdateCombat(float DeltaTime)
{
	UpdateBossCombatTimers(DeltaTime);

	if (!EnsureBossCombatTargetValid())
	{
		return;
	}

	CombatSubStateElapsedTime += DeltaTime;
	UpdatePlannedCombatPlanAge(DeltaTime);
	UpdateCurrentBossCombatState(DeltaTime);
}

void AJunBoss::UpdateBossCombatTimers(float DeltaTime)
{
	UpdateTimedMontageSuperArmor(DeltaTime);
	UpdatePlayerPressure(DeltaTime);
	UpdateParryExchangeDecay(DeltaTime);

	if (ParryExchangeState.SuccessExitActionLockRemainTime > 0.f)
	{
		ParryExchangeState.SuccessExitActionLockRemainTime = FMath::Max(0.f, ParryExchangeState.SuccessExitActionLockRemainTime - DeltaTime);
	}

	if (CodeFacingLockRemainTime > 0.f)
	{
		CodeFacingLockRemainTime = FMath::Max(0.f, CodeFacingLockRemainTime - DeltaTime);
	}

	if (CloseRangeBackDashCooldownRemainTime > 0.f)
	{
		CloseRangeBackDashCooldownRemainTime = FMath::Max(0.f, CloseRangeBackDashCooldownRemainTime - DeltaTime);
	}
}

bool AJunBoss::EnsureBossCombatTargetValid()
{
	if (!CurrentTarget)
	{
		RestoreAttackGroundMotionOverride();
		SetMonsterState(EMonsterState::Patrol);
		return false;
	}

	if (!CanRemainInCombat(CurrentTarget))
	{
		RestoreAttackGroundMotionOverride();
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		CurrentTarget = nullptr;
		bIsHasTarget = false;
		CombatMoveInput = FVector2D::ZeroVector;
		SetMonsterState(EMonsterState::Return);
		return false;
	}

	return true;
}

void AJunBoss::UpdateCurrentBossCombatState(float DeltaTime)
{
	switch (CurrentCombatSubState)
	{
	case EBossCombatState::Approach:
		UpdateApproachState(DeltaTime);
		break;
	case EBossCombatState::Idle:
		UpdateIdleState(DeltaTime);
		break;
	case EBossCombatState::Hit:
		UpdateHitState(DeltaTime);
		break;
	case EBossCombatState::Attack:
		UpdateAttackState(DeltaTime);
		break;
	case EBossCombatState::ParrySuccess:
		UpdateParrySuccessState(DeltaTime);
		break;
	case EBossCombatState::Recovery:
		UpdateRecoveryState(DeltaTime);
		break;
	case EBossCombatState::Reposition:
		UpdateRepositionState(DeltaTime);
		break;
	case EBossCombatState::NonAttackAction:
		UpdateNonAttackActionState(DeltaTime);
		break;
	case EBossCombatState::Evade:
		UpdateEvadeState(DeltaTime);
		break;
	case EBossCombatState::Turn:
		UpdateTurnState(DeltaTime);
		break;
	default:
		break;
	}
}

void AJunBoss::TransitionBossCombatState(
	EBossCombatState NewState,
	EBossCombatStateChangeReason Reason,
	EBossCombatStateTransitionMode TransitionMode)
{
	const bool bShouldEnterState = TransitionMode == EBossCombatStateTransitionMode::EnterState;
	const bool bStateChanged = CurrentCombatSubState != NewState;
	if (!bStateChanged && bShouldEnterState)
	{
		return;
	}

	const bool bTransitionAllowed = IsBossCombatStateTransitionAllowed(
		CurrentCombatSubState,
		NewState,
		Reason,
		TransitionMode);
	if (!bTransitionAllowed)
	{
		if (bAuditBossCombatStateTransitions)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("BossFSM PolicyViolation | From=%s To=%s Reason=%s Mode=%s Enforced=%d"),
				GetBossCombatStateDebugText(CurrentCombatSubState),
				GetBossCombatStateDebugText(NewState),
				GetBossCombatStateChangeReasonDebugText(Reason),
				TransitionMode == EBossCombatStateTransitionMode::EnterState
					? TEXT("EnterState")
					: TEXT("SynchronizeOnly"),
				bEnforceBossCombatStateTransitionPolicy ? 1 : 0);
		}

		if (bEnforceBossCombatStateTransitionPolicy)
		{
			return;
		}
	}

	const EBossCombatState PreviousState = CurrentCombatSubState;
	if (bStateChanged)
	{
		ExitCurrentBossCombatState(NewState, Reason);
	}

	CurrentCombatSubState = NewState;
	CombatSubStateElapsedTime = 0.f;
	LastCombatSubStateChangeReason = Reason;
	if (bShouldEnterState)
	{
		CodeFacingLockRemainTime = FMath::Max(CodeFacingLockRemainTime, FacingStateEntryLockDuration);
	}
	RecordBossCombatStateTransition(PreviousState, NewState, Reason, bShouldEnterState);

	if (!bShouldEnterState)
	{
		return;
	}

	switch (CurrentCombatSubState)
	{
	case EBossCombatState::Approach:
		EnterApproachState();
		break;
	case EBossCombatState::Idle:
		EnterIdleState();
		break;
	case EBossCombatState::Hit:
		EnterHitState();
		break;
	case EBossCombatState::Attack:
		EnterAttackState();
		break;
	case EBossCombatState::ParrySuccess:
		EnterParrySuccessState();
		break;
	case EBossCombatState::Recovery:
		EnterRecoveryState();
		break;
	case EBossCombatState::Reposition:
		EnterRepositionState();
		break;
	case EBossCombatState::NonAttackAction:
		EnterNonAttackActionState();
		break;
	case EBossCombatState::Evade:
		EnterEvadeState();
		break;
	case EBossCombatState::Turn:
		EnterTurnState();
		break;
	default:
		break;
	}
}

bool AJunBoss::IsBossCombatStateTransitionAllowed(
	EBossCombatState FromState,
	EBossCombatState ToState,
	EBossCombatStateChangeReason Reason,
	EBossCombatStateTransitionMode TransitionMode) const
{
	if (FromState == ToState)
	{
		return TransitionMode == EBossCombatStateTransitionMode::SynchronizeOnly;
	}

	const bool bGlobalInterruptTransition =
		(Reason == EBossCombatStateChangeReason::HitReactStarted && ToState == EBossCombatState::Hit) ||
		(Reason == EBossCombatStateChangeReason::ParrySucceeded && ToState == EBossCombatState::ParrySuccess) ||
		Reason == EBossCombatStateChangeReason::ExecutionReadyStarted ||
		Reason == EBossCombatStateChangeReason::ExecutionReadyEnded ||
		Reason == EBossCombatStateChangeReason::PlayerDeathWait ||
		Reason == EBossCombatStateChangeReason::PlayerDeathReset;
	if (bGlobalInterruptTransition)
	{
		return true;
	}

	switch (FromState)
	{
	case EBossCombatState::Approach:
		return ToState == EBossCombatState::Idle ||
			ToState == EBossCombatState::Attack ||
			ToState == EBossCombatState::Reposition ||
			ToState == EBossCombatState::Turn;

	case EBossCombatState::Idle:
		return ToState == EBossCombatState::Approach ||
			ToState == EBossCombatState::Attack ||
			ToState == EBossCombatState::Reposition ||
			ToState == EBossCombatState::NonAttackAction ||
			ToState == EBossCombatState::Evade ||
			ToState == EBossCombatState::Turn;

	case EBossCombatState::Hit:
		return ToState == EBossCombatState::Idle ||
			ToState == EBossCombatState::Turn ||
			ToState == EBossCombatState::Attack;

	case EBossCombatState::Attack:
		return ToState == EBossCombatState::Recovery;

	case EBossCombatState::ParrySuccess:
		return ToState == EBossCombatState::Idle ||
			ToState == EBossCombatState::Attack;

	case EBossCombatState::Recovery:
		return ToState == EBossCombatState::Idle ||
			ToState == EBossCombatState::Attack;

	case EBossCombatState::Reposition:
		return ToState == EBossCombatState::Idle ||
			ToState == EBossCombatState::Approach ||
			ToState == EBossCombatState::Attack ||
			ToState == EBossCombatState::Evade ||
			ToState == EBossCombatState::Turn;

	case EBossCombatState::NonAttackAction:
		return ToState == EBossCombatState::Idle;

	case EBossCombatState::Evade:
		return ToState == EBossCombatState::Idle ||
			ToState == EBossCombatState::Attack;

	case EBossCombatState::Turn:
		return ToState == EBossCombatState::Idle;

	default:
		return false;
	}
}

void AJunBoss::ExitCurrentBossCombatState(EBossCombatState NextState, EBossCombatStateChangeReason Reason)
{
	switch (CurrentCombatSubState)
	{
	case EBossCombatState::Approach:
	case EBossCombatState::Reposition:
		CombatMoveInput = FVector2D::ZeroVector;
		SetDesiredMoveAxes(0.f, 0.f);
		break;
	case EBossCombatState::ParrySuccess:
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
		}
		break;
	case EBossCombatState::Evade:
		if (ReactiveActionState.ActiveAction.Type != EBossReactiveActionType::None)
		{
			ResetActiveReactiveActionState();
		}
		break;
	default:
		break;
	}
}

void AJunBoss::RecordBossCombatStateTransition(
	EBossCombatState FromState,
	EBossCombatState ToState,
	EBossCombatStateChangeReason Reason,
	bool bEnteredState)
{
	FString TransitionWarning;
	if (IsBossCombatStateTransitionSuspicious(FromState, ToState, Reason, bEnteredState, TransitionWarning))
	{
		UE_LOG(
			LogJun,
			Warning,
			TEXT("BossFSM SuspiciousTransition | Boss=%s From=%s To=%s Reason=%s Enter=%d ExecReady=%d BeingExecuted=%d Attack=%d HitReact=%d Warning=%s"),
			*GetNameSafe(this),
			GetBossCombatStateDebugText(FromState),
			GetBossCombatStateDebugText(ToState),
			GetBossCombatStateChangeReasonDebugText(Reason),
			bEnteredState ? 1 : 0,
			bExecutionReady ? 1 : 0,
			bBeingExecuted ? 1 : 0,
			bIsAttacking ? 1 : 0,
			IsInHitReact() ? 1 : 0,
			*TransitionWarning);
	}

	FBossCombatStateTransitionRecord Record;
	Record.FromState = FromState;
	Record.ToState = ToState;
	Record.Reason = Reason;
	Record.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Record.bEnteredState = bEnteredState;

	CombatStateTransitionHistory.Add(Record);

	const int32 MaxHistory = FMath::Max(1, MaxCombatStateTransitionHistory);
	while (CombatStateTransitionHistory.Num() > MaxHistory)
	{
		CombatStateTransitionHistory.RemoveAt(0, 1, EAllowShrinking::No);
	}
}

bool AJunBoss::IsBossCombatStateTransitionSuspicious(
	EBossCombatState FromState,
	EBossCombatState ToState,
	EBossCombatStateChangeReason Reason,
	bool bEnteredState,
	FString& OutWarning) const
{
	if (bExecutionReady || bBeingExecuted)
	{
		const bool bExecutionReason =
			Reason == EBossCombatStateChangeReason::ExecutionReadyStarted ||
			Reason == EBossCombatStateChangeReason::ExecutionReadyEnded ||
			Reason == EBossCombatStateChangeReason::PlayerDeathReset ||
			Reason == EBossCombatStateChangeReason::PlayerDeathWait;

		if (!bExecutionReason)
		{
			OutWarning = TEXT("Execution state is active but boss combat state changed for non-execution reason.");
			return true;
		}
	}

	if (FromState == EBossCombatState::Attack &&
		ToState == EBossCombatState::Idle &&
		Reason != EBossCombatStateChangeReason::AttackEnded &&
		Reason != EBossCombatStateChangeReason::PlanInvalid &&
		Reason != EBossCombatStateChangeReason::ExecutionReadyStarted &&
		Reason != EBossCombatStateChangeReason::PlayerDeathReset &&
		Reason != EBossCombatStateChangeReason::PlayerDeathWait)
	{
		OutWarning = TEXT("Attack changed directly to Idle with an unusual reason.");
		return true;
	}

	if (FromState == EBossCombatState::Hit &&
		ToState == EBossCombatState::Attack &&
		Reason != EBossCombatStateChangeReason::AttackStarted &&
		Reason != EBossCombatStateChangeReason::ReactiveAction)
	{
		OutWarning = TEXT("Hit changed to Attack without a planned attack or reactive action reason.");
		return true;
	}

	if (FromState == EBossCombatState::ParrySuccess &&
		ToState == EBossCombatState::Approach)
	{
		OutWarning = TEXT("ParrySuccess changed to Approach directly; this may skip parry follow-up handling.");
		return true;
	}

	if (!bEnteredState && ToState != EBossCombatState::Attack && ToState != EBossCombatState::ParrySuccess)
	{
		OutWarning = TEXT("State was synchronized without Enter for a state that is not an approved sync target.");
		return true;
	}

	return false;
}

