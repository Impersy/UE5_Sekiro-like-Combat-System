#include "Character/WukongBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/JunPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Player/JunPlayerController.h"
#include "System/JunCombatVFXSubsystem.h"
#include "Weapon/ArrowProjectile.h"

namespace
{
	const TCHAR* GetWukongCombatStateDebugText(EWukongCombatState State)
	{
		switch (State)
		{
		case EWukongCombatState::Approach:
			return TEXT("Approach");
		case EWukongCombatState::Idle:
			return TEXT("Idle");
		case EWukongCombatState::Attack:
			return TEXT("Attack");
		case EWukongCombatState::ParrySuccess:
			return TEXT("ParrySuccess");
		case EWukongCombatState::Recovery:
			return TEXT("Recovery");
		case EWukongCombatState::Reposition:
			return TEXT("Reposition");
		case EWukongCombatState::NonAttackAction:
			return TEXT("NonAttackAction");
		case EWukongCombatState::Evade:
			return TEXT("Evade");
		case EWukongCombatState::Turn:
			return TEXT("Turn");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* GetWukongPlannedAttackDebugText(EWukongPlannedAttackType AttackType)
	{
		switch (AttackType)
		{
		case EWukongPlannedAttackType::ComboAttack:
			return TEXT("ComboAttack");
		case EWukongPlannedAttackType::NormalAttack:
			return TEXT("NormalAttack");
		case EWukongPlannedAttackType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetWukongPlannedActionDebugText(EWukongPlannedActionType ActionType)
	{
		switch (ActionType)
		{
		case EWukongPlannedActionType::Attack:
			return TEXT("Attack");
		case EWukongPlannedActionType::NonAttack:
			return TEXT("NonAttack");
		case EWukongPlannedActionType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetWukongPlannedNonAttackDebugText(EWukongPlannedNonAttackType NonAttackType)
	{
		switch (NonAttackType)
		{
		case EWukongPlannedNonAttackType::Strafe:
			return TEXT("Strafe");
		case EWukongPlannedNonAttackType::Dash:
			return TEXT("Dash");
		case EWukongPlannedNonAttackType::Dodge:
			return TEXT("Dodge");
		case EWukongPlannedNonAttackType::Hold:
			return TEXT("Hold");
		case EWukongPlannedNonAttackType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetWukongComboSetDebugText(EWukongComboSet ComboSet)
	{
		switch (ComboSet)
		{
		case EWukongComboSet::ComboA:
			return TEXT("ComboA");
		case EWukongComboSet::ComboB:
			return TEXT("ComboB");
		case EWukongComboSet::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetWukongNormalAttackDebugText(EWukongNormalAttackType AttackType)
	{
		switch (AttackType)
		{
		case EWukongNormalAttackType::JumpAttack:
			return TEXT("JumpAttack");
		case EWukongNormalAttackType::ChargeAttack:
			return TEXT("ChargeAttack");
		case EWukongNormalAttackType::DodgeAttack:
			return TEXT("DodgeAttack");
		case EWukongNormalAttackType::NinjaA:
			return TEXT("NinjaA");
		case EWukongNormalAttackType::NinjaB:
			return TEXT("NinjaB");
		case EWukongNormalAttackType::Execution:
			return TEXT("Execution");
		case EWukongNormalAttackType::Bow4Combo:
			return TEXT("Bow4Combo");
		case EWukongNormalAttackType::BowBackDashAttack:
			return TEXT("BowBackDashAttack");
		case EWukongNormalAttackType::BowHeavyAttack:
			return TEXT("BowHeavyAttack");
		case EWukongNormalAttackType::FastComeSlash:
			return TEXT("FastComeSlash");
		case EWukongNormalAttackType::SlowComeSlash:
			return TEXT("SlowComeSlash");
		case EWukongNormalAttackType::FakeDownSlash:
			return TEXT("FakeDownSlash");
		case EWukongNormalAttackType::LionSlash:
			return TEXT("LionSlash");
		case EWukongNormalAttackType::SpinSlash:
			return TEXT("SpinSlash");
		case EWukongNormalAttackType::LightningSword:
			return TEXT("LightningSword");
		case EWukongNormalAttackType::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* GetWukongInitiativeDebugText(EWukongInitiativeState Initiative)
	{
		switch (Initiative)
		{
		case EWukongInitiativeState::PlayerPressure:
			return TEXT("PlayerPressure");
		case EWukongInitiativeState::BossPattern:
			return TEXT("BossPattern");
		case EWukongInitiativeState::BossParryCounter:
			return TEXT("BossParryCounter");
		case EWukongInitiativeState::Neutral:
		default:
			return TEXT("Neutral");
		}
	}

	const TCHAR* GetWukongCodeMoveStopReasonDebugText(EWukongCodeMoveStopReason StopReason)
	{
		switch (StopReason)
		{
		case EWukongCodeMoveStopReason::Started:
			return TEXT("Started");
		case EWukongCodeMoveStopReason::TooClose:
			return TEXT("TooClose");
		case EWukongCodeMoveStopReason::MaxDistance:
			return TEXT("MaxDistance");
		case EWukongCodeMoveStopReason::DurationEnd:
			return TEXT("DurationEnd");
		case EWukongCodeMoveStopReason::LostTarget:
			return TEXT("LostTarget");
		case EWukongCodeMoveStopReason::InvalidMovement:
			return TEXT("InvalidMovement");
		case EWukongCodeMoveStopReason::Interrupted:
			return TEXT("Interrupted");
		case EWukongCodeMoveStopReason::None:
		default:
			return TEXT("None");
		}
	}

	constexpr EWukongNormalAttackType AllWukongNormalAttackTypes[] = {
		EWukongNormalAttackType::JumpAttack,
		EWukongNormalAttackType::ChargeAttack,
		EWukongNormalAttackType::DodgeAttack,
		EWukongNormalAttackType::NinjaA,
		EWukongNormalAttackType::NinjaB,
		EWukongNormalAttackType::Execution,
		EWukongNormalAttackType::Bow4Combo,
		EWukongNormalAttackType::BowBackDashAttack,
		EWukongNormalAttackType::BowHeavyAttack,
		EWukongNormalAttackType::FastComeSlash,
		EWukongNormalAttackType::SlowComeSlash,
		EWukongNormalAttackType::FakeDownSlash,
		EWukongNormalAttackType::LionSlash,
		EWukongNormalAttackType::SpinSlash,
		EWukongNormalAttackType::LightningSword
	};
}

AWukongBoss::AWukongBoss()
{
	MaxLifeCount = 2;
	MaxPosture = 700.f;
	ExecutionReadyDuration = 2.5f;
	bDisablePostureGain = false;
	bStartWithCutsceneWait = true;
	bStartWithPatrol = false;

	WalkMoveSpeed = 200.f;
	RunMoveSpeed = 700.f;

	ComboAttackA.MaxComboLength = 4;
	ComboAttackA.ContinueChance = 0.8f;
	ComboAttackA.PlayRate = 0.8f;
	ComboAttackA.FacingDurations = { 0.6f, 0.9f, 1.2f, 1.5f };
	ComboAttackA.AttackRange = 350.f;
	ComboAttackA.CandidateRange = 650.f;

	ComboAttackB.MaxComboLength = 5;
	ComboAttackB.ContinueChance = 0.9f;
	ComboAttackB.PlayRate = 0.85f;
	ComboAttackB.FacingDurations = { 0.6f, 0.9f, 1.2f, 1.8f };
	ComboAttackB.AttackRange = 350.f;
	ComboAttackB.CandidateRange = 650.f;

	JumpAttack.MinRange = 0.f;
	JumpAttack.MaxRange = 800.f;
	JumpAttack.CandidateMaxRange = 900.f;
	JumpAttack.bFaceTargetDuringAttack = true;
	JumpAttack.PlayRate = 1.3f;
	JumpAttack.bUseAnimNotifyTiming = true;
	JumpAttack.MoveStartTimeAtPlayRateOne = 0.14f;
	JumpAttack.MoveSpeed = 1200.f;
	JumpAttack.MoveStandOffDistance = 60.f;
	JumpAttack.GroundMotionOverrideDuration = 0.8f;
	JumpAttack.AirTrackInterpSpeed = 8.f;
	JumpAttack.CodeMoveStopDistance = 20.f;
	JumpAttack.CodeMoveMaxDistance = 700.f;
	JumpAttack.bTryTurnAfterAttack = true;
	JumpAttack.PostAttackTurnStartAngle = TurnStartAngle;

	ChargeAttack.MinRange = 0.f;
	ChargeAttack.MaxRange = 700.f;
	ChargeAttack.CandidateMaxRange = 900.f;
	ChargeAttack.bFaceTargetDuringAttack = true;
	ChargeAttack.bUseAnimNotifyTiming = true;
	ChargeAttack.MoveStartTimeAtPlayRateOne = 0.28f;
	ChargeAttack.MoveSpeed = 2500.f;
	ChargeAttack.MoveStandOffDistance = 20.f;
	ChargeAttack.GroundMotionOverrideDuration = 0.45f;
	ChargeAttack.AirTrackInterpSpeed = 10.f;
	ChargeAttack.CodeMoveStopDistance = 120.f;
	ChargeAttack.CodeMoveMaxDistance = 600.f;
	ChargeAttack.bTryTurnAfterAttack = true;
	ChargeAttack.PostAttackTurnStartAngle = TurnStartAngle;

	DodgeAttack.MinRange = 200.f;
	DodgeAttack.MaxRange = 700.f;
	DodgeAttack.CandidateMaxRange = 900.f;
	DodgeAttack.bFaceTargetDuringAttack = true;
	DodgeAttack.PlayRate = 1.3f;
	DodgeAttack.bUseAnimNotifyTiming = true;
	DodgeAttack.bTryTurnAfterAttack = true;
	DodgeAttack.PostAttackTurnStartAngle = TurnStartAngle;
	DodgeAttack.SelectionWeight = 1;

	NinjaAAttack.MinRange = 0.f;
	NinjaAAttack.MaxRange = 450.f;
	NinjaAAttack.CandidateMaxRange = 900.f;
	NinjaAAttack.PlayRate = 0.95f;
	NinjaAAttack.bUseAnimNotifyTiming = true;
	NinjaAAttack.bTryTurnAfterAttack = true;
	NinjaAAttack.PostAttackTurnStartAngle = TurnStartAngle;

	NinjaBAttack.MinRange = 0.f;
	NinjaBAttack.MaxRange = 700.f;
	NinjaBAttack.CandidateMaxRange = 900.f;
	NinjaBAttack.bUseAnimNotifyTiming = true;
	NinjaBAttack.MoveStartTimeAtPlayRateOne = 0.35f;
	NinjaBAttack.MoveSpeed = 2500.f;
	NinjaBAttack.MoveStandOffDistance = 50.f;
	NinjaBAttack.GroundMotionOverrideDuration = 0.5f;
	NinjaBAttack.AirTrackInterpSpeed = 8.f;
	NinjaBAttack.CodeMoveStopDistance = 120.f;
	NinjaBAttack.CodeMoveMaxDistance = 500.f;

	ExecutionAttack.MinRange = 0.f;
	ExecutionAttack.MaxRange = 450.f;
	ExecutionAttack.CandidateMaxRange = 900.f;
	ExecutionAttack.bUseAnimNotifyTiming = true;
	ExecutionAttack.bTryTurnAfterAttack = true;
	ExecutionAttack.PostAttackTurnStartAngle = TurnStartAngle;

	LightningSwordAttack.MinRange = 0.f;
	LightningSwordAttack.MaxRange = 700.f;
	LightningSwordAttack.CandidateMaxRange = 900.f;
	LightningSwordAttack.bFaceTargetDuringAttack = true;
	LightningSwordAttack.FacingDuration = 0.8f;
	LightningSwordAttack.FacingInterpSpeed = 10.f;
	LightningSwordAttack.bUseAnimNotifyTiming = true;
	LightningSwordAttack.SelectionWeight = 1;
	LightningSwordAttack.bTryTurnAfterAttack = true;
	LightningSwordAttack.PostAttackTurnStartAngle = TurnStartAngle;

	BowBackDashAttack.MinRange = 100.f;
	BowBackDashAttack.MaxRange = 500.f;
	BowBackDashAttack.CandidateMaxRange = 800.f;
	BowBackDashAttack.bFaceTargetDuringAttack = true;
	BowBackDashAttack.FacingDuration = 0.35f;
	BowBackDashAttack.FacingInterpSpeed = 18.f;
	BowBackDashAttack.SelectionWeight = 1;
	BowBackDashAttack.bTryTurnAfterAttack = true;
	BowBackDashAttack.PostAttackTurnStartAngle = TurnStartAngle;

	Bow4ComboAttack.MinRange = 500.f;
	Bow4ComboAttack.MaxRange = 1600.f;
	Bow4ComboAttack.CandidateMaxRange = 1800.f;
	Bow4ComboAttack.bFaceTargetDuringAttack = true;
	Bow4ComboAttack.FacingDuration = 0.5f;
	Bow4ComboAttack.FacingInterpSpeed = 16.f;
	Bow4ComboAttack.SelectionWeight = 1;
	Bow4ComboAttack.bTryTurnAfterAttack = true;
	Bow4ComboAttack.PostAttackTurnStartAngle = TurnStartAngle;

	BowHeavyAttack.MinRange = 100.f;
	BowHeavyAttack.MaxRange = 1600.f;
	BowHeavyAttack.CandidateMaxRange = 1800.f;
	BowHeavyAttack.bFaceTargetDuringAttack = true;
	BowHeavyAttack.FacingDuration = 0.65f;
	BowHeavyAttack.FacingInterpSpeed = 16.f;
	BowHeavyAttack.SelectionWeight = 1;
	BowHeavyAttack.bTryTurnAfterAttack = true;
	BowHeavyAttack.PostAttackTurnStartAngle = TurnStartAngle;

	FastComeSlashAttack.MinRange = 150.f;
	FastComeSlashAttack.MaxRange = 700.f;
	FastComeSlashAttack.CandidateMaxRange = 900.f;
	FastComeSlashAttack.bFaceTargetDuringAttack = true;
	FastComeSlashAttack.FacingDuration = 0.35f;
	FastComeSlashAttack.FacingInterpSpeed = 18.f;
	FastComeSlashAttack.bUseAnimNotifyTiming = true;
	FastComeSlashAttack.SelectionWeight = 1;
	FastComeSlashAttack.bTryTurnAfterAttack = true;
	FastComeSlashAttack.PostAttackTurnStartAngle = TurnStartAngle;

	SlowComeSlashAttack.MinRange = 100.f;
	SlowComeSlashAttack.MaxRange = 450.f;
	SlowComeSlashAttack.CandidateMaxRange = 900.f;
	SlowComeSlashAttack.bFaceTargetDuringAttack = true;
	SlowComeSlashAttack.FacingDuration = 0.35f;
	SlowComeSlashAttack.FacingInterpSpeed = 16.f;
	SlowComeSlashAttack.bUseAnimNotifyTiming = true;
	SlowComeSlashAttack.SelectionWeight = 1;
	SlowComeSlashAttack.bTryTurnAfterAttack = true;
	SlowComeSlashAttack.PostAttackTurnStartAngle = TurnStartAngle;

	FakeDownSlashAttack.MinRange = 0.f;
	FakeDownSlashAttack.MaxRange = 700.f;
	FakeDownSlashAttack.CandidateMaxRange = 900.f;
	FakeDownSlashAttack.bFaceTargetDuringAttack = true;
	FakeDownSlashAttack.FacingDuration = 0.4f;
	FakeDownSlashAttack.FacingInterpSpeed = 16.f;
	FakeDownSlashAttack.bUseAnimNotifyTiming = true;
	FakeDownSlashAttack.SelectionWeight = 1;
	FakeDownSlashAttack.bTryTurnAfterAttack = true;
	FakeDownSlashAttack.PostAttackTurnStartAngle = TurnStartAngle;

	LionSlashAttack.MinRange = 0.f;
	LionSlashAttack.MaxRange = 450.f;
	LionSlashAttack.CandidateMaxRange = 900.f;
	LionSlashAttack.bFaceTargetDuringAttack = true;
	LionSlashAttack.FacingDuration = 0.35f;
	LionSlashAttack.FacingInterpSpeed = 18.f;
	LionSlashAttack.bUseAnimNotifyTiming = true;
	LionSlashAttack.SelectionWeight = 1;
	LionSlashAttack.bTryTurnAfterAttack = true;
	LionSlashAttack.PostAttackTurnStartAngle = TurnStartAngle;

	SpinSlashAttack.MinRange = 0.f;
	SpinSlashAttack.MaxRange = 700.f;
	SpinSlashAttack.CandidateMaxRange = 900.f;
	SpinSlashAttack.bFaceTargetDuringAttack = true;
	SpinSlashAttack.FacingDuration = 0.35f;
	SpinSlashAttack.FacingInterpSpeed = 18.f;
	SpinSlashAttack.bUseAnimNotifyTiming = true;
	SpinSlashAttack.SelectionWeight = 1;
	SpinSlashAttack.bTryTurnAfterAttack = true;
	SpinSlashAttack.PostAttackTurnStartAngle = TurnStartAngle;
}

void AWukongBoss::EnterCombatState()
{
	Super::EnterCombatState();
	ResetPlannedCombatPlan();
	ResetCurrentAttackRuntimeState();
	ResetParryExchangeCycle();
	RecentActionHistory.Reset();
	LastStartedNormalAttackType = EWukongNormalAttackType::None;
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetReactiveEvadePressure();
	ClearPlayerPressure();
	ClearNoAttackCandidateApproach();
	bUseNonAttackFallbackUntilAttackCandidateAppears = false;
	bPostBowCloseRangeApproachInProgress = false;
	CloseRangeBackDashCooldownRemainTime = 0.f;
	SetWukongCombatState(EWukongCombatState::Approach);
}

void AWukongBoss::EnterPlayerDeathWait()
{
	Super::EnterPlayerDeathWait();
	ResetPlannedCombatPlan();
	ResetParryExchangeCycle();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetPendingAttackMotionState();
	CurrentCombatSubState = EWukongCombatState::Idle;
	CombatSubStateElapsedTime = 0.f;
}

void AWukongBoss::ResumeAfterPlayerFakeDeath()
{
	Super::ResumeAfterPlayerFakeDeath();
	ResetPlannedCombatPlan();
	ResetParryExchangeCycle();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	SetWukongCombatState(EWukongCombatState::Idle);
}

void AWukongBoss::ResetAfterPlayerRealDeath()
{
	Super::ResetAfterPlayerRealDeath();
	ResetPlannedCombatPlan();
	ResetParryExchangeCycle();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	RecentActionHistory.Reset();
	LastStartedNormalAttackType = EWukongNormalAttackType::None;
	CurrentCombatSubState = EWukongCombatState::Idle;
	CombatSubStateElapsedTime = 0.f;
}

void AWukongBoss::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	Super::HandleGameplayEventNotify(EventTag);

	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Wukong_ParryCounter))
	{
		TryStartParryCounterDecision();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Wukong_ParryCounterFollowUpDecision))
	{
		TryStartParryCounterFollowUp();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Wukong_BowAttackRestoreWeapon))
	{
		EndBowAttackPresentation();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_DangerAttack))
	{
		if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			JunPlayerController->PlayDangerMarkerOnPlayer();
		}
	}
}

void AWukongBoss::HandleHitTurnDecisionPoint()
{
	TryStartHitReactTurnTransition(true);
}

void AWukongBoss::UpdateCombat(float DeltaTime)
{
	UpdateTimedMontageSuperArmor(DeltaTime);
	UpdatePlayerPressure(DeltaTime);
	UpdateParryExchangeDecay(DeltaTime);

	if (ParrySuccessExitActionLockRemainTime > 0.f)
	{
		ParrySuccessExitActionLockRemainTime = FMath::Max(0.f, ParrySuccessExitActionLockRemainTime - DeltaTime);
	}

	if (CodeFacingLockRemainTime > 0.f)
	{
		CodeFacingLockRemainTime = FMath::Max(0.f, CodeFacingLockRemainTime - DeltaTime);
	}

	if (CloseRangeBackDashCooldownRemainTime > 0.f)
	{
		CloseRangeBackDashCooldownRemainTime = FMath::Max(0.f, CloseRangeBackDashCooldownRemainTime - DeltaTime);
	}

	if (PostHeavyHitSuperArmorRemainTime > 0.f)
	{
		PostHeavyHitSuperArmorRemainTime = FMath::Max(0.f, PostHeavyHitSuperArmorRemainTime - DeltaTime);
		if (PostHeavyHitSuperArmorRemainTime <= 0.f && !bIsAttacking)
		{
			RemoveGameplayTag(JunGameplayTags::State_Condition_PostHitArmor);
		}
	}

	if (!CurrentTarget)
	{
		RestoreAttackGroundMotionOverride();
		SetMonsterState(EMonsterState::Patrol);
		return;
	}

	if (!CanRemainInCombat(CurrentTarget))
	{
		RestoreAttackGroundMotionOverride();
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		CurrentTarget = nullptr;
		bIsHasTarget = false;
		CombatMoveInput = FVector2D::ZeroVector;
		SetMonsterState(EMonsterState::Return);
		return;
	}

	CombatSubStateElapsedTime += DeltaTime;
	UpdatePlannedCombatPlanAge(DeltaTime);

	switch (CurrentCombatSubState)
	{
	case EWukongCombatState::Approach:
		UpdateApproachState(DeltaTime);
		break;
	case EWukongCombatState::Idle:
		UpdateIdleState(DeltaTime);
		break;
	case EWukongCombatState::Hit:
		UpdateHitState(DeltaTime);
		break;
	case EWukongCombatState::Attack:
		UpdateAttackState(DeltaTime);
		break;
	case EWukongCombatState::ParrySuccess:
		UpdateParrySuccessState(DeltaTime);
		break;
	case EWukongCombatState::Recovery:
		UpdateRecoveryState(DeltaTime);
		break;
	case EWukongCombatState::Reposition:
		UpdateRepositionState(DeltaTime);
		break;
	case EWukongCombatState::NonAttackAction:
		UpdateNonAttackActionState(DeltaTime);
		break;
	case EWukongCombatState::Evade:
		UpdateEvadeState(DeltaTime);
		break;
	case EWukongCombatState::Turn:
		UpdateTurnState(DeltaTime);
		break;
	default:
		break;
	}
}

FMonsterAttackSelection AWukongBoss::ChooseNextAttackSelection() const
{
	FMonsterAttackSelection Selection = Super::ChooseNextAttackSelection();

	if (PlannedAttackType == EWukongPlannedAttackType::NormalAttack)
	{
		if (const FWukongNormalAttackData* NormalAttackData = GetPlannedNormalAttackData())
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

	if (PlannedAttackType == EWukongPlannedAttackType::ComboAttack)
	{
		if (UAnimMontage* PlannedComboMontage = GetPlannedComboMontage())
		{
			const FWukongComboAttackData* ComboAttackData = GetComboAttackData(PlannedComboSet);
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

void AWukongBoss::SetWukongCombatState(EWukongCombatState NewState)
{
	if (CurrentCombatSubState == NewState)
	{
		return;
	}

	CurrentCombatSubState = NewState;
	CombatSubStateElapsedTime = 0.f;
	CodeFacingLockRemainTime = FMath::Max(CodeFacingLockRemainTime, FacingStateEntryLockDuration);

	switch (CurrentCombatSubState)
	{
	case EWukongCombatState::Approach:
		EnterApproachState();
		break;
	case EWukongCombatState::Idle:
		EnterIdleState();
		break;
	case EWukongCombatState::Hit:
		EnterHitState();
		break;
	case EWukongCombatState::Attack:
		EnterAttackState();
		break;
	case EWukongCombatState::ParrySuccess:
		EnterParrySuccessState();
		break;
	case EWukongCombatState::Recovery:
		EnterRecoveryState();
		break;
	case EWukongCombatState::Reposition:
		EnterRepositionState();
		break;
	case EWukongCombatState::NonAttackAction:
		EnterNonAttackActionState();
		break;
	case EWukongCombatState::Evade:
		EnterEvadeState();
		break;
	case EWukongCombatState::Turn:
		EnterTurnState();
		break;
	default:
		break;
	}
}

void AWukongBoss::EnterApproachState()
{
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bRunLocomotionRequested = false;
}

void AWukongBoss::EnterIdleState()
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

	if (PlannedActionType == EWukongPlannedActionType::Attack &&
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

void AWukongBoss::EnterHitState()
{
	StartPlayerPressure();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	RemoveGameplayTag(JunGameplayTags::State_Condition_PostHitArmor);
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();
	ResetPlannedCombatPlan();
}

void AWukongBoss::EnterAttackState()
{
	ResetReactiveEvadePressure();
	const FMonsterAttackSelection AttackSelection = ChooseNextAttackSelection();
	const FWukongNormalAttackData* CurrentNormalAttackData =
		(PlannedAttackType == EWukongPlannedAttackType::NormalAttack)
			? GetPlannedNormalAttackData()
			: nullptr;
	const float CurrentNormalAttackPlayRate = CurrentNormalAttackData
		? FMath::Max(CurrentNormalAttackData->PlayRate, KINDA_SMALL_NUMBER)
		: 1.f;

	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();

	if (PlannedAttackType == EWukongPlannedAttackType::NormalAttack && AttackSelection.Montage != nullptr)
	{
		CurrentAttackActionType = EWukongActionType::NormalAttack;
		CurrentAttackNormalAttackType = PlannedNormalAttackType;
		LastStartedNormalAttackType = CurrentAttackNormalAttackType;
		InitiativeState = EWukongInitiativeState::BossPattern;
		if (IsBowNormalAttackType(CurrentAttackNormalAttackType))
		{
			BeginBowAttackPresentation();
			bPostBowCloseRangeApproachInProgress = bForceApproachAfterBowAttack;
			PostBowCloseRangeForceElapsedTime = 0.f;
		}
		else
		{
			bPostBowCloseRangeApproachInProgress = false;
		}

		if (CurrentNormalAttackData && TryStartVariantNormalAttack(CurrentAttackNormalAttackType, *CurrentNormalAttackData))
		{
			return;
		}
	}
	else if (AttackSelection.Montage == GetPlannedComboMontage())
	{
		CurrentAttackActionType = EWukongActionType::ComboAttack;
		CurrentAttackComboSet = PlannedComboSet;
		CurrentAttackComboLength = 1;
		PlannedComboLength = 1;
		InitiativeState = EWukongInitiativeState::BossPattern;
		bPostBowCloseRangeApproachInProgress = false;
	}

	const bool bUseNormalAttackAnimNotifyTiming = CurrentNormalAttackData && CurrentNormalAttackData->bUseAnimNotifyTiming;
	const FWukongComboAttackData* CurrentComboAttackData =
		(CurrentAttackActionType == EWukongActionType::ComboAttack)
			? GetComboAttackData(CurrentAttackComboSet)
			: nullptr;
	const bool bUseComboAttackAnimNotifyTiming = CurrentComboAttackData && CurrentComboAttackData->bUseAnimNotifyTiming;

	if (!bUseNormalAttackAnimNotifyTiming &&
		CurrentAttackActionType == EWukongActionType::NormalAttack &&
		CurrentNormalAttackData &&
		CurrentNormalAttackData->MoveSpeed > 0.f)
	{
		const float MoveStartTime = CurrentNormalAttackData->MoveStartTimeAtPlayRateOne;
		QueueCodeDrivenAttackMove(CurrentAttackNormalAttackType, MoveStartTime / CurrentNormalAttackPlayRate);
	}

	const float AttackMontageDuration = AttackSelection.Montage
		? AttackSelection.Montage->GetPlayLength() / FMath::Max(AttackSelection.PlayRate, KINDA_SMALL_NUMBER)
		: 0.f;
	if (!bUseNormalAttackAnimNotifyTiming && !bUseComboAttackAnimNotifyTiming)
	{
		ApplyTimedMontageSuperArmor(AttackMontageDuration);
	}

	TryAttack();

	if (CurrentAttackActionType == EWukongActionType::ComboAttack)
	{
		ConfigurePlannedComboMontageSections();
	}
}

void AWukongBoss::EnterRecoveryState()
{
	ResetReactiveEvadePressure();
	ClearTimedMontageSuperArmor();
	ResetActiveReactiveActionState();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	StopAIMovement();
	RestoreAttackGroundMotionOverride();

	if (!bAttackInterruptedByHitReact && CurrentAttackActionType != EWukongActionType::None)
	{
		RecordAction(CurrentAttackActionType, CurrentAttackComboSet, CurrentAttackComboLength, CurrentAttackNormalAttackType);
	}

	ResetPlannedCombatPlan();

	ResetCurrentAttackRuntimeState();
	bAttackInterruptedByHitReact = false;
}

void AWukongBoss::EnterParrySuccessState()
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

void AWukongBoss::EnterRepositionState()
{
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (PlannedActionType == EWukongPlannedActionType::NonAttack &&
		PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe &&
		PlannedMovementDirection == EWukongMovementDirection::None)
	{
		if (CurrentTarget && GetTargetDistance2D() < StrafeMinDistanceToTarget)
		{
			PlannedMovementDirection = EWukongMovementDirection::Backward;
		}
		else
		{
			PlannedMovementDirection = FMath::RandBool() ? EWukongMovementDirection::Left : EWukongMovementDirection::Right;
		}
	}

	if (PlannedActionType == EWukongPlannedActionType::NonAttack &&
		PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe &&
		PlannedMovementDuration <= 0.f)
	{
		PlannedMovementDuration = StrafeMinDuration;
	}

	if (PlannedActionType == EWukongPlannedActionType::NonAttack &&
		PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe)
	{
		switch (PlannedMovementDirection)
		{
		case EWukongMovementDirection::Left:
			CurrentRepositionDirection = -1.f;
			break;
		case EWukongMovementDirection::Backward:
			CurrentRepositionDirection = 0.f;
			break;
		case EWukongMovementDirection::Right:
		default:
			CurrentRepositionDirection = 1.f;
			break;
		}
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::Attack &&
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

void AWukongBoss::EnterNonAttackActionState()
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
		PlannedMovementDuration = NonAttackMontage->GetPlayLength();
	}
	else if (PlannedMovementDuration <= 0.f)
	{
		PlannedMovementDuration = NoAttackHoldDuration;
	}
}

void AWukongBoss::EnterEvadeState()
{
	ResetReactiveEvadePressure();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;
	ClearTimedMontageSuperArmor();
	const bool bUsingReactiveEvade = ActiveReactiveActionData.Type != EWukongReactiveActionType::None;

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
				ActiveReactiveActionData.Duration = PlayedMontageLength;
			}
			else
			{
				PlannedMovementDuration = PlayedMontageLength;
			}
		}
		else
		{
			ApplyTimedMontageSuperArmor(NonAttackMontage->GetPlayLength(), 1.f);
			if (bUsingReactiveEvade)
			{
				ActiveReactiveActionData.Duration = NonAttackMontage->GetPlayLength();
			}
			else
			{
				PlannedMovementDuration = NonAttackMontage->GetPlayLength();
			}
		}
	}
	else if (bUsingReactiveEvade)
	{
		if (ActiveReactiveActionData.Duration <= 0.f)
		{
			ActiveReactiveActionData.Duration = EvadeFallbackDuration;
		}
	}
	else if (PlannedMovementDuration <= 0.f)
	{
		PlannedMovementDuration = EvadeFallbackDuration;
	}
}

void AWukongBoss::EnterTurnState()
{
	bRunLocomotionRequested = false;
	TryStartCombatTurnWithThreshold(TurnStartAngle);
}

void AWukongBoss::UpdateApproachState(float DeltaTime)
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
			SetWukongCombatState(EWukongCombatState::Idle);
			return;
		}

		if (TryStartOrWaitForWukongTurn(MoveTurnStartAngle))
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
			SetWukongCombatState(EWukongCombatState::Idle);
		}

		return;
	}

	if (PlannedActionType != EWukongPlannedActionType::Attack || !HasValidPlannedAttack())
	{
		SetWukongCombatState(EWukongCombatState::Idle);
		return;
	}

	if (TryStartOrWaitForWukongTurn(MoveTurnStartAngle))
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
		SetWukongCombatState(EWukongCombatState::Reposition);
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
				SetWukongCombatState(EWukongCombatState::Attack);
			}
			else
			{
				SetWukongCombatState(EWukongCombatState::Idle);
			}
			return;
		}

		SetWukongCombatState(EWukongCombatState::Reposition);
		return;
	}

	if (CurrentTarget)
	{
		MoveToTarget(CurrentTarget);
	}
}

void AWukongBoss::UpdateIdleState(float DeltaTime)
{
	if (ParrySuccessExitActionLockRemainTime > 0.f)
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
		TryStartOrWaitForWukongTurn(TurnStartAngle);
		return;
	}

	if (TryEnterReactiveStateFromIdle())
	{
		return;
	}

	EnsureCombatPlan();
	TryEnterPlannedStateFromIdle();
}

void AWukongBoss::UpdateAttackState(float DeltaTime)
{
	if (!bIsAttacking)
	{
		if (InitiativeState == EWukongInitiativeState::PlayerPressure)
		{
			PlayerPressureRemainTime = FMath::Max(PlayerPressureRemainTime, PlayerPressureHoldDuration);
		}

		if (InitiativeState == EWukongInitiativeState::BossPattern ||
			InitiativeState == EWukongInitiativeState::BossParryCounter)
		{
			InitiativeState = EWukongInitiativeState::Neutral;
		}

		ResetPendingAttackMotionState();
		RestoreAttackGroundMotionOverride();
		SetWukongCombatState(EWukongCombatState::Recovery);
	}
}

void AWukongBoss::UpdateHitState(float DeltaTime)
{
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (!IsInHitReact())
	{
		SetWukongCombatState(EWukongCombatState::Idle);
	}
}

bool AWukongBoss::IsPlayerPressureActive() const
{
	return InitiativeState == EWukongInitiativeState::PlayerPressure && PlayerPressureRemainTime > 0.f;
}

void AWukongBoss::StartPlayerPressure()
{
	InitiativeState = EWukongInitiativeState::PlayerPressure;
	PlayerPressureRemainTime = FMath::Max(PlayerPressureRemainTime, PlayerPressureHoldDuration);
}

void AWukongBoss::ClearPlayerPressure()
{
	InitiativeState = EWukongInitiativeState::Neutral;
	PlayerPressureRemainTime = 0.f;
}

void AWukongBoss::UpdatePlayerPressure(float DeltaTime)
{
	if (InitiativeState != EWukongInitiativeState::PlayerPressure || PlayerPressureRemainTime <= 0.f)
	{
		return;
	}

	if (bIsAttacking ||
		IsInHitReact() ||
		CurrentCombatSubState == EWukongCombatState::ParrySuccess)
	{
		return;
	}

	PlayerPressureRemainTime = FMath::Max(0.f, PlayerPressureRemainTime - DeltaTime);
	if (PlayerPressureRemainTime <= 0.f)
	{
		InitiativeState = EWukongInitiativeState::Neutral;
	}
}

void AWukongBoss::UpdateParrySuccessState(float DeltaTime)
{
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (CombatSubStateElapsedTime <= ParrySuccessFacingDuration)
	{
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, ParrySuccessFacingInterpSpeed);
	}

	if (ParrySuccessFallbackRemainTime > 0.f)
	{
		ParrySuccessFallbackRemainTime = FMath::Max(0.f, ParrySuccessFallbackRemainTime - DeltaTime);
		if (ParrySuccessFallbackRemainTime <= 0.f)
		{
			FinishParrySuccessState();
		}
	}
}

void AWukongBoss::UpdateRecoveryState(float DeltaTime)
{
	if (CombatSubStateElapsedTime < RecoveryDuration)
	{
		return;
	}

	SetWukongCombatState(EWukongCombatState::Idle);
}

void AWukongBoss::UpdateRepositionState(float DeltaTime)
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

	if (TryStartOrWaitForWukongTurn(MoveTurnStartAngle))
	{
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::Attack && IsTargetTooFarForPlannedAttack())
	{
		SetWukongCombatState(EWukongCombatState::Approach);
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::NonAttack &&
		PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe)
	{
		UpdateFacingTowardsTargetWithoutTurn(DeltaTime, CombatFacingInterpSpeed);
		if (CurrentTarget && GetTargetDistance2D() < StrafeMinDistanceToTarget)
		{
			if (DashBackwardMontage && CanTriggerCloseRangeBackDash() && CanStartMobilityMontageSafely())
			{
				StartCloseRangeBackDashCooldown();
				PlannedNonAttackType = EWukongPlannedNonAttackType::Dash;
				PlannedMovementDirection = EWukongMovementDirection::Backward;
				SetWukongCombatState(EWukongCombatState::Evade);
				return;
			}

			PlannedMovementDirection = EWukongMovementDirection::Backward;
		}

		if (PlannedMovementDirection == EWukongMovementDirection::Backward)
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

		if (CombatSubStateElapsedTime >= PlannedMovementDuration)
		{
			CombatMoveInput = FVector2D::ZeroVector;
			SetDesiredMoveAxes(0.f, 0.f);
			LastCompletedNonAttackType = EWukongPlannedNonAttackType::Strafe;
			LastCompletedStrafeDirection = PlannedMovementDirection;
			ResetPlannedCombatPlan();
			SetWukongCombatState(EWukongCombatState::Idle);
		}

		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::Attack && IsTargetTooCloseForPlannedAttack())
	{
		ResetPlannedCombatPlan();
		SetWukongCombatState(EWukongCombatState::Idle);
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::Attack)
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
			SetWukongCombatState(EWukongCombatState::Attack);
			return;
		}

		SetWukongCombatState(EWukongCombatState::Idle);
		return;
	}

	SetDesiredMoveAxes(0.f, 0.f);
	SetWukongCombatState(EWukongCombatState::Idle);
}

void AWukongBoss::UpdateNonAttackActionState(float DeltaTime)
{
	if (PlannedNonAttackType == EWukongPlannedNonAttackType::Hold)
	{
		BeginNoAttackCandidateApproach();
		SetWukongCombatState(EWukongCombatState::Approach);
		return;
	}

	StopAIMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (CombatSubStateElapsedTime < PlannedMovementDuration)
	{
		return;
	}

	LastCompletedNonAttackType = PlannedNonAttackType;
	ResetPlannedCombatPlan();
	SetWukongCombatState(EWukongCombatState::Idle);
}

void AWukongBoss::UpdateEvadeState(float DeltaTime)
{
	const bool bUsingReactiveEvade = ActiveReactiveActionData.Type != EWukongReactiveActionType::None;
	const EWukongPlannedNonAttackType ResolvedEvadeType =
		bUsingReactiveEvade ? ActiveReactiveActionData.NonAttackType : PlannedNonAttackType;
	const float ResolvedEvadeDuration =
		bUsingReactiveEvade ? ActiveReactiveActionData.Duration : PlannedMovementDuration;

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
	case EWukongPlannedNonAttackType::Dash:
		RecordAction(EWukongActionType::Dash);
		break;
	case EWukongPlannedNonAttackType::Dodge:
		RecordAction(EWukongActionType::Dodge);
		break;
	default:
		break;
	}

	if (bUsingReactiveEvade)
	{
		ResetActiveReactiveActionState();
	}

	ResetPlannedCombatPlan();

	SetWukongCombatState(EWukongCombatState::Idle);
}

void AWukongBoss::UpdateTurnState(float DeltaTime)
{
	if (!IsCombatTurnPlaying())
	{
		SetWukongCombatState(EWukongCombatState::Idle);
		return;
	}

	if (bEarlyBlendOutCombatTurnOnTargetYaw &&
		FMath::Abs(GetCombatTargetYawDelta()) <= CombatTurnEarlyBlendOutYawTolerance)
	{
		FinishCombatTurnEarly(CombatTurnEarlyBlendOutTime);
		SetWukongCombatState(EWukongCombatState::Idle);
	}
}

bool AWukongBoss::HasAnyAttackCandidateForCurrentDistance() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	const float TargetDistance = GetTargetDistance2D();
	auto IsWithinRange = [TargetDistance](float MinRange, float MaxRange)
	{
		return TargetDistance >= MinRange && TargetDistance <= MaxRange;
	};

	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
		if (IsBowNormalAttackType(NormalAttackType) && ShouldSuppressBowAttackCandidate())
		{
			continue;
		}

		if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FWukongNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (NormalAttackData &&
			NormalAttackData->Montage &&
			IsWithinRange(NormalAttackData->MinRange, NormalAttackData->CandidateMaxRange))
		{
			return true;
		}
	}

	return false;
}

bool AWukongBoss::CanStartWukongTurn() const
{
	if (!CanStartCombatTurn())
	{
		return false;
	}

	return GetTargetYawAbsDelta() >= TurnStartAngle;
}

bool AWukongBoss::TryStartOrWaitForWukongTurn(float YawThreshold)
{
	if (GetTargetYawAbsDelta() < YawThreshold)
	{
		return false;
	}

	if (CanStartWukongTurn())
	{
		SetWukongCombatState(EWukongCombatState::Turn);
		return true;
	}

	// 큰 각도에서는 코드 Facing으로 억지 정렬하지 않고, 이동 관성이 빠질 때까지 Turn 대기한다.
	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;
	return true;
}

bool AWukongBoss::ShouldTurnAfterHitReact(ECharacterHitReactDirection HitDirection) const
{
	if (!CurrentTarget || GetTargetYawAbsDelta() < TurnStartAngle)
	{
		return false;
	}

	return HitDirection == ECharacterHitReactDirection::Left_L ||
		HitDirection == ECharacterHitReactDirection::Right_R ||
		HitDirection == ECharacterHitReactDirection::Back_B;
}

bool AWukongBoss::TryStartHitReactTurnTransition(bool bStopHitReactMontage)
{
	if (CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::Hit ||
		!IsInHitReact() ||
		!ShouldTurnAfterHitReact(LastHitReactDirectionForTurn))
	{
		return false;
	}

	if (bStopHitReactMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage())
			{
				AnimInstance->Montage_Stop(HitReactTurnBlendOutTime, ActiveMontage);
			}
		}
	}

	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	bHitReactTurnTransitionInProgress = true;
	EndHitReact();
	bHitReactTurnTransitionInProgress = false;

	SetWukongCombatState(EWukongCombatState::Turn);
	return true;
}

bool AWukongBoss::HasValidPlannedAttack() const
{
	switch (PlannedAttackType)
	{
	case EWukongPlannedAttackType::ComboAttack:
		return GetPlannedComboMontage() != nullptr;
	case EWukongPlannedAttackType::NormalAttack:
		{
			const FWukongNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData && NormalAttackData->Montage != nullptr;
		}
	case EWukongPlannedAttackType::None:
	default:
		return false;
	}
}

bool AWukongBoss::HasValidPlannedMovement() const
{
	if (PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe)
	{
		return true;
	}

	if (PlannedNonAttackType == EWukongPlannedNonAttackType::Hold)
	{
		return false;
	}

	if (IsMobilityNonAttackType(PlannedNonAttackType))
	{
		return GetPlannedNonAttackMontage() != nullptr;
	}

	return false;
}

bool AWukongBoss::IsMobilityNonAttackType(EWukongPlannedNonAttackType NonAttackType) const
{
	return NonAttackType == EWukongPlannedNonAttackType::Strafe ||
		NonAttackType == EWukongPlannedNonAttackType::Dash ||
		NonAttackType == EWukongPlannedNonAttackType::Dodge;
}

bool AWukongBoss::IsComboSetEnabledByTestFilter(EWukongComboSet ComboSet) const
{
	if (!bUseAttackPatternTestFilter)
	{
		return IsComboSetAllowedInCurrentPhase(ComboSet);
	}

	bool bEnabled = false;
	switch (ComboSet)
	{
	case EWukongComboSet::ComboA:
		bEnabled = bTestEnableComboA;
		break;
	case EWukongComboSet::ComboB:
		bEnabled = bTestEnableComboB;
		break;
	case EWukongComboSet::None:
	default:
		return false;
	}

	return bEnabled && IsComboSetAllowedInCurrentPhase(ComboSet);
}

bool AWukongBoss::IsNormalAttackEnabledByTestFilter(EWukongNormalAttackType NormalAttackType) const
{
	if (!bUseAttackPatternTestFilter)
	{
		return IsNormalAttackAllowedInCurrentPhase(NormalAttackType);
	}

	bool bEnabled = false;
	switch (NormalAttackType)
	{
	case EWukongNormalAttackType::JumpAttack:
		bEnabled = bTestEnableJumpAttack;
		break;
	case EWukongNormalAttackType::ChargeAttack:
		bEnabled = bTestEnableChargeAttack;
		break;
	case EWukongNormalAttackType::DodgeAttack:
		bEnabled = bTestEnableDodgeAttack;
		break;
	case EWukongNormalAttackType::NinjaA:
		bEnabled = bTestEnableNinjaA;
		break;
	case EWukongNormalAttackType::NinjaB:
		bEnabled = bTestEnableNinjaB;
		break;
	case EWukongNormalAttackType::Execution:
		bEnabled = bTestEnableExecution;
		break;
	case EWukongNormalAttackType::FastComeSlash:
		bEnabled = bTestEnableFastComeSlash;
		break;
	case EWukongNormalAttackType::SlowComeSlash:
		bEnabled = bTestEnableSlowComeSlash;
		break;
	case EWukongNormalAttackType::FakeDownSlash:
		bEnabled = bTestEnableFakeDownSlash;
		break;
	case EWukongNormalAttackType::LionSlash:
		bEnabled = bTestEnableLionSlash;
		break;
	case EWukongNormalAttackType::SpinSlash:
		bEnabled = bTestEnableSpinSlash;
		break;
	case EWukongNormalAttackType::LightningSword:
		bEnabled = bTestEnableLightningSword;
		break;
	case EWukongNormalAttackType::Bow4Combo:
		bEnabled = bTestEnableBow4Combo;
		break;
	case EWukongNormalAttackType::BowBackDashAttack:
		bEnabled = bTestEnableBowBackDashAttack;
		break;
	case EWukongNormalAttackType::BowHeavyAttack:
		bEnabled = bTestEnableBowHeavyAttack;
		break;
	case EWukongNormalAttackType::None:
	default:
		return false;
	}

	return bEnabled && IsNormalAttackAllowedInCurrentPhase(NormalAttackType);
}

bool AWukongBoss::IsPhaseTwoActive() const
{
	return GetCurrentLifeCount() < GetMaxLifeCount();
}

bool AWukongBoss::IsNormalAttackAllowedByCurrentPhase(EWukongNormalAttackType NormalAttackType) const
{
	return IsNormalAttackAllowedInCurrentPhase(NormalAttackType);
}

bool AWukongBoss::IsComboSetAllowedInCurrentPhase(EWukongComboSet ComboSet) const
{
	bool bRequiresPhaseTwo = false;
	switch (ComboSet)
	{
	case EWukongComboSet::ComboA:
		bRequiresPhaseTwo = bTestComboARequiresPhaseTwo;
		break;
	case EWukongComboSet::ComboB:
		bRequiresPhaseTwo = bTestComboBRequiresPhaseTwo;
		break;
	case EWukongComboSet::None:
	default:
		return false;
	}

	return !bRequiresPhaseTwo || IsPhaseTwoActive();
}

bool AWukongBoss::IsNormalAttackAllowedInCurrentPhase(EWukongNormalAttackType NormalAttackType) const
{
	bool bRequiresPhaseTwo = false;
	switch (NormalAttackType)
	{
	case EWukongNormalAttackType::JumpAttack:
		bRequiresPhaseTwo = bTestJumpAttackRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::ChargeAttack:
		bRequiresPhaseTwo = bTestChargeAttackRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::DodgeAttack:
		bRequiresPhaseTwo = bTestDodgeAttackRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::NinjaA:
		bRequiresPhaseTwo = bTestNinjaARequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::NinjaB:
		bRequiresPhaseTwo = bTestNinjaBRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::Execution:
		bRequiresPhaseTwo = bTestExecutionRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::FastComeSlash:
		bRequiresPhaseTwo = bTestFastComeSlashRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::SlowComeSlash:
		bRequiresPhaseTwo = bTestSlowComeSlashRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::FakeDownSlash:
		bRequiresPhaseTwo = bTestFakeDownSlashRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::LionSlash:
		bRequiresPhaseTwo = bTestLionSlashRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::SpinSlash:
		bRequiresPhaseTwo = bTestSpinSlashRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::LightningSword:
		bRequiresPhaseTwo = bTestLightningSwordRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::Bow4Combo:
		bRequiresPhaseTwo = bTestBow4ComboRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::BowBackDashAttack:
		bRequiresPhaseTwo = bTestBowBackDashAttackRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::BowHeavyAttack:
		bRequiresPhaseTwo = bTestBowHeavyAttackRequiresPhaseTwo;
		break;
	case EWukongNormalAttackType::None:
	default:
		return false;
	}

	return !bRequiresPhaseTwo || IsPhaseTwoActive();
}

float AWukongBoss::GetMaxEnabledComboCandidateRange() const
{
	float MaxCandidateRange = 0.f;
	if (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboA) && IsComboAttackUsable(ComboAttackA))
	{
		MaxCandidateRange = FMath::Max(MaxCandidateRange, ComboAttackA.CandidateRange);
	}

	if (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboB) && IsComboAttackUsable(ComboAttackB))
	{
		MaxCandidateRange = FMath::Max(MaxCandidateRange, ComboAttackB.CandidateRange);
	}

	return MaxCandidateRange;
}

float AWukongBoss::GetMaxEnabledAttackCandidateRange() const
{
	float MaxCandidateRange = 0.f;
	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
		if (IsBowNormalAttackType(NormalAttackType) && ShouldSuppressBowAttackCandidate())
		{
			continue;
		}

		if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FWukongNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (NormalAttackData && NormalAttackData->Montage)
		{
			MaxCandidateRange = FMath::Max(MaxCandidateRange, NormalAttackData->CandidateMaxRange);
		}
	}

	return MaxCandidateRange;
}

bool AWukongBoss::IsFartherThanAllAttackCandidates() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	const float MaxCandidateRange = GetMaxEnabledAttackCandidateRange();
	return MaxCandidateRange > 0.f && GetTargetDistance2D() > MaxCandidateRange;
}

bool AWukongBoss::HasAnyNonBowAttackCandidateForCurrentDistance() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	const float TargetDistance = GetTargetDistance2D();
	auto IsWithinRange = [TargetDistance](float MinRange, float MaxRange)
	{
		return TargetDistance >= MinRange && TargetDistance <= MaxRange;
	};

	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
		if (IsBowNormalAttackType(NormalAttackType) || !IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FWukongNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (NormalAttackData &&
			NormalAttackData->Montage &&
			IsWithinRange(NormalAttackData->MinRange, NormalAttackData->CandidateMaxRange))
		{
			return true;
		}
	}

	return false;
}

EWukongMovementDirection AWukongBoss::ChooseStrafeDirectionAvoidingImmediateReverse() const
{
	EWukongMovementDirection ChosenDirection = FMath::RandBool()
		? EWukongMovementDirection::Left
		: EWukongMovementDirection::Right;

	if (LastCompletedNonAttackType == EWukongPlannedNonAttackType::Strafe &&
		IsOppositeStrafeDirection(ChosenDirection, LastCompletedStrafeDirection))
	{
		ChosenDirection = LastCompletedStrafeDirection;
	}

	return ChosenDirection;
}

bool AWukongBoss::IsOppositeStrafeDirection(EWukongMovementDirection A, EWukongMovementDirection B) const
{
	return (A == EWukongMovementDirection::Left && B == EWukongMovementDirection::Right) ||
		(A == EWukongMovementDirection::Right && B == EWukongMovementDirection::Left);
}

void AWukongBoss::UpdatePlannedCombatPlanAge(float DeltaTime)
{
	if (bPostBowCloseRangeApproachInProgress)
	{
		PostBowCloseRangeForceElapsedTime += DeltaTime;
		if (PostBowCloseRangeForceMaxTime > 0.f &&
			PostBowCloseRangeForceElapsedTime >= PostBowCloseRangeForceMaxTime)
		{
			bPostBowCloseRangeApproachInProgress = false;
			PostBowCloseRangeForceElapsedTime = 0.f;
			if (bNoAttackCandidateApproachInProgress)
			{
				ClearNoAttackCandidateApproach();
			}
			ResetPlannedCombatPlan();
		}
	}
	else
	{
		PostBowCloseRangeForceElapsedTime = 0.f;
	}

	if (PlannedActionType == EWukongPlannedActionType::Attack &&
		CurrentCombatSubState != EWukongCombatState::Attack)
	{
		PlannedAttackWaitTime += DeltaTime;
		if (IsPlannedAttackExpired())
		{
			ResetPlannedCombatPlan();
		}
	}
	else if (CurrentCombatSubState == EWukongCombatState::Attack ||
		PlannedActionType != EWukongPlannedActionType::Attack)
	{
		PlannedAttackWaitTime = 0.f;
	}
}

bool AWukongBoss::IsPlannedAttackExpired() const
{
	return PlannedAttackMaxWaitTime > 0.f &&
		PlannedAttackWaitTime >= PlannedAttackMaxWaitTime;
}

bool AWukongBoss::ShouldForceCloseAfterBowAttack() const
{
	return bForceApproachAfterBowAttack &&
		bPostBowCloseRangeApproachInProgress &&
		(PostBowCloseRangeForceMaxTime <= 0.f ||
			PostBowCloseRangeForceElapsedTime < PostBowCloseRangeForceMaxTime);
}

bool AWukongBoss::ShouldSuppressBowAttackCandidate() const
{
	return ShouldForceCloseAfterBowAttack();
}

void AWukongBoss::CollectNormalAttackCandidates(float TargetDistance, bool bIgnoreRepeat, TArray<EWukongNormalAttackType>& OutCandidates) const
{
	auto IsWithinRange = [TargetDistance](float MinRange, float MaxRange)
	{
		return TargetDistance >= MinRange && TargetDistance <= MaxRange;
	};

	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
		const bool bIsBowAttack = IsBowNormalAttackType(NormalAttackType);
		if (bIsBowAttack && ShouldSuppressBowAttackCandidate())
		{
			continue;
		}

		if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FWukongNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (!NormalAttackData || !NormalAttackData->Montage)
		{
			continue;
		}

		if (!IsWithinRange(NormalAttackData->MinRange, NormalAttackData->CandidateMaxRange))
		{
			continue;
		}

		if (!bIgnoreRepeat && WasRecentlyUsed(NormalAttackType, 1))
		{
			continue;
		}

		if (!bIgnoreRepeat && NormalAttackType == LastStartedNormalAttackType)
		{
			continue;
		}

		for (int32 WeightIndex = 0; WeightIndex < FMath::Max(1, NormalAttackData->SelectionWeight); ++WeightIndex)
		{
			OutCandidates.Add(NormalAttackType);
		}
	}
}

int32 AWukongBoss::GetComboAttackSelectionWeight() const
{
	int32 ComboAttackSelectionWeight = 0;
	if (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboA) && IsComboAttackUsable(ComboAttackA))
	{
		ComboAttackSelectionWeight += FMath::Max(1, ComboAttackA.SelectionWeight);
	}

	if (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboB) && IsComboAttackUsable(ComboAttackB))
	{
		ComboAttackSelectionWeight += FMath::Max(1, ComboAttackB.SelectionWeight);
	}

	return ComboAttackSelectionWeight;
}

bool AWukongBoss::HasAnyComboMontage() const
{
	return (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboA) && IsComboAttackUsable(ComboAttackA)) ||
		(IsComboSetEnabledByTestFilter(EWukongComboSet::ComboB) && IsComboAttackUsable(ComboAttackB));
}

const FWukongComboAttackData* AWukongBoss::GetComboAttackData(EWukongComboSet ComboSet) const
{
	switch (ComboSet)
	{
	case EWukongComboSet::ComboA:
		return &ComboAttackA;
	case EWukongComboSet::ComboB:
		return &ComboAttackB;
	case EWukongComboSet::None:
	default:
		return nullptr;
	}
}

bool AWukongBoss::IsComboAttackUsable(const FWukongComboAttackData& ComboAttackData) const
{
	return ComboAttackData.Montage != nullptr && GetComboAttackMaxLength(ComboAttackData) >= 2;
}

int32 AWukongBoss::GetComboAttackMaxLength(const FWukongComboAttackData& ComboAttackData) const
{
	return FMath::Max(0, ComboAttackData.MaxComboLength);
}

FName AWukongBoss::GetComboSectionName(int32 ComboLength) const
{
	return FName(*FString::FromInt(ComboLength));
}

const FWukongReactiveActionTuningData* AWukongBoss::GetReactiveActionTuningData(EWukongReactiveActionType ReactiveActionType) const
{
	switch (ReactiveActionType)
	{
	case EWukongReactiveActionType::EvadeBackward:
		return &ReactiveBackwardEvade;
	case EWukongReactiveActionType::None:
	default:
		return nullptr;
	}
}

bool AWukongBoss::IsWithinSelectionRange(const FMonsterAttackSelection& Selection) const
{
	if (!CurrentTarget || !Selection.Montage)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return DistSq <= FMath::Square(Selection.AttackRange);
}

const FWukongNormalAttackData* AWukongBoss::GetNormalAttackData(EWukongNormalAttackType AttackType) const
{
	switch (AttackType)
	{
	case EWukongNormalAttackType::JumpAttack:
		return &JumpAttack;
	case EWukongNormalAttackType::ChargeAttack:
		return &ChargeAttack;
	case EWukongNormalAttackType::DodgeAttack:
		return &DodgeAttack;
	case EWukongNormalAttackType::NinjaA:
		return &NinjaAAttack;
	case EWukongNormalAttackType::NinjaB:
		return &NinjaBAttack;
	case EWukongNormalAttackType::Execution:
		return &ExecutionAttack;
	case EWukongNormalAttackType::FastComeSlash:
		return &FastComeSlashAttack;
	case EWukongNormalAttackType::SlowComeSlash:
		return &SlowComeSlashAttack;
	case EWukongNormalAttackType::FakeDownSlash:
		return &FakeDownSlashAttack;
	case EWukongNormalAttackType::LionSlash:
		return &LionSlashAttack;
	case EWukongNormalAttackType::SpinSlash:
		return &SpinSlashAttack;
	case EWukongNormalAttackType::LightningSword:
		return &LightningSwordAttack;
	case EWukongNormalAttackType::Bow4Combo:
		return &Bow4ComboAttack;
	case EWukongNormalAttackType::BowBackDashAttack:
		return &BowBackDashAttack;
	case EWukongNormalAttackType::BowHeavyAttack:
		return &BowHeavyAttack;
	case EWukongNormalAttackType::None:
	default:
		return nullptr;
	}
}

const FWukongNormalAttackData* AWukongBoss::GetPlannedNormalAttackData() const
{
	return GetNormalAttackData(PlannedNormalAttackType);
}

UAnimMontage* AWukongBoss::GetPlannedComboMontage() const
{
	const FWukongComboAttackData* ComboAttackData = GetComboAttackData(PlannedComboSet);

	if (!ComboAttackData || !ComboAttackData->Montage || PlannedComboLength <= 0)
	{
		return nullptr;
	}

	return PlannedComboLength <= GetComboAttackMaxLength(*ComboAttackData)
		? ComboAttackData->Montage.Get()
		: nullptr;
}

float AWukongBoss::GetPlannedComboFacingDuration() const
{
	const FWukongComboAttackData* ComboAttackData = GetComboAttackData(PlannedComboSet);
	const TArray<float>* FacingDurations = ComboAttackData ? &ComboAttackData->FacingDurations : nullptr;

	if (!FacingDurations || PlannedComboLength <= 0)
	{
		return -1.f;
	}

	const int32 DurationIndex = PlannedComboLength - 1;
	return FacingDurations->IsValidIndex(DurationIndex)
		? (*FacingDurations)[DurationIndex]
		: -1.f;
}

void AWukongBoss::ConfigurePlannedComboMontageSections()
{
	if (PlannedAttackType != EWukongPlannedAttackType::ComboAttack)
	{
		return;
	}

	const FWukongComboAttackData* ComboAttackData = GetComboAttackData(PlannedComboSet);
	UAnimMontage* ComboMontage = GetPlannedComboMontage();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!ComboAttackData || !ComboMontage || !AnimInstance)
	{
		return;
	}

	const FName FirstSectionName = GetComboSectionName(1);
	if (ComboMontage->GetSectionIndex(FirstSectionName) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(FirstSectionName, ComboMontage);
	}
}

bool AWukongBoss::ShouldContinueComboAttack(const FWukongComboAttackData& ComboAttackData, int32 CurrentComboLength) const
{
	const int32 MaxComboLength = GetComboAttackMaxLength(ComboAttackData);
	if (CurrentComboLength >= MaxComboLength)
	{
		return false;
	}

	if (!CurrentTarget)
	{
		return false;
	}

	if (GetTargetDistance2D() > ComboAttackData.CandidateRange)
	{
		return false;
	}

	return FMath::FRand() <= FMath::Clamp(ComboAttackData.ContinueChance, 0.f, 1.f);
}

void AWukongBoss::HandleComboDecisionPoint()
{
	if (CurrentAttackActionType != EWukongActionType::ComboAttack)
	{
		return;
	}

	const FWukongComboAttackData* ComboAttackData = GetComboAttackData(CurrentAttackComboSet);
	UAnimMontage* ComboMontage = ComboAttackData ? ComboAttackData->Montage.Get() : nullptr;
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!ComboAttackData || !ComboMontage || !AnimInstance)
	{
		return;
	}

	const int32 CurrentComboLength = FMath::Max(1, CurrentAttackComboLength);
	const FName CurrentSectionName = GetComboSectionName(CurrentComboLength);
	if (ComboMontage->GetSectionIndex(CurrentSectionName) == INDEX_NONE)
	{
		return;
	}

	if (!ShouldContinueComboAttack(*ComboAttackData, CurrentComboLength))
	{
		return;
	}

	const int32 NextComboLength = CurrentComboLength + 1;
	const FName NextSectionName = GetComboSectionName(NextComboLength);
	if (ComboMontage->GetSectionIndex(NextSectionName) == INDEX_NONE)
	{
		return;
	}

	CurrentAttackComboLength = NextComboLength;
	PlannedComboLength = NextComboLength;
	AnimInstance->Montage_JumpToSection(NextSectionName, ComboMontage);
}

bool AWukongBoss::TryStartNormalAttackLinkFromNotify(
	EWukongNormalAttackType NextAttackType,
	float TriggerChance,
	float BlendOutTime,
	float BlendInTime,
	bool bRequireRange,
	bool bMoveToRangeWhenOutOfRange,
	bool bDelayMoveToRangeWhenOutOfRange,
	bool bUseTestFilter)
{
	if (bExecutionReady || bBeingExecuted)
	{
		return false;
	}

	if (TriggerChance <= 0.f || FMath::FRand() > FMath::Clamp(TriggerChance, 0.f, 1.f))
	{
		return false;
	}

	if (!CurrentTarget)
	{
		return false;
	}

	if (const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(CurrentTarget))
	{
		if (TargetCharacter->Is_Dead())
		{
			return false;
		}
	}

	if (const AJunPlayer* TargetPlayer = Cast<AJunPlayer>(CurrentTarget))
	{
		if (TargetPlayer->IsInDeathSequence())
		{
			return false;
		}
	}

	const bool bCanLinkFromCurrentState =
		CurrentCombatSubState == EWukongCombatState::Attack ||
		CurrentCombatSubState == EWukongCombatState::Recovery ||
		CurrentCombatSubState == EWukongCombatState::ParrySuccess;
	const bool bLinkingFromParrySuccess = CurrentCombatSubState == EWukongCombatState::ParrySuccess;
	const bool bNormalParrySuccessLinkAllowed =
		!bLinkingFromParrySuccess ||
		(bParryCounterStarted && bCurrentParryCounterPerfectParried && !bParryCounterFollowUpStarted);

	if (CurrentState != EMonsterState::Combat ||
		!bCanLinkFromCurrentState ||
		!bNormalParrySuccessLinkAllowed ||
		IsInHitReact() ||
		NextAttackType == EWukongNormalAttackType::None ||
		!IsNormalAttackAllowedInCurrentPhase(NextAttackType) ||
		(bUseTestFilter && !IsNormalAttackEnabledByTestFilter(NextAttackType)))
	{
		return false;
	}

	const FWukongNormalAttackData* NextAttackData = GetNormalAttackData(NextAttackType);
	UAnimMontage* NextMontage = NextAttackData ? NextAttackData->Montage.Get() : nullptr;
	UAnimInstance* CurrentAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!NextAttackData || !NextMontage || !CurrentAnimInstance)
	{
		return false;
	}

	const float TargetDistance = GetTargetDistance2D();
	if (bRequireRange &&
		(TargetDistance < NextAttackData->MinRange || TargetDistance > NextAttackData->MaxRange))
	{
		if (bMoveToRangeWhenOutOfRange)
		{
			if (bDelayMoveToRangeWhenOutOfRange)
			{
				bHasDelayedNormalAttackRangeLink = true;
				DelayedNormalAttackRangeLinkType = NextAttackType;
				DelayedNormalAttackRangeLinkBlendOutTime = FMath::Max(0.f, BlendOutTime);
				DelayedNormalAttackRangeLinkBlendInTime = FMath::Max(0.f, BlendInTime);
				bDelayedNormalAttackRangeLinkUseTestFilter = bUseTestFilter;
				return true;
			}

			if (bLinkingFromParrySuccess)
			{
				CurrentAnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
				if (CurrentParrySuccessMontage)
				{
					CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, BlendOutTime), CurrentParrySuccessMontage);
				}
				CurrentParrySuccessMontage = nullptr;
				ParrySuccessFallbackRemainTime = 0.f;
			}

			if (CurrentAttackMontage)
			{
				CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, BlendOutTime), CurrentAttackMontage);
			}

			if (!bAttackInterruptedByHitReact && CurrentAttackActionType != EWukongActionType::None)
			{
				RecordAction(CurrentAttackActionType, CurrentAttackComboSet, CurrentAttackComboLength, CurrentAttackNormalAttackType);
			}

			if (!IsBowNormalAttackType(NextAttackType) && IsBowNormalAttackType(CurrentAttackNormalAttackType))
			{
				EndBowAttackPresentation();
			}

			StopAllAttackTraces();
			ResetPendingAttackMotionState();
			RestoreAttackGroundMotionOverride();
			ClearTimedMontageSuperArmor();
			ResetCurrentAttackRuntimeState();
			bParryCounterStarted = false;
			bCurrentParryCounterPerfectParried = false;
			bParryCounterFollowUpStarted = false;

			bIsAttacking = false;
			bAttackInterruptedByHitReact = false;
			AttackTime = 0.f;
			AttackFacingRemainTime = 0.f;
			CombatMoveInput = FVector2D::ZeroVector;
			SetDesiredMoveAxes(0.f, 0.f);
			RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
			RemoveGameplayTag(JunGameplayTags::State_Block_Move);

			PlannedActionType = EWukongPlannedActionType::Attack;
			PlannedAttackType = EWukongPlannedAttackType::NormalAttack;
			PlannedNormalAttackType = NextAttackType;
			PlannedComboSet = EWukongComboSet::None;
			PlannedComboLength = 0;
			PlannedAttackWaitTime = 0.f;

			SetWukongCombatState(TargetDistance > NextAttackData->MaxRange
				? EWukongCombatState::Approach
				: EWukongCombatState::Reposition);
			return true;
		}

		return false;
	}

	ClearDelayedNormalAttackRangeLink();

	const bool bNextAttackIsBow = IsBowNormalAttackType(NextAttackType);
	if (!bNextAttackIsBow && IsBowNormalAttackType(CurrentAttackNormalAttackType))
	{
		EndBowAttackPresentation();
	}

	if (bLinkingFromParrySuccess)
	{
		CurrentAnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
		if (CurrentParrySuccessMontage)
		{
			CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, BlendOutTime), CurrentParrySuccessMontage);
		}
		CurrentParrySuccessMontage = nullptr;
		ParrySuccessFallbackRemainTime = 0.f;
	}

	if (CurrentAttackMontage)
	{
		CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, BlendOutTime), CurrentAttackMontage);
	}

	if (!bAttackInterruptedByHitReact && CurrentAttackActionType != EWukongActionType::None)
	{
		RecordAction(CurrentAttackActionType, CurrentAttackComboSet, CurrentAttackComboLength, CurrentAttackNormalAttackType);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	bParryCounterStarted = false;
	bCurrentParryCounterPerfectParried = false;
	bParryCounterFollowUpStarted = false;

	CurrentCombatSubState = EWukongCombatState::Attack;
	CombatSubStateElapsedTime = 0.f;
	CurrentAttackActionType = EWukongActionType::NormalAttack;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = NextAttackType;
	LastStartedNormalAttackType = NextAttackType;
	PlannedAttackType = EWukongPlannedAttackType::NormalAttack;
	PlannedNormalAttackType = NextAttackType;

	if (TryStartVariantNormalAttack(NextAttackType, *NextAttackData))
	{
		return true;
	}

	CurrentAttackSelection.Montage = NextMontage;
	CurrentAttackSelection.AttackRange = NextAttackData->MaxRange;
	CurrentAttackSelection.bFaceTargetDuringAttack = NextAttackData->bFaceTargetDuringAttack;
	CurrentAttackSelection.FacingDuration = NextAttackData->bUseAnimNotifyTiming ? 0.f : NextAttackData->FacingDuration;
	CurrentAttackSelection.FacingInterpSpeed = NextAttackData->FacingInterpSpeed;
	CurrentAttackSelection.PlayRate = NextAttackData->PlayRate;
	CurrentAttackSelection.bTryTurnAfterAttack = NextAttackData->bTryTurnAfterAttack;
	CurrentAttackSelection.PostAttackTurnStartAngle = NextAttackData->PostAttackTurnStartAngle;
	CurrentAttackMontage = NextMontage;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	AttackTime = NextMontage->GetPlayLength() / FMath::Max(NextAttackData->PlayRate, KINDA_SMALL_NUMBER);
	AttackFacingRemainTime = CurrentAttackSelection.FacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;
	InitiativeState = EWukongInitiativeState::BossPattern;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (bNextAttackIsBow)
	{
		BeginBowAttackPresentation();
		bPostBowCloseRangeApproachInProgress = bForceApproachAfterBowAttack;
		PostBowCloseRangeForceElapsedTime = 0.f;
	}
	else
	{
		bPostBowCloseRangeApproachInProgress = false;
	}

	if (!NextAttackData->bUseAnimNotifyTiming)
	{
		if (NextAttackData->MoveSpeed > 0.f)
		{
			const float MoveStartTime = NextAttackData->MoveStartTimeAtPlayRateOne;
			QueueCodeDrivenAttackMove(NextAttackType, MoveStartTime / FMath::Max(NextAttackData->PlayRate, KINDA_SMALL_NUMBER));
		}

		ApplyTimedMontageSuperArmor(AttackTime);
	}

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, BlendInTime));
	if (CurrentAnimInstance->Montage_PlayWithBlendSettings(NextMontage, BlendInSettings, NextAttackData->PlayRate) <= 0.f)
	{
		FinishAttack();
		return false;
	}

	return true;
}

bool AWukongBoss::TryExecuteDelayedNormalAttackRangeLinkFromNotify(float BlendOutTimeOverride)
{
	if (bExecutionReady || bBeingExecuted)
	{
		ClearDelayedNormalAttackRangeLink();
		return false;
	}

	if (!bHasDelayedNormalAttackRangeLink ||
		DelayedNormalAttackRangeLinkType == EWukongNormalAttackType::None)
	{
		return false;
	}

	const EWukongNormalAttackType NextAttackType = DelayedNormalAttackRangeLinkType;
	const float BlendOutTime = BlendOutTimeOverride >= 0.f
		? BlendOutTimeOverride
		: DelayedNormalAttackRangeLinkBlendOutTime;
	const float BlendInTime = DelayedNormalAttackRangeLinkBlendInTime;
	const bool bUseTestFilter = bDelayedNormalAttackRangeLinkUseTestFilter;

	ClearDelayedNormalAttackRangeLink();

	return TryStartNormalAttackLinkFromNotify(
		NextAttackType,
		1.f,
		BlendOutTime,
		BlendInTime,
		true,
		true,
		false,
		bUseTestFilter);
}

bool AWukongBoss::TryForceNormalAttackLinkWhenTriggerChanceFails(float BlendOutTime, float BlendInTime)
{
	if (bExecutionReady || bBeingExecuted)
	{
		return false;
	}

	auto CollectRangeIgnoredCandidates = [this](bool bIgnoreRepeat, TArray<EWukongNormalAttackType>& OutCandidates)
	{
		for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
		{
			if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
			{
				continue;
			}

			const FWukongNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
			if (!NormalAttackData || !NormalAttackData->Montage)
			{
				continue;
			}

			if (!bIgnoreRepeat &&
				(WasRecentlyUsed(NormalAttackType, 1) || NormalAttackType == LastStartedNormalAttackType))
			{
				continue;
			}

			for (int32 WeightIndex = 0; WeightIndex < FMath::Max(1, NormalAttackData->SelectionWeight); ++WeightIndex)
			{
				OutCandidates.Add(NormalAttackType);
			}
		}
	};

	TArray<EWukongNormalAttackType> NormalAttackCandidates;
	CollectRangeIgnoredCandidates(false, NormalAttackCandidates);
	if (NormalAttackCandidates.Num() <= 0)
	{
		CollectRangeIgnoredCandidates(true, NormalAttackCandidates);
	}

	if (NormalAttackCandidates.Num() <= 0)
	{
		return false;
	}

	const EWukongNormalAttackType NextNormalAttackType =
		NormalAttackCandidates[FMath::RandRange(0, NormalAttackCandidates.Num() - 1)];

	return TryStartNormalAttackLinkFromNotify(
		NextNormalAttackType,
		1.f,
		BlendOutTime,
		BlendInTime,
		false,
		false,
		false,
		true);
}

bool AWukongBoss::TryStartParryBackJump()
{
	UAnimMontage* EscapeMontage = GetParryExchangeEscapeMontage();
	if (bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::ParrySuccess ||
		bParryCounterStarted ||
		IsInHitReact() ||
		!EscapeMontage)
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);

	if (CurrentParrySuccessMontage)
	{
		AnimInstance->Montage_Stop(FMath::Max(0.f, ParryBackJumpBlendOutTime), CurrentParrySuccessMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ResetCurrentAttackRuntimeState();

	bParryCounterStarted = true;
	bParryCounterFollowUpStarted = true;
	bCurrentParryCounterPerfectParried = false;
	ParrySuccessFallbackRemainTime = 0.f;
	CurrentParrySuccessMontage = EscapeMontage;
	CurrentCombatSubState = EWukongCombatState::ParrySuccess;
	CombatSubStateElapsedTime = 0.f;
	InitiativeState = EWukongInitiativeState::BossParryCounter;

	CombatMoveInput = FVector2D::ZeroVector;
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	AnimInstance->OnMontageEnded.AddDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, ParryBackJumpBlendInTime));
	const float PlayedDuration = AnimInstance->Montage_PlayWithBlendSettings(
		CurrentParrySuccessMontage,
		BlendInSettings,
		FMath::Max(ParryBackJumpPlayRate, KINDA_SMALL_NUMBER));
	if (PlayedDuration <= 0.f)
	{
		FinishParrySuccessState();
		return false;
	}

	ParrySuccessFallbackRemainTime = PlayedDuration;
	return true;
}

UAnimMontage* AWukongBoss::GetMobilityNonAttackMontage(EWukongPlannedNonAttackType NonAttackType, EWukongMovementDirection MovementDirection) const
{
	switch (NonAttackType)
	{
	case EWukongPlannedNonAttackType::Dash:
		switch (MovementDirection)
		{
		case EWukongMovementDirection::Forward:
			return DashForwardMontage.Get();
		case EWukongMovementDirection::Backward:
			return DashBackwardMontage.Get();
		case EWukongMovementDirection::Left:
			return DashLeftMontage.Get();
		case EWukongMovementDirection::Right:
		default:
			return DashRightMontage.Get();
		}
	case EWukongPlannedNonAttackType::Dodge:
		switch (MovementDirection)
		{
		case EWukongMovementDirection::Forward:
			return DodgeForwardMontage.Get();
		case EWukongMovementDirection::Backward:
			return DodgeBackwardMontage.Get();
		case EWukongMovementDirection::Left:
			return DodgeLeftMontage.Get();
		case EWukongMovementDirection::Right:
		default:
			return DodgeRightMontage.Get();
		}
	case EWukongPlannedNonAttackType::Strafe:
	case EWukongPlannedNonAttackType::Hold:
	case EWukongPlannedNonAttackType::None:
	default:
		return nullptr;
	}
}

UAnimMontage* AWukongBoss::GetPlannedNonAttackMontage() const
{
	if (IsMobilityNonAttackType(PlannedNonAttackType))
	{
		return GetMobilityNonAttackMontage(PlannedNonAttackType, PlannedMovementDirection);
	}

	return nullptr;
}

UAnimMontage* AWukongBoss::GetReactiveActionMontage() const
{
	return GetMobilityNonAttackMontage(ActiveReactiveActionData.NonAttackType, ActiveReactiveActionData.MovementDirection);
}

EWukongCombatState AWukongBoss::GetReactiveActionTargetState(EWukongReactiveActionType ReactiveActionType) const
{
	switch (ReactiveActionType)
	{
	case EWukongReactiveActionType::EvadeBackward:
		return EWukongCombatState::Evade;
	case EWukongReactiveActionType::None:
	default:
		return EWukongCombatState::Idle;
	}
}

void AWukongBoss::RecordAction(EWukongActionType ActionType, EWukongComboSet ComboSet, int32 ComboLength, EWukongNormalAttackType NormalAttackType)
{
	FWukongActionRecord Record;
	Record.ActionType = ActionType;
	Record.ComboSet = ComboSet;
	Record.ComboLength = ComboLength;
	Record.NormalAttackType = NormalAttackType;

	RecentActionHistory.Add(Record);

	if (RecentActionHistory.Num() > MaxRecentActionHistory)
	{
		RecentActionHistory.RemoveAt(0, RecentActionHistory.Num() - MaxRecentActionHistory);
	}

	if (bForceApproachAfterBowAttack &&
		ActionType == EWukongActionType::NormalAttack &&
		IsBowNormalAttackType(NormalAttackType))
	{
		bPostBowCloseRangeApproachInProgress = true;
		PostBowCloseRangeForceElapsedTime = 0.f;
	}
}

bool AWukongBoss::WasRecentlyUsed(EWukongActionType ActionType, int32 Depth) const
{
	const int32 HistoryNum = RecentActionHistory.Num();
	const int32 NumToCheck = FMath::Clamp(Depth, 0, HistoryNum);
	for (int32 Offset = 0; Offset < NumToCheck; ++Offset)
	{
		const int32 Index = HistoryNum - 1 - Offset;
		if (RecentActionHistory[Index].ActionType == ActionType)
		{
			return true;
		}
	}

	return false;
}

bool AWukongBoss::WasRecentlyUsed(EWukongComboSet ComboSet, int32 ComboLength, int32 Depth) const
{
	const int32 HistoryNum = RecentActionHistory.Num();
	const int32 NumToCheck = FMath::Clamp(Depth, 0, HistoryNum);
	for (int32 Offset = 0; Offset < NumToCheck; ++Offset)
	{
		const int32 Index = HistoryNum - 1 - Offset;
		const FWukongActionRecord& Record = RecentActionHistory[Index];
		if (Record.ComboSet == ComboSet && Record.ComboLength == ComboLength)
		{
			return true;
		}
	}

	return false;
}

bool AWukongBoss::WasRecentlyUsed(EWukongNormalAttackType AttackType, int32 Depth) const
{
	const int32 HistoryNum = RecentActionHistory.Num();
	const int32 NumToCheck = FMath::Clamp(Depth, 0, HistoryNum);
	for (int32 Offset = 0; Offset < NumToCheck; ++Offset)
	{
		const int32 Index = HistoryNum - 1 - Offset;
		if (RecentActionHistory[Index].NormalAttackType == AttackType)
		{
			return true;
		}
	}

	return false;
}

bool AWukongBoss::WasNormalAttackRecentlyUsed(EWukongNormalAttackType AttackType, int32 Depth) const
{
	return WasRecentlyUsed(AttackType, Depth);
}

bool AWukongBoss::WasRecentlyUsedBowNormalAttack(int32 Depth) const
{
	const int32 HistoryNum = RecentActionHistory.Num();
	const int32 NumToCheck = FMath::Clamp(Depth, 0, HistoryNum);
	for (int32 Offset = 0; Offset < NumToCheck; ++Offset)
	{
		const int32 Index = HistoryNum - 1 - Offset;
		const FWukongActionRecord& Record = RecentActionHistory[Index];
		if (Record.ActionType == EWukongActionType::NormalAttack && IsBowNormalAttackType(Record.NormalAttackType))
		{
			return true;
		}
	}

	return false;
}

float AWukongBoss::GetTargetYawAbsDelta() const
{
	return FMath::Abs(GetCombatTargetYawDelta());
}

float AWukongBoss::GetTargetDistance2D() const
{
	if (!CurrentTarget)
	{
		return 0.f;
	}

	return FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
}

float AWukongBoss::GetPlannedAttackMinRange() const
{
	switch (PlannedAttackType)
	{
	case EWukongPlannedAttackType::NormalAttack:
		{
			const FWukongNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData ? NormalAttackData->MinRange : 0.f;
		}
	case EWukongPlannedAttackType::ComboAttack:
	case EWukongPlannedAttackType::None:
	default:
		return 0.f;
	}
}

float AWukongBoss::GetPlannedAttackMaxRange() const
{
	switch (PlannedAttackType)
	{
	case EWukongPlannedAttackType::NormalAttack:
		{
			const FWukongNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData ? NormalAttackData->MaxRange : 0.f;
		}
	case EWukongPlannedAttackType::ComboAttack:
		{
			const FWukongComboAttackData* ComboAttackData = GetComboAttackData(PlannedComboSet);
			return ComboAttackData ? ComboAttackData->AttackRange : 250.f;
		}
	case EWukongPlannedAttackType::None:
	default:
		return 0.f;
	}
}

float AWukongBoss::GetAttackCandidateMaxRange(EWukongPlannedAttackType AttackType) const
{
	switch (AttackType)
	{
	case EWukongPlannedAttackType::NormalAttack:
		{
			const FWukongNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData ? NormalAttackData->CandidateMaxRange : 0.f;
		}
	case EWukongPlannedAttackType::ComboAttack:
		{
			const FWukongComboAttackData* ComboAttackData = GetComboAttackData(PlannedComboSet);
			if (ComboAttackData)
			{
				return ComboAttackData->CandidateRange;
			}

			return FMath::Max(ComboAttackA.CandidateRange, ComboAttackB.CandidateRange);
		}
	case EWukongPlannedAttackType::None:
	default:
		return 0.f;
	}
}

bool AWukongBoss::IsTargetTooFarForPlannedAttack() const
{
	if (!CurrentTarget || !HasValidPlannedAttack())
	{
		return false;
	}

	return GetTargetDistance2D() > GetPlannedAttackMaxRange();
}

bool AWukongBoss::IsTargetTooCloseForPlannedAttack() const
{
	if (!CurrentTarget || !HasValidPlannedAttack())
	{
		return false;
	}

	return GetTargetDistance2D() < GetPlannedAttackMinRange();
}

bool AWukongBoss::CanTriggerCloseRangeBackDash() const
{
	return CloseRangeBackDashCooldownRemainTime <= 0.f && FMath::FRand() <= CloseRangeBackDashChance;
}

void AWukongBoss::StartCloseRangeBackDashCooldown()
{
	CloseRangeBackDashCooldownRemainTime = FMath::Max(0.f, CloseRangeBackDashCooldown);
}

void AWukongBoss::ResetReactiveActionState()
{
	CurrentReactiveAction = EWukongReactiveActionType::None;
}

void AWukongBoss::ResetActiveReactiveActionState()
{
	ActiveReactiveActionData = FWukongReactiveActionExecutionData();
}

void AWukongBoss::ResetReactiveEvadePressure()
{
	ReactiveBackwardEvadeCloseHitCount = 0;
	ReactiveBackwardEvadeAccumulatedTriggerChance = 0.f;
}

void AWukongBoss::ResetPendingAttackMotionState()
{
	StopCodeDrivenAttackMove(false);
	StopLightningSwordAirMove(false);
}

void AWukongBoss::ResetCurrentAttackRuntimeState()
{
	ClearDelayedNormalAttackRangeLink();
	ClearVariantNormalAttackState();
	EndBowAttackPresentation();
	bAirborneHitReactLockWindowActive = false;
	Super::ResetCurrentAttackRuntimeState();
	CurrentAttackActionType = EWukongActionType::None;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = EWukongNormalAttackType::None;
}

void AWukongBoss::ClearDelayedNormalAttackRangeLink()
{
	bHasDelayedNormalAttackRangeLink = false;
	DelayedNormalAttackRangeLinkType = EWukongNormalAttackType::None;
	DelayedNormalAttackRangeLinkBlendOutTime = 0.12f;
	DelayedNormalAttackRangeLinkBlendInTime = 0.12f;
	bDelayedNormalAttackRangeLinkUseTestFilter = true;
}

bool AWukongBoss::TryStartVariantNormalAttack(EWukongNormalAttackType AttackType, const FWukongNormalAttackData& AttackData)
{
	if (!IsVariantNormalAttackType(AttackType) ||
		!AttackData.Montage ||
		!CurrentTarget ||
		!IsPhaseTwoActive() ||
		!IsWithinVariantNormalAttackDistance(AttackType, AttackData) ||
		FMath::FRand() > FMath::Clamp(GetVariantNormalAttackChance(AttackType), 0.f, 1.f))
	{
		return false;
	}

	UAnimMontage* DashMontage = GetVariantNormalAttackDashMontage(AttackType);
	if (!DashMontage)
	{
		return false;
	}

	bVariantNormalAttackActive = true;
	VariantNormalAttackTargetType = AttackType;
	VariantNormalAttackDashPlayCount = 0;
	VariantNormalAttackRequiredDashCount = GetVariantNormalAttackRequiredDashCount(AttackType);
	bPostBowCloseRangeApproachInProgress = false;

	return PlayVariantNormalAttackDashStep();
}

bool AWukongBoss::AdvanceVariantNormalAttackFromNotify()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::Attack ||
		!bVariantNormalAttackActive ||
		VariantNormalAttackTargetType == EWukongNormalAttackType::None ||
		IsInHitReact())
	{
		return false;
	}

	if (VariantNormalAttackDashPlayCount < VariantNormalAttackRequiredDashCount)
	{
		return PlayVariantNormalAttackDashStep();
	}

	return PlayVariantNormalAttackFinalMontage();
}

bool AWukongBoss::PlayVariantNormalAttackDashStep()
{
	if (bExecutionReady || bBeingExecuted)
	{
		ClearVariantNormalAttackState();
		return false;
	}

	UAnimMontage* DashMontage = GetVariantNormalAttackDashMontage(VariantNormalAttackTargetType);
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!DashMontage || !AnimInstance)
	{
		ClearVariantNormalAttackState();
		return false;
	}

	if (CurrentAttackMontage)
	{
		AnimInstance->Montage_Stop(FMath::Max(0.f, VariantDashBlendOutTime), CurrentAttackMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();

	CurrentAttackSelection.Montage = DashMontage;
	CurrentAttackSelection.AttackRange = 0.f;
	CurrentAttackSelection.bFaceTargetDuringAttack = bVariantDashFaceTarget;
	CurrentAttackSelection.FacingDuration = VariantDashFacingDuration;
	CurrentAttackSelection.FacingInterpSpeed = VariantDashFacingInterpSpeed;
	CurrentAttackSelection.PlayRate = VariantDashPlayRate;
	CurrentAttackSelection.bTryTurnAfterAttack = false;
	CurrentAttackSelection.PostAttackTurnStartAngle = TurnStartAngle;
	CurrentAttackMontage = DashMontage;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	AttackTime = DashMontage->GetPlayLength() / FMath::Max(VariantDashPlayRate, KINDA_SMALL_NUMBER);
	AttackFacingRemainTime = CurrentAttackSelection.FacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;
	InitiativeState = EWukongInitiativeState::BossPattern;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, VariantDashBlendInTime));
	if (AnimInstance->Montage_PlayWithBlendSettings(DashMontage, BlendInSettings, VariantDashPlayRate) <= 0.f)
	{
		ClearVariantNormalAttackState();
		FinishAttack();
		return false;
	}

	++VariantNormalAttackDashPlayCount;
	return true;
}

bool AWukongBoss::PlayVariantNormalAttackFinalMontage()
{
	if (bExecutionReady || bBeingExecuted)
	{
		ClearVariantNormalAttackState();
		return false;
	}

	const EWukongNormalAttackType AttackType = VariantNormalAttackTargetType;
	const FWukongNormalAttackData* AttackData = GetNormalAttackData(AttackType);
	UAnimMontage* TargetAttackMontage = AttackData ? GetVariantNormalAttackFinalMontage(AttackType, *AttackData) : nullptr;
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AttackData || !TargetAttackMontage || !AnimInstance)
	{
		ClearVariantNormalAttackState();
		FinishAttack();
		return false;
	}

	if (CurrentAttackMontage)
	{
		AnimInstance->Montage_Stop(FMath::Max(0.f, VariantDashBlendOutTime), CurrentAttackMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ClearVariantNormalAttackState();

	CurrentCombatSubState = EWukongCombatState::Attack;
	CombatSubStateElapsedTime = 0.f;
	CurrentAttackActionType = EWukongActionType::NormalAttack;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = AttackType;
	LastStartedNormalAttackType = AttackType;
	PlannedAttackType = EWukongPlannedAttackType::NormalAttack;
	PlannedNormalAttackType = AttackType;

	CurrentAttackSelection.Montage = TargetAttackMontage;
	CurrentAttackSelection.AttackRange = AttackData->MaxRange;
	CurrentAttackSelection.bFaceTargetDuringAttack = AttackData->bFaceTargetDuringAttack;
	CurrentAttackSelection.FacingDuration = AttackData->bUseAnimNotifyTiming ? 0.f : AttackData->FacingDuration;
	CurrentAttackSelection.FacingInterpSpeed = AttackData->FacingInterpSpeed;
	CurrentAttackSelection.PlayRate = AttackData->PlayRate;
	CurrentAttackSelection.bTryTurnAfterAttack = AttackData->bTryTurnAfterAttack;
	CurrentAttackSelection.PostAttackTurnStartAngle = AttackData->PostAttackTurnStartAngle;
	CurrentAttackMontage = TargetAttackMontage;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	AttackTime = TargetAttackMontage->GetPlayLength() / FMath::Max(AttackData->PlayRate, KINDA_SMALL_NUMBER);
	AttackFacingRemainTime = CurrentAttackSelection.FacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;
	InitiativeState = EWukongInitiativeState::BossPattern;
	bPostBowCloseRangeApproachInProgress = false;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (!AttackData->bUseAnimNotifyTiming)
	{
		if (AttackData->MoveSpeed > 0.f)
		{
			const float MoveStartTime = AttackData->MoveStartTimeAtPlayRateOne;
			QueueCodeDrivenAttackMove(AttackType, MoveStartTime / FMath::Max(AttackData->PlayRate, KINDA_SMALL_NUMBER));
		}

		ApplyTimedMontageSuperArmor(AttackTime);
	}

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, VariantDashBlendInTime));
	if (AnimInstance->Montage_PlayWithBlendSettings(TargetAttackMontage, BlendInSettings, AttackData->PlayRate) <= 0.f)
	{
		FinishAttack();
		return false;
	}

	return true;
}

bool AWukongBoss::IsVariantNormalAttackType(EWukongNormalAttackType AttackType) const
{
	return AttackType == EWukongNormalAttackType::ChargeAttack ||
		AttackType == EWukongNormalAttackType::NinjaB;
}

float AWukongBoss::GetVariantNormalAttackChance(EWukongNormalAttackType AttackType) const
{
	switch (AttackType)
	{
	case EWukongNormalAttackType::ChargeAttack:
		return ChargeVariantChance;
	case EWukongNormalAttackType::NinjaB:
		return NinjaBVariantChance;
	default:
		return 0.f;
	}
}

bool AWukongBoss::IsWithinVariantNormalAttackDistance(EWukongNormalAttackType AttackType, const FWukongNormalAttackData& AttackData) const
{
	float MinDistance = AttackData.MinRange;
	float MaxDistance = AttackData.CandidateMaxRange > 0.f ? AttackData.CandidateMaxRange : AttackData.MaxRange;

	switch (AttackType)
	{
	case EWukongNormalAttackType::ChargeAttack:
		MinDistance = ChargeVariantMinDistance > 0.f ? ChargeVariantMinDistance : MinDistance;
		MaxDistance = ChargeVariantMaxDistance > 0.f ? ChargeVariantMaxDistance : MaxDistance;
		break;
	case EWukongNormalAttackType::NinjaB:
		MinDistance = NinjaBVariantMinDistance > 0.f ? NinjaBVariantMinDistance : MinDistance;
		MaxDistance = NinjaBVariantMaxDistance > 0.f ? NinjaBVariantMaxDistance : MaxDistance;
		break;
	default:
		break;
	}

	const float TargetDistance = GetTargetDistance2D();
	return TargetDistance >= MinDistance && (MaxDistance <= 0.f || TargetDistance <= MaxDistance);
}

UAnimMontage* AWukongBoss::GetVariantNormalAttackDashMontage(EWukongNormalAttackType AttackType) const
{
	switch (AttackType)
	{
	case EWukongNormalAttackType::ChargeAttack:
		if (bVariantNormalAttackActive && VariantNormalAttackDashPlayCount > 0 && ChargeVariantFollowUpDashMontage)
		{
			return ChargeVariantFollowUpDashMontage.Get();
		}
		return ChargeVariantDashMontage.Get();
	case EWukongNormalAttackType::NinjaB:
		if (bVariantNormalAttackActive && VariantNormalAttackDashPlayCount > 0 && NinjaBVariantFollowUpDashMontage)
		{
			return NinjaBVariantFollowUpDashMontage.Get();
		}
		return NinjaBVariantDashMontage.Get();
	default:
		return nullptr;
	}
}

UAnimMontage* AWukongBoss::GetVariantNormalAttackFinalMontage(EWukongNormalAttackType AttackType, const FWukongNormalAttackData& AttackData) const
{
	switch (AttackType)
	{
	case EWukongNormalAttackType::ChargeAttack:
		return ChargeVariantAttackMontage ? ChargeVariantAttackMontage.Get() : AttackData.Montage.Get();
	case EWukongNormalAttackType::NinjaB:
		return NinjaBVariantAttackMontage ? NinjaBVariantAttackMontage.Get() : AttackData.Montage.Get();
	default:
		return AttackData.Montage.Get();
	}
}

int32 AWukongBoss::GetVariantNormalAttackRequiredDashCount(EWukongNormalAttackType AttackType) const
{
	return AttackType == EWukongNormalAttackType::NinjaB ? 2 : 1;
}

void AWukongBoss::ClearVariantNormalAttackState()
{
	bVariantNormalAttackActive = false;
	VariantNormalAttackTargetType = EWukongNormalAttackType::None;
	VariantNormalAttackDashPlayCount = 0;
	VariantNormalAttackRequiredDashCount = 0;
}

bool AWukongBoss::IsBowNormalAttackType(EWukongNormalAttackType AttackType) const
{
	return AttackType == EWukongNormalAttackType::Bow4Combo ||
		AttackType == EWukongNormalAttackType::BowBackDashAttack ||
		AttackType == EWukongNormalAttackType::BowHeavyAttack;
}

void AWukongBoss::BeginBowAttackPresentation()
{
	bBowAttackPresentationActive = true;
	SetWeaponVisible(false);
	SetBowVisible(true);
}

void AWukongBoss::EndBowAttackPresentation()
{
	if (AttachedArrow)
	{
		AttachedArrow->Destroy();
		AttachedArrow = nullptr;
	}

	SetBowVisible(false);
	AttachWeaponToHandSocket();
	SetWeaponVisible(true);
	bBowAttackPresentationActive = false;
}

void AWukongBoss::ApplyTimedMontageSuperArmor(float MontageDuration, float DurationScale)
{
	const float ResolvedDurationScale = DurationScale >= 0.f ? DurationScale : MontageSuperArmorDurationScale;
	const float SuperArmorDuration = FMath::Max(0.f, MontageDuration * ResolvedDurationScale);
	TimedMontageSuperArmorRemainTime = SuperArmorDuration;

	if (TimedMontageSuperArmorRemainTime > 0.f)
	{
		AddGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
	}
}

void AWukongBoss::UpdateTimedMontageSuperArmor(float DeltaTime)
{
	if (TimedMontageSuperArmorRemainTime <= 0.f)
	{
		return;
	}

	TimedMontageSuperArmorRemainTime = FMath::Max(0.f, TimedMontageSuperArmorRemainTime - DeltaTime);
	if (TimedMontageSuperArmorRemainTime <= 0.f)
	{
		RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
	}
}

void AWukongBoss::ClearTimedMontageSuperArmor()
{
	TimedMontageSuperArmorRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
}

void AWukongBoss::BeginAttackSuperArmorWindow()
{
	TimedMontageSuperArmorRemainTime = 0.f;
	AddGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
}

void AWukongBoss::EndAttackSuperArmorWindow()
{
	TimedMontageSuperArmorRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
}

void AWukongBoss::BeginAirborneHitReactLockWindow()
{
	bAirborneHitReactLockWindowActive = true;
}

void AWukongBoss::EndAirborneHitReactLockWindow()
{
	bAirborneHitReactLockWindowActive = false;
}

bool AWukongBoss::HasQueuedReactiveAction() const
{
	return CurrentReactiveAction != EWukongReactiveActionType::None;
}

bool AWukongBoss::TryEnterReactiveStateFromIdle()
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

bool AWukongBoss::TryEnterPlannedStateFromIdle()
{
	if (bNoAttackCandidateApproachInProgress)
	{
		SetWukongCombatState(EWukongCombatState::Approach);
		return true;
	}

	switch (PlannedActionType)
	{
	case EWukongPlannedActionType::Attack:
		return TryEnterPlannedAttackStateFromIdle();
	case EWukongPlannedActionType::NonAttack:
		return TryEnterPlannedNonAttackStateFromIdle();
	case EWukongPlannedActionType::None:
	default:
		return false;
	}
}

bool AWukongBoss::TryEnterPlannedAttackStateFromIdle()
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
		if (CanStartWukongTurn())
		{
			SetWukongCombatState(EWukongCombatState::Turn);
			return true;
		}

		if (!IsTargetTooFarForPlannedAttack() && !IsTargetTooCloseForPlannedAttack())
		{
			SetWukongCombatState(EWukongCombatState::Reposition);
			return true;
		}

		SetWukongCombatState(EWukongCombatState::Approach);
		return true;
	}

	if (IsTargetTooFarForPlannedAttack())
	{
		SetWukongCombatState(EWukongCombatState::Approach);
		return true;
	}

	if (IsTargetTooCloseForPlannedAttack())
	{
		SetWukongCombatState(EWukongCombatState::Reposition);
		return true;
	}

	if (CanAttackTarget())
	{
		SetWukongCombatState(EWukongCombatState::Attack);
		return true;
	}

	return true;
}

bool AWukongBoss::TryEnterPlannedNonAttackStateFromIdle()
{
	if (!HasValidPlannedMovement())
	{
		ResetPlannedCombatPlan();
		return true;
	}

	if (PlannedNonAttackType == EWukongPlannedNonAttackType::Hold)
	{
		BeginNoAttackCandidateApproach();
		SetWukongCombatState(EWukongCombatState::Approach);
		return true;
	}

	if (IsMobilityNonAttackType(PlannedNonAttackType) && !CanStartMobilityMontageSafely())
	{
		SetDesiredMoveAxes(0.f, 0.f);
		return true;
	}

	if (GetTargetYawAbsDelta() >= MoveTurnStartAngle && CanStartWukongTurn())
	{
		SetWukongCombatState(EWukongCombatState::Turn);
		return true;
	}

	if (PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe)
	{
		if (CurrentTarget && GetTargetDistance2D() < StrafeMinDistanceToTarget)
		{
			if (DashBackwardMontage && CanTriggerCloseRangeBackDash() && CanStartMobilityMontageSafely())
			{
				StartCloseRangeBackDashCooldown();
				PlannedNonAttackType = EWukongPlannedNonAttackType::Dash;
				PlannedMovementDirection = EWukongMovementDirection::Backward;
				SetWukongCombatState(EWukongCombatState::Evade);
				return true;
			}

			PlannedMovementDirection = EWukongMovementDirection::Backward;
		}

		SetWukongCombatState(EWukongCombatState::Reposition);
		return true;
	}

	SetWukongCombatState(IsMobilityNonAttackType(PlannedNonAttackType)
		? EWukongCombatState::Evade
		: EWukongCombatState::NonAttackAction);
	return true;
}

bool AWukongBoss::CanStartMobilityMontageSafely() const
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	return !AnimInstance || AnimInstance->GetCurrentActiveMontage() == nullptr;
}

bool AWukongBoss::CanParryIncomingHit(EHitReactType HitType, AActor* DamageCauser, const FVector& SwingDirection) const
{
	const bool bCanChainParryFromParrySuccess =
		CurrentCombatSubState == EWukongCombatState::ParrySuccess;

	if (CurrentState != EMonsterState::Combat ||
		!IsParryableHitType(HitType) ||
		!DamageCauser ||
		CurrentAttackActionType == EWukongActionType::ParryCounter ||
		bBeingExecuted ||
		bExecutionReady)
	{
		return false;
	}

	if (!bCanChainParryFromParrySuccess &&
		(!CanBeInterruptedBy(HitType) || !ShouldStartHitReact(HitType)))
	{
		return false;
	}

	if (!IsDamageCauserInParryFrontCone(DamageCauser))
	{
		return false;
	}

	const ECharacterHitReactDirection HitDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	if (HitDirection != ECharacterHitReactDirection::Front_F &&
		HitDirection != ECharacterHitReactDirection::Front_FL &&
		HitDirection != ECharacterHitReactDirection::Front_FR)
	{
		return false;
	}

	return true;
}

bool AWukongBoss::IsDamageCauserInParryFrontCone(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return false;
	}

	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(DamageCauser->GetActorLocation() - GetActorLocation());
	ToAttackerLocal.Z = 0.f;
	if (ToAttackerLocal.IsNearlyZero())
	{
		return true;
	}

	const float AttackerYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(ToAttackerLocal.Y, ToAttackerLocal.X));
	const float HalfConeAngle = FMath::Clamp(ParryFrontConeAngle * 0.5f, 0.f, 180.f);
	return FMath::Abs(AttackerYawDegrees) <= HalfConeAngle;
}

bool AWukongBoss::IsParryableHitType(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
	case EHitReactType::LargeHit_Short:
	case EHitReactType::LargeHit_Long:
	case EHitReactType::Airborne:
	case EHitReactType::Knockdown:
		return true;
	case EHitReactType::None:
	case EHitReactType::Execution:
	case EHitReactType::Dead:
	default:
		return false;
	}
}

float AWukongBoss::GetPerfectParryChanceForHitType(EHitReactType HitType) const
{
	return FMath::Clamp(
		HitType == EHitReactType::LightHit ? CurrentPerfectParryChance : CurrentStrongPerfectParryChance,
		0.f,
		1.f);
}

float AWukongBoss::GetNormalParryChanceForHitType(EHitReactType HitType) const
{
	return FMath::Clamp(
		HitType == EHitReactType::LightHit ? CurrentNormalParryChance : CurrentStrongNormalParryChance,
		0.f,
		1.f);
}

void AWukongBoss::ResetParryExchangeCycle()
{
	CurrentNormalParryChance = 0.f;
	CurrentPerfectParryChance = 0.f;
	CurrentStrongNormalParryChance = 0.f;
	CurrentStrongPerfectParryChance = 0.f;
	CurrentParryExchangeNormalParryCount = 0;
	ParryExchangeIdleTime = 0.f;
	bPendingParryExchangeChanceGain = false;
	PendingParryExchangeHitType = EHitReactType::None;
}

void AWukongBoss::ResetParryExchangePerfectChance(EHitReactType HitType)
{
	if (HitType == EHitReactType::LightHit)
	{
		CurrentPerfectParryChance = 0.f;
	}
	else
	{
		CurrentStrongPerfectParryChance = 0.f;
	}

	ParryExchangeIdleTime = 0.f;
}

void AWukongBoss::AccumulateParryExchangeNormalChance(EHitReactType HitType)
{
	if (HitType == EHitReactType::LightHit)
	{
		CurrentNormalParryChance = FMath::Clamp(
			CurrentNormalParryChance + FMath::Clamp(NormalParryChanceGainPerIncomingHit, 0.f, 1.f),
			0.f,
			1.f);
	}
	else
	{
		CurrentStrongNormalParryChance = FMath::Clamp(
			CurrentStrongNormalParryChance + FMath::Clamp(StrongNormalParryChanceGainPerIncomingHit, 0.f, 1.f),
			0.f,
			1.f);
	}

	ParryExchangeIdleTime = 0.f;
}

void AWukongBoss::AccumulateParryExchangePerfectChance(EHitReactType HitType)
{
	if (HitType == EHitReactType::LightHit)
	{
		CurrentPerfectParryChance = FMath::Clamp(
			CurrentPerfectParryChance + FMath::Clamp(PerfectParryChanceGainPerNormalParry, 0.f, 1.f),
			0.f,
			1.f);
	}
	else
	{
		CurrentStrongPerfectParryChance = FMath::Clamp(
			CurrentStrongPerfectParryChance + FMath::Clamp(StrongPerfectParryChanceGainPerNormalParry, 0.f, 1.f),
			0.f,
			1.f);
	}

	++CurrentParryExchangeNormalParryCount;
	ParryExchangeIdleTime = 0.f;
}

void AWukongBoss::UpdateParryExchangeDecay(float DeltaTime)
{
	if (CurrentNormalParryChance <= 0.f &&
		CurrentPerfectParryChance <= 0.f &&
		CurrentStrongNormalParryChance <= 0.f &&
		CurrentStrongPerfectParryChance <= 0.f)
	{
		CurrentNormalParryChance = 0.f;
		CurrentPerfectParryChance = 0.f;
		CurrentStrongNormalParryChance = 0.f;
		CurrentStrongPerfectParryChance = 0.f;
		CurrentParryExchangeNormalParryCount = 0;
		ParryExchangeIdleTime = 0.f;
		return;
	}

	if (CurrentCombatSubState == EWukongCombatState::ParrySuccess ||
		(InitiativeState == EWukongInitiativeState::BossParryCounter &&
			CurrentCombatSubState == EWukongCombatState::Attack))
	{
		ParryExchangeIdleTime = 0.f;
		return;
	}

	ParryExchangeIdleTime += DeltaTime;
	if (ParryExchangeIdleTime < ParryExchangeDecayDelay)
	{
		return;
	}

	const float DecayAmount = FMath::Max(0.f, ParryExchangeChanceDecayPerSecond) * DeltaTime;
	CurrentNormalParryChance = FMath::Max(0.f, CurrentNormalParryChance - DecayAmount);
	CurrentPerfectParryChance = FMath::Max(0.f, CurrentPerfectParryChance - DecayAmount);
	CurrentStrongNormalParryChance = FMath::Max(0.f, CurrentStrongNormalParryChance - DecayAmount);
	CurrentStrongPerfectParryChance = FMath::Max(0.f, CurrentStrongPerfectParryChance - DecayAmount);

	if (CurrentNormalParryChance <= 0.f &&
		CurrentPerfectParryChance <= 0.f &&
		CurrentStrongNormalParryChance <= 0.f &&
		CurrentStrongPerfectParryChance <= 0.f)
	{
		CurrentParryExchangeNormalParryCount = 0;
	}
}

bool AWukongBoss::ShouldEscapeParryExchangeBeforeCounter() const
{
	return GetParryExchangeEscapeMontage() &&
		CurrentParryExchangeNormalParryCount >= FMath::Max(0, ParryExchangeEscapeMinNormalParryCount) &&
		FMath::FRand() <= FMath::Clamp(ParryExchangeEscapeChance, 0.f, 1.f);
}

UAnimMontage* AWukongBoss::GetParryExchangeEscapeMontage() const
{
	const bool bCanUseDash = DashBackwardMontage != nullptr;
	const bool bCanUseDodge = DodgeBackwardMontage != nullptr;
	if (bCanUseDash && bCanUseDodge)
	{
		return FMath::RandBool() ? DashBackwardMontage.Get() : DodgeBackwardMontage.Get();
	}

	if (bCanUseDash)
	{
		return DashBackwardMontage.Get();
	}

	return bCanUseDodge ? DodgeBackwardMontage.Get() : nullptr;
}

UAnimMontage* AWukongBoss::GetParrySuccessMontage(const FVector& SwingDirection)
{
	if (!bHasSelectedInitialParrySuccessSide)
	{
		bNextParrySuccessUsesLeftSide = FMath::RandBool();
		bHasSelectedInitialParrySuccessSide = true;
	}

	const bool bUseLeft = bNextParrySuccessUsesLeftSide;
	bNextParrySuccessUsesLeftSide = !bNextParrySuccessUsesLeftSide;

	UAnimMontage* SelectedMontage = nullptr;
	if (bUseLeft)
	{
		SelectedMontage = ParrySuccessLUpMontage ? ParrySuccessLUpMontage.Get() : ParrySuccessLDownMontage.Get();
	}
	else
	{
		SelectedMontage = ParrySuccessRUpMontage ? ParrySuccessRUpMontage.Get() : ParrySuccessRDownMontage.Get();
	}

	bCurrentParryCounterUsesLeftMontage =
		SelectedMontage == ParrySuccessLUpMontage.Get() ||
		SelectedMontage == ParrySuccessLDownMontage.Get();
	bCurrentParryCounterUsesDownMontage =
		SelectedMontage == ParrySuccessLDownMontage.Get() ||
		SelectedMontage == ParrySuccessRDownMontage.Get();
	return SelectedMontage;
}

UAnimMontage* AWukongBoss::GetParryCounterMontage() const
{
	if (bCurrentParryCounterUsesDownMontage)
	{
		if (bCurrentParryCounterUsesLeftMontage)
		{
			return ParryCounterDownLeftMontage
				? ParryCounterDownLeftMontage.Get()
				: (ParryCounterLeftMontage ? ParryCounterLeftMontage.Get() : ParryCounterRightMontage.Get());
		}

		return ParryCounterDownRightMontage
			? ParryCounterDownRightMontage.Get()
			: (ParryCounterRightMontage ? ParryCounterRightMontage.Get() : ParryCounterLeftMontage.Get());
	}

	if (bCurrentParryCounterUsesLeftMontage)
	{
		return ParryCounterLeftMontage ? ParryCounterLeftMontage.Get() : ParryCounterRightMontage.Get();
	}

	return ParryCounterRightMontage ? ParryCounterRightMontage.Get() : ParryCounterLeftMontage.Get();
}

UAnimMontage* AWukongBoss::GetParryCounterFollowUpMontage() const
{
	if (bCurrentParryCounterUsesDownMontage)
	{
		return nullptr;
	}

	if (bCurrentParryCounterUsesLeftMontage)
	{
		return ParryCounterFollowUpLeftMontage
			? ParryCounterFollowUpLeftMontage.Get()
			: ParryCounterFollowUpRightMontage.Get();
	}

	return ParryCounterFollowUpRightMontage
		? ParryCounterFollowUpRightMontage.Get()
		: ParryCounterFollowUpLeftMontage.Get();
}

UNiagaraComponent* AWukongBoss::FindNiagaraComponentByName(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent && NiagaraComponent->GetFName() == ComponentName)
		{
			return NiagaraComponent;
		}
	}

	return nullptr;
}

USceneComponent* AWukongBoss::FindSceneComponentByName(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == ComponentName)
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void AWukongBoss::PlayParryParticle()
{
	if (!CachedParryParticleComponent)
	{
		CachedParryParticleComponent = FindNiagaraComponentByName(ParryParticleComponentName);
	}

	if (!CachedParryParticleComponent)
	{
		return;
	}

	if (!CachedParryEffectLocationComponent)
	{
		CachedParryEffectLocationComponent = FindSceneComponentByName(ParryEffectLocationComponentName);
	}

	UNiagaraSystem* ParryNiagaraAsset = CachedParryParticleComponent->GetAsset();
	if (!ParryNiagaraAsset)
	{
		return;
	}

	if (UJunCombatVFXSubsystem* VFXSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UJunCombatVFXSubsystem>() : nullptr)
	{
		const USceneComponent* SpawnLocationComponent = CachedParryEffectLocationComponent ? CachedParryEffectLocationComponent.Get() : GetMesh();
		VFXSubsystem->SpawnCombatNiagaraAtComponent(ParryNiagaraAsset, SpawnLocationComponent);
	}
}

void AWukongBoss::StartParrySuccessAgainstIncomingHit(
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	EndBowAttackPresentation();
	PlayParryParticle();

	bParryCounterStarted = false;
	bCurrentParryCounterPerfectParried = false;
	bParryCounterFollowUpStarted = false;
	InitiativeState = EWukongInitiativeState::BossParryCounter;
	CurrentParrySuccessMontage = GetParrySuccessMontage(SwingDirection);

	if (CurrentCombatSubState == EWukongCombatState::ParrySuccess)
	{
		CombatSubStateElapsedTime = 0.f;
		EnterParrySuccessState();
	}
	else
	{
		SetWukongCombatState(EWukongCombatState::ParrySuccess);
	}

	ApplyHitReactKnockback(DamageCauser, DefenseKnockbackData.ParrySuccess, CurrentParrySuccessMontage);

	if (CurrentParrySuccessMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
			AnimInstance->OnMontageEnded.AddDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
		}

		const float PlayedDuration = PlayAnimMontage(CurrentParrySuccessMontage);
		ParrySuccessFallbackRemainTime = PlayedDuration > 0.f ? 0.f : ParrySuccessFallbackDuration;
	}
	else
	{
		ParrySuccessFallbackRemainTime = ParrySuccessFallbackDuration;
	}
}

bool AWukongBoss::TryStartParryCounterDecision()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		bParryCounterStarted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::ParrySuccess)
	{
		return false;
	}

	if (ShouldEscapeParryExchangeBeforeCounter() && TryStartParryBackJump())
	{
		ResetParryExchangeCycle();
		return true;
	}

	return TryStartParryCounterAttack();
}

bool AWukongBoss::TryStartParryCounterAttack()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		bParryCounterStarted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::ParrySuccess)
	{
		return false;
	}

	UAnimMontage* CounterMontage = GetParryCounterMontage();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!CounterMontage || !AnimInstance)
	{
		return false;
	}

	bParryCounterStarted = true;
	bCurrentParryCounterPerfectParried = false;
	bParryCounterFollowUpStarted = false;
	ParrySuccessFallbackRemainTime = 0.f;
	CurrentCombatSubState = EWukongCombatState::Attack;
	CombatSubStateElapsedTime = 0.f;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
	if (CurrentParrySuccessMontage)
	{
		AnimInstance->Montage_Stop(ParryCounterBlendOutTime, CurrentParrySuccessMontage);
	}

	CurrentParrySuccessMontage = nullptr;
	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();

	CurrentAttackActionType = EWukongActionType::ParryCounter;
	CurrentAttackSelection.Montage = CounterMontage;
	CurrentAttackSelection.AttackRange = ParryCounterAttackRange;
	CurrentAttackSelection.bFaceTargetDuringAttack = true;
	CurrentAttackSelection.FacingDuration = ParryCounterFacingDuration;
	CurrentAttackSelection.FacingInterpSpeed = ParryCounterFacingInterpSpeed;
	CurrentAttackSelection.PlayRate = ParryCounterPlayRate;
	CurrentAttackSelection.bTryTurnAfterAttack = true;
	CurrentAttackSelection.PostAttackTurnStartAngle = TurnStartAngle;
	CurrentAttackMontage = CounterMontage;
	InitiativeState = EWukongInitiativeState::BossParryCounter;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	AttackTime = CounterMontage->GetPlayLength() / FMath::Max(ParryCounterPlayRate, KINDA_SMALL_NUMBER);
	AttackFacingRemainTime = ParryCounterFacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	const FMontageBlendSettings CounterBlendSettings(ParryCounterBlendInTime);
	if (AnimInstance->Montage_PlayWithBlendSettings(CounterMontage, CounterBlendSettings, ParryCounterPlayRate) <= 0.f)
	{
		FinishAttack();
		SetWukongCombatState(EWukongCombatState::Idle);
		return false;
	}

	return true;
}

bool AWukongBoss::TryStartParryCounterFollowUp()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		bParryCounterFollowUpStarted ||
		bCurrentParryCounterPerfectParried ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::Attack ||
		CurrentAttackActionType != EWukongActionType::ParryCounter ||
		InitiativeState != EWukongInitiativeState::BossParryCounter)
	{
		return false;
	}

	UAnimMontage* FollowUpMontage = GetParryCounterFollowUpMontage();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!FollowUpMontage || !AnimInstance)
	{
		return false;
	}

	bParryCounterFollowUpStarted = true;

	if (CurrentAttackMontage)
	{
		AnimInstance->Montage_Stop(ParryCounterFollowUpBlendOutTime, CurrentAttackMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();

	CurrentAttackSelection.Montage = FollowUpMontage;
	CurrentAttackSelection.AttackRange = ParryCounterFollowUpAttackRange;
	CurrentAttackSelection.bFaceTargetDuringAttack = true;
	CurrentAttackSelection.FacingDuration = ParryCounterFollowUpFacingDuration;
	CurrentAttackSelection.FacingInterpSpeed = ParryCounterFollowUpFacingInterpSpeed;
	CurrentAttackSelection.PlayRate = ParryCounterFollowUpPlayRate;
	CurrentAttackSelection.bTryTurnAfterAttack = true;
	CurrentAttackSelection.PostAttackTurnStartAngle = TurnStartAngle;
	CurrentAttackMontage = FollowUpMontage;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	AttackTime = FollowUpMontage->GetPlayLength() / FMath::Max(ParryCounterFollowUpPlayRate, KINDA_SMALL_NUMBER);
	AttackFacingRemainTime = ParryCounterFollowUpFacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;
	InitiativeState = EWukongInitiativeState::BossParryCounter;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	const FMontageBlendSettings FollowUpBlendSettings(ParryCounterFollowUpBlendInTime);
	if (AnimInstance->Montage_PlayWithBlendSettings(FollowUpMontage, FollowUpBlendSettings, ParryCounterFollowUpPlayRate) <= 0.f)
	{
		return false;
	}

	return true;
}

void AWukongBoss::OnParrySuccessMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!CurrentParrySuccessMontage || Montage != CurrentParrySuccessMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	FinishParrySuccessState();
}

void AWukongBoss::FinishParrySuccessStateFromNotify()
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	FinishParrySuccessState();
}

void AWukongBoss::FinishParrySuccessState()
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
	}

	CurrentParrySuccessMontage = nullptr;
	ParrySuccessFallbackRemainTime = 0.f;
	ParrySuccessExitActionLockRemainTime = FMath::Max(0.f, ParrySuccessExitActionLockDuration);
	CodeFacingLockRemainTime = FMath::Max(CodeFacingLockRemainTime, ParrySuccessExitActionLockDuration);
	bParryCounterStarted = false;
	bCurrentParryCounterPerfectParried = false;
	bParryCounterFollowUpStarted = false;

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ResetCurrentAttackRuntimeState();

	if (CurrentState == EMonsterState::Combat &&
		CurrentCombatSubState == EWukongCombatState::ParrySuccess)
	{
		if (InitiativeState == EWukongInitiativeState::BossParryCounter)
		{
			InitiativeState = EWukongInitiativeState::Neutral;
		}

		SetWukongCombatState(EWukongCombatState::Idle);
	}
}

void AWukongBoss::UpdateFacingTowardsTargetWithoutTurn(float DeltaTime, float InterpSpeed)
{
	if (!CurrentTarget)
	{
		return;
	}

	if (CodeFacingLockRemainTime > 0.f ||
		CurrentCombatSubState == EWukongCombatState::Idle ||
		CurrentCombatSubState == EWukongCombatState::ParrySuccess)
	{
		return;
	}

	if (CurrentCombatSubState == EWukongCombatState::Idle &&
		bUseIdleHoldForYawMismatch &&
		CombatSubStateElapsedTime < YawMismatchIdleHoldDuration)
	{
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = ToTarget.Rotation();
	const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw));
	const bool bMovementFacingState =
		CurrentCombatSubState == EWukongCombatState::Approach ||
		CurrentCombatSubState == EWukongCombatState::Reposition ||
		CurrentCombatSubState == EWukongCombatState::Evade;
	if (!bMovementFacingState && YawDelta > MaxCodeFacingAngle)
	{
		if (CanStartWukongTurn())
		{
			SetWukongCombatState(EWukongCombatState::Turn);
		}
		return;
	}

	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);
	SetActorRotation(NewRotation);
}

FVector AWukongBoss::GetLockOnTargetPoint() const
{
	const FVector DefaultTargetPoint = Super::GetLockOnTargetPoint();

	if (CurrentAttackActionType != EWukongActionType::NormalAttack ||
		CurrentAttackNormalAttackType != EWukongNormalAttackType::JumpAttack)
	{
		return DefaultTargetPoint;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return DefaultTargetPoint;
	}

	static const FName LockOnBoneName(TEXT("spine_03"));
	if (MeshComp->GetBoneIndex(LockOnBoneName) == INDEX_NONE)
	{
		return DefaultTargetPoint;
	}

	FVector LockOnPoint = DefaultTargetPoint;
	LockOnPoint.Z = MeshComp->GetBoneLocation(LockOnBoneName).Z;
	return LockOnPoint;
}

bool AWukongBoss::TryHandleIncomingHitBeforeDamage(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!DefenseRuleData.bCanBeParried)
	{
		return false;
	}

	bPendingParryExchangeChanceGain = false;
	PendingParryExchangeHitType = EHitReactType::None;

	if (!CanParryIncomingHit(HitType, DamageCauser, SwingDirection))
	{
		return false;
	}

	const float PerfectChance = GetPerfectParryChanceForHitType(HitType);
	const float NormalChance = GetNormalParryChanceForHitType(HitType);

	if (FMath::FRand() <= PerfectChance)
	{
		ResetParryExchangePerfectChance(HitType);
		PlayDefenseSound(EJunDefenseSoundType::PerfectParry);
		StartParrySuccessAgainstIncomingHit(DamageCauser, SwingDirection, DefenseKnockbackData);
		return true;
	}

	if (FMath::FRand() <= NormalChance)
	{
		AccumulateParryExchangePerfectChance(HitType);
		PlayDefenseSound(EJunDefenseSoundType::NormalParry);
		AddPosture(NormalParryPostureGain);
		if (bExecutionReady || bBeingExecuted)
		{
			return true;
		}

		const EWukongInitiativeState SavedInitiativeState = InitiativeState;
		StartParrySuccessAgainstIncomingHit(DamageCauser, SwingDirection, DefenseKnockbackData);
		InitiativeState = SavedInitiativeState;
		bParryCounterStarted = true;
		bCurrentParryCounterPerfectParried = true;
		return true;
	}

	bPendingParryExchangeChanceGain = true;
	PendingParryExchangeHitType = HitType;
	return false;
}

void AWukongBoss::NotifyAttackParriedBy(
	AJunPlayer* Parrier,
	float PostureScale,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	Super::NotifyAttackParriedBy(Parrier, PostureScale, DefenseRuleData);
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	const bool bIsParryCounterAttack =
		CurrentAttackActionType == EWukongActionType::ParryCounter &&
		InitiativeState == EWukongInitiativeState::BossParryCounter;

	if (PostureScale >= 1.f && bIsParryCounterAttack)
	{
		ResetParryExchangeCycle();
	}

	if (PostureScale >= 1.f && TryStartPerfectParryRebound(Parrier, DefenseRuleData))
	{
		return;
	}

	// 일반 패턴 공격은 플레이어가 패리해도 공격권을 넘기지 않는다.
	// 보스가 플레이어 공격을 완벽 패리해서 얻은 반격만 다시 빼앗길 수 있다.
	if (!bIsParryCounterAttack || PostureScale < 1.f)
	{
		return;
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	CurrentAttackActionType = EWukongActionType::None;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = EWukongNormalAttackType::None;
	AttackFacingRemainTime = 0.f;

	StartPlayerPressure();
	bCurrentParryCounterPerfectParried = true;
}

bool AWukongBoss::TryStartPerfectParryRebound(AJunPlayer* Parrier, const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!DefenseRuleData.bCanReboundOnPerfectParry ||
		!Parrier ||
		bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::Attack ||
		!bIsAttacking ||
		IsInHitReact())
	{
		return false;
	}

	if (FMath::FRand() > FMath::Clamp(DefenseRuleData.ReboundChance, 0.f, 1.f))
	{
		return false;
	}

	UAnimMontage* ReboundMontage = GetPerfectParryReboundMontage(Parrier);
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!ReboundMontage || !AnimInstance)
	{
		return false;
	}

	UAnimMontage* InterruptedMontage = CurrentAttackMontage.Get();
	if (InterruptedMontage)
	{
		AnimInstance->Montage_Stop(FMath::Max(0.f, DefenseRuleData.ReboundBlendTime), InterruptedMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	StopCodeDrivenAttackMove(false);
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ClearDelayedNormalAttackRangeLink();
	EndBowAttackPresentation();

	CurrentAttackMontage = ReboundMontage;
	CurrentAttackSelection.Montage = ReboundMontage;
	CurrentAttackSelection.AttackRange = 0.f;
	CurrentAttackSelection.bFaceTargetDuringAttack = true;
	CurrentAttackSelection.FacingDuration = 0.f;
	CurrentAttackSelection.FacingInterpSpeed = AttackFacingInterpSpeed;
	CurrentAttackSelection.PlayRate = 1.f;
	CurrentAttackSelection.bTryTurnAfterAttack = false;
	CurrentAttackActionType = EWukongActionType::None;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = EWukongNormalAttackType::None;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = true;
	AttackTime = ReboundMontage->GetPlayLength();
	AttackFacingRemainTime = 0.f;
	CombatMoveInput = FVector2D::ZeroVector;
	InitiativeState = EWukongInitiativeState::PlayerPressure;
	StartPlayerPressure();

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->bOrientRotationToMovement = false;
	}

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, DefenseRuleData.ReboundBlendTime));
	if (AnimInstance->Montage_PlayWithBlendSettings(ReboundMontage, BlendInSettings) <= 0.f)
	{
		FinishAttack();
		return false;
	}

	return true;
}

UAnimMontage* AWukongBoss::GetPerfectParryReboundMontage(const AJunPlayer* Parrier) const
{
	if (Parrier && Parrier->DidLastParrySuccessUseLeftSide())
	{
		return PerfectParryReboundLMontage ? PerfectParryReboundLMontage.Get() : PerfectParryReboundRMontage.Get();
	}

	return PerfectParryReboundRMontage ? PerfectParryReboundRMontage.Get() : PerfectParryReboundLMontage.Get();
}

bool AWukongBoss::NotifyMikiriCounteredBy(AJunPlayer* CounterPlayer)
{
	if (!CounterPlayer ||
		bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::Attack ||
		CurrentAttackActionType != EWukongActionType::NormalAttack ||
		CurrentAttackNormalAttackType != EWukongNormalAttackType::NinjaB ||
		!CurrentAttackMontage ||
		!MikiriCounterMontage)
	{
		return false;
	}

	UAnimMontage* InterruptedMontage = CurrentAttackMontage.Get();
	UAnimMontage* CounterMontage = MikiriCounterMontage.Get();
	UAnimInstance* CurrentAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!CurrentAnimInstance || !InterruptedMontage || !CounterMontage)
	{
		return false;
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	AttackFacingRemainTime = 0.f;

	CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, MikiriCounterBlendOutTime), InterruptedMontage);
	CurrentAttackMontage = CounterMontage;
	CurrentAttackSelection.Montage = CounterMontage;
	CurrentAttackSelection.PlayRate = MikiriCounterPlayRate;
	CurrentAttackSelection.bFaceTargetDuringAttack = false;
	CurrentAttackSelection.FacingDuration = 0.f;
	AttackTime = CounterMontage->GetPlayLength() / FMath::Max(MikiriCounterPlayRate, KINDA_SMALL_NUMBER);
	AttackFacingRemainTime = 0.f;
	bIsAttacking = true;

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, MikiriCounterBlendInTime));
	if (CurrentAnimInstance->Montage_PlayWithBlendSettings(CounterMontage, BlendInSettings, MikiriCounterPlayRate) <= 0.f)
	{
		CurrentAttackMontage = nullptr;
		AttackTime = 0.f;
		return false;
	}

	AddPosture(MikiriCounterPostureGain);
	if (bExecutionReady || bBeingExecuted)
	{
		CurrentAnimInstance->Montage_Stop(0.05f, CounterMontage);
		CurrentAttackMontage = nullptr;
		AttackTime = 0.f;
		return true;
	}

	return true;
}

float AWukongBoss::GetHitReactDuration(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
		return WukongLightHitDuration;
	case EHitReactType::LargeHit_Short:
		return WukongLargeHitDuration;
	case EHitReactType::LargeHit_Long:
		return WukongLargeHitLongDuration;
	default:
		return Super::GetHitReactDuration(HitType);
	}
}

float AWukongBoss::GetHitReactControlLockDuration(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::HeavyHit_A:
	case EHitReactType::HeavyHit_B:
	case EHitReactType::HeavyHit_C:
		return WukongHeavyHitControlLockDuration;
	default:
		return Super::GetHitReactControlLockDuration(HitType);
	}
}

void AWukongBoss::OnHitReactStarted(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection)
{
	Super::OnHitReactStarted(NewHitReact, NewHitDirection);
	if (bPendingParryExchangeChanceGain && PendingParryExchangeHitType == NewHitReact)
	{
		if (GetHitReactMontage(NewHitReact, NewHitDirection))
		{
			AccumulateParryExchangeNormalChance(NewHitReact);
		}

		bPendingParryExchangeChanceGain = false;
		PendingParryExchangeHitType = EHitReactType::None;
	}

	EndBowAttackPresentation();
	LastHitReactDirectionForTurn = NewHitDirection;

	if (CurrentState == EMonsterState::Combat)
	{
		SetWukongCombatState(EWukongCombatState::Hit);
	}
}

void AWukongBoss::OnIncomingHitResolvedWithoutHitReact(EHitReactType HitType)
{
	Super::OnIncomingHitResolvedWithoutHitReact(HitType);
	if (bPendingParryExchangeChanceGain && PendingParryExchangeHitType == HitType)
	{
		bPendingParryExchangeChanceGain = false;
		PendingParryExchangeHitType = EHitReactType::None;
	}
}

bool AWukongBoss::ShouldStartHitReact(EHitReactType IncomingHitReact) const
{
	if (bAirborneHitReactLockWindowActive)
	{
		return false;
	}

	if (CurrentCombatSubState == EWukongCombatState::ParrySuccess &&
		InitiativeState == EWukongInitiativeState::BossParryCounter)
	{
		return false;
	}

	const bool bIncomingStrongHit =
		IncomingHitReact == EHitReactType::HeavyHit_A ||
		IncomingHitReact == EHitReactType::HeavyHit_B ||
		IncomingHitReact == EHitReactType::HeavyHit_C ||
		IncomingHitReact == EHitReactType::LargeHit_Long;
	if (PostHeavyHitSuperArmorRemainTime > 0.f && !bIncomingStrongHit)
	{
		return false;
	}

	return Super::ShouldStartHitReact(IncomingHitReact);
}

ECharacterHitReactDirection AWukongBoss::AdjustHitReactDirection(
	EHitReactType HitType,
	ECharacterHitReactDirection HitDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	if (HitType == EHitReactType::LightHit)
	{
		switch (DefenseRuleData.FrontHitReactDirectionOverride)
		{
		case EJunFrontHitReactDirectionOverride::Front_FL:
			return ECharacterHitReactDirection::Front_FL;
		case EJunFrontHitReactDirectionOverride::Front_FR:
			return ECharacterHitReactDirection::Front_FR;
		case EJunFrontHitReactDirectionOverride::Front_F:
			return ECharacterHitReactDirection::Front_F;
		case EJunFrontHitReactDirectionOverride::Auto:
		default:
			break;
		}

		if (HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ECharacterHitReactDirection::Front_F;
		}
	}

	return Super::AdjustHitReactDirection(HitType, HitDirection, DefenseRuleData);
}

void AWukongBoss::OnHitReactEnded(EHitReactType EndedHitReact)
{
	Super::OnHitReactEnded(EndedHitReact);

	if (bHitReactTurnTransitionInProgress)
	{
		return;
	}

	const bool bWasHeavy =
		EndedHitReact == EHitReactType::HeavyHit_A ||
		EndedHitReact == EHitReactType::HeavyHit_B ||
		EndedHitReact == EHitReactType::HeavyHit_C;

	if (bWasHeavy)
	{
		PostHeavyHitSuperArmorRemainTime = WukongPostHeavyHitSuperArmorDuration;
		AddGameplayTag(JunGameplayTags::State_Condition_PostHitArmor);
	}

	if (CurrentState == EMonsterState::Combat &&
		CurrentCombatSubState == EWukongCombatState::Hit)
	{
		ResetPlannedCombatPlan();

		if (ShouldTurnAfterHitReact(LastHitReactDirectionForTurn))
		{
			StopAIMovement();
			CombatMoveInput = FVector2D::ZeroVector;
			SetDesiredMoveAxes(0.f, 0.f);
			bRunLocomotionRequested = false;
			SetWukongCombatState(EWukongCombatState::Turn);
			return;
		}

		if (TryReactToHitDirection(LastHitReactDirectionForTurn))
		{
			SetWukongCombatState(EWukongCombatState::Idle);
			return;
		}

		SetWukongCombatState(EWukongCombatState::Idle);
	}
}

void AWukongBoss::OnExecutionReadyStarted()
{
	Super::OnExecutionReadyStarted();

	RestoreAttackGroundMotionOverride();
	StopCodeDrivenAttackMove(false);
	EndBowAttackPresentation();
	ClearTimedMontageSuperArmor();
	ClearPlayerPressure();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetPlannedCombatPlan();
	ResetCurrentAttackRuntimeState();
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
		if (CurrentParrySuccessMontage)
		{
			AnimInstance->Montage_Stop(0.05f, CurrentParrySuccessMontage);
		}
	}
	CurrentParrySuccessMontage = nullptr;
	ParrySuccessFallbackRemainTime = 0.f;
	bParryCounterStarted = false;
	bCurrentParryCounterPerfectParried = false;
	bParryCounterFollowUpStarted = false;
	InitiativeState = EWukongInitiativeState::Neutral;
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (CurrentState == EMonsterState::Combat)
	{
		SetWukongCombatState(EWukongCombatState::Idle);
	}
}

void AWukongBoss::OnExecutionReadyEnded(bool bMissedExecution)
{
	Super::OnExecutionReadyEnded(bMissedExecution);

	if (!bMissedExecution)
	{
		return;
	}

	ResetPlannedCombatPlan();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (CurrentState == EMonsterState::Combat)
	{
		SetWukongCombatState(EWukongCombatState::Idle);
	}
}

void AWukongBoss::FinishExecutionRecovery()
{
	Super::FinishExecutionRecovery();

	if (CurrentState != EMonsterState::Combat || Is_Dead())
	{
		return;
	}

	RestoreAttackGroundMotionOverride();
	StopCodeDrivenAttackMove(true);
	EndBowAttackPresentation();
	ClearTimedMontageSuperArmor();
	ClearPlayerPressure();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetReactiveEvadePressure();
	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();
	ResetPlannedCombatPlan();
	ClearNoAttackCandidateApproach();
	ResetParryExchangeCycle();

	bAttackInterruptedByHitReact = false;
	bPostBowCloseRangeApproachInProgress = false;
	bUseNonAttackFallbackUntilAttackCandidateAppears = false;
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	SetWukongCombatState(EWukongCombatState::Idle);
}

FString AWukongBoss::GetMonsterDebugExtraText() const
{
	return FString::Printf(
		TEXT("Initiative: %s | Pressure: %.2f"),
		GetWukongInitiativeDebugText(InitiativeState),
		PlayerPressureRemainTime
	);
}

void AWukongBoss::ResetPlannedCombatPlan()
{
	ClearDelayedNormalAttackRangeLink();
	PlannedActionType = EWukongPlannedActionType::None;
	PlannedAttackType = EWukongPlannedAttackType::None;
	PlannedNormalAttackType = EWukongNormalAttackType::None;
	PlannedNonAttackType = EWukongPlannedNonAttackType::None;
	PlannedMovementDirection = EWukongMovementDirection::None;
	PlannedMovementDuration = 0.f;
	PlannedComboSet = EWukongComboSet::None;
	PlannedComboLength = 0;
	PlannedAttackWaitTime = 0.f;
}

void AWukongBoss::BeginNoAttackCandidateApproach()
{
	if (bNoAttackCandidateApproachInProgress)
	{
		return;
	}

	bNoAttackCandidateApproachInProgress = true;
	NoAttackCandidateApproachRemainTime = NoAttackCandidateApproachDelay;
}

void AWukongBoss::ClearNoAttackCandidateApproach()
{
	bNoAttackCandidateApproachInProgress = false;
	NoAttackCandidateApproachRemainTime = 0.f;
}

bool AWukongBoss::TryQueueReactiveAction(EWukongReactiveActionType ReactiveActionType)
{
	if (ReactiveActionType == EWukongReactiveActionType::None)
	{
		return false;
	}

	CurrentReactiveAction = ReactiveActionType;
	return true;
}

bool AWukongBoss::TryQueueReactiveBackwardEvade()
{
	if (!DashBackwardMontage && !DodgeBackwardMontage)
	{
		return false;
	}

	return TryQueueReactiveAction(EWukongReactiveActionType::EvadeBackward);
}

bool AWukongBoss::TryReactToHitDirection(ECharacterHitReactDirection HitDirection)
{
	switch (HitDirection)
	{
	case ECharacterHitReactDirection::Front_F:
	case ECharacterHitReactDirection::Front_FL:
	case ECharacterHitReactDirection::Front_FR:
		return TryAccumulateReactiveBackwardEvadePressure(HitDirection);
	case ECharacterHitReactDirection::Left_L:
	case ECharacterHitReactDirection::Right_R:
	case ECharacterHitReactDirection::Back_B:
		ResetReactiveEvadePressure();
		return false;
	default:
		return false;
	}
}

bool AWukongBoss::TryAccumulateReactiveBackwardEvadePressure(ECharacterHitReactDirection HitDirection)
{
	const FWukongReactiveActionTuningData* ReactiveEvadeTuningData =
		GetReactiveActionTuningData(EWukongReactiveActionType::EvadeBackward);

	if (!CurrentTarget || !ReactiveEvadeTuningData)
	{
		ResetReactiveEvadePressure();
		return false;
	}

	if (GetTargetDistance2D() > ReactiveEvadeTuningData->TriggerRange)
	{
		ResetReactiveEvadePressure();
		return false;
	}

	++ReactiveBackwardEvadeCloseHitCount;
	const int32 AdditionalCloseHitCount = FMath::Max(0, ReactiveBackwardEvadeCloseHitCount - 1);
	const float RawTriggerChance =
		ReactiveEvadeTuningData->BaseTriggerChance +
		(AdditionalCloseHitCount * ReactiveEvadeTuningData->TriggerChancePerHitCount) +
		ReactiveEvadeTuningData->TriggerChanceBonus;

	ReactiveBackwardEvadeAccumulatedTriggerChance = FMath::Clamp(
		RawTriggerChance,
		0.f,
		ReactiveEvadeTuningData->MaxTriggerChance
	);

	if (FMath::FRand() > ReactiveBackwardEvadeAccumulatedTriggerChance)
	{
		return false;
	}

	if (!TryQueueReactiveBackwardEvade())
	{
		return false;
	}

	ResetReactiveEvadePressure();
	return true;
}

bool AWukongBoss::TryActivateReactiveAction(EWukongReactiveActionType ReactiveActionType)
{
	switch (ReactiveActionType)
	{
	case EWukongReactiveActionType::EvadeBackward:
		if (TryPlanReactiveBackwardEvade())
		{
			ActiveReactiveActionData.Type = ReactiveActionType;
			return true;
		}
		break;
	case EWukongReactiveActionType::None:
	default:
		break;
	}

	return false;
}

bool AWukongBoss::TryConsumeReactiveAction()
{
	if (!HasQueuedReactiveAction())
	{
		return false;
	}

	const EWukongReactiveActionType ReactiveActionToConsume = CurrentReactiveAction;
	ResetReactiveActionState();
	ResetPlannedCombatPlan();

	if (!TryActivateReactiveAction(ReactiveActionToConsume))
	{
		return false;
	}

	SetWukongCombatState(GetReactiveActionTargetState(ReactiveActionToConsume));
	return true;
}

void AWukongBoss::EnsureCombatPlan()
{
	if (bNoAttackCandidateApproachInProgress)
	{
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::None)
	{
		RefreshPlannedCombatPlan();
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::Attack && !HasValidPlannedAttack())
	{
		RefreshPlannedCombatPlan();
		return;
	}

	if (PlannedActionType == EWukongPlannedActionType::NonAttack && !HasValidPlannedMovement())
	{
		RefreshPlannedCombatPlan();
	}
}

void AWukongBoss::OnAttackTick(float DeltaTime)
{
	Super::OnAttackTick(DeltaTime);

	if (CurrentAttackActionType == EWukongActionType::ComboAttack)
	{
		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* ComboMontage = GetPlannedComboMontage();
		if (AnimInstance && ComboMontage && !AnimInstance->Montage_IsPlaying(ComboMontage))
		{
			FinishAttack();
			return;
		}
	}

	if (bPendingCodeDrivenAttackMove)
	{
		CodeDrivenAttackMoveDelayRemainTime = FMath::Max(0.f, CodeDrivenAttackMoveDelayRemainTime - DeltaTime);
		if (CodeDrivenAttackMoveDelayRemainTime <= 0.f)
		{
			StartCodeDrivenAttackMove(CodeDrivenAttackMoveType);
		}
	}

	UpdateCodeDrivenAttackMove(DeltaTime);
	UpdateLightningSwordAirMove(DeltaTime);

	if (bAttackGroundMotionOverrideActive)
	{
		if (AttackGroundMotionOverrideRemainTime > 0.f)
		{
			AttackGroundMotionOverrideRemainTime = FMath::Max(0.f, AttackGroundMotionOverrideRemainTime - DeltaTime);
		}

		if (!GetCharacterMovement()->IsMovingOnGround() || AttackGroundMotionOverrideRemainTime == 0.f)
		{
			StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::DurationEnd);
		}
	}
}

void AWukongBoss::QueueCodeDrivenAttackMove(EWukongNormalAttackType AttackType, float Delay)
{
	const FWukongNormalAttackData* AttackData = GetNormalAttackData(AttackType);
	if (!AttackData || AttackData->MoveSpeed <= 0.f)
	{
		CacheCodeDrivenAttackMoveDebug(EWukongCodeMoveStopReason::InvalidMovement);
		return;
	}

	bPendingCodeDrivenAttackMove = true;
	bCodeDrivenAttackMoveActive = false;
	CodeDrivenAttackMoveType = AttackType;
	CodeDrivenAttackMoveDelayRemainTime = FMath::Max(0.f, Delay);
	CacheCodeDrivenAttackMoveDebug(EWukongCodeMoveStopReason::Started);
	if (CodeDrivenAttackMoveDelayRemainTime <= 0.f)
	{
		StartCodeDrivenAttackMove(AttackType);
	}
}

void AWukongBoss::BeginCodeDrivenAttackMoveWindow(EWukongNormalAttackType AttackType)
{
	if (AttackType == EWukongNormalAttackType::None)
	{
		AttackType = CurrentAttackNormalAttackType;
	}

	bPendingCodeDrivenAttackMove = false;
	CodeDrivenAttackMoveDelayRemainTime = 0.f;
	StartCodeDrivenAttackMove(AttackType);
}

void AWukongBoss::EndCodeDrivenAttackMoveWindow()
{
	StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::DurationEnd);
}

void AWukongBoss::BeginLightningSwordAirMoveWindow(
	EWukongLightningSwordAirMoveMode MoveMode,
	float TargetHeightOffset,
	float RiseSpeed,
	float DiveSpeed,
	float DiveGravityScale,
	bool bLockHorizontalVelocity)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (!bLightningSwordAirMoveActive)
	{
		LightningSwordAirMovePreviousMovementMode = MovementComponent->MovementMode;
		LightningSwordAirMovePreviousCustomMovementMode = MovementComponent->CustomMovementMode;
		LightningSwordAirMovePreviousGravityScale = MovementComponent->GravityScale;
	}

	bLightningSwordAirMoveActive = true;
	LightningSwordAirMoveMode = MoveMode;
	LightningSwordAirMoveTargetZ = (MoveMode == EWukongLightningSwordAirMoveMode::Hover)
		? GetActorLocation().Z
		: GetActorLocation().Z + FMath::Max(0.f, TargetHeightOffset);
	LightningSwordAirMoveRiseSpeed = FMath::Max(0.f, RiseSpeed);
	LightningSwordAirMoveDiveSpeed = FMath::Max(0.f, DiveSpeed);
	LightningSwordAirMoveDiveGravityScale = FMath::Max(0.f, DiveGravityScale);
	bLightningSwordAirMoveLockHorizontalVelocity = bLockHorizontalVelocity;

	FVector NewVelocity = MovementComponent->Velocity;
	if (bLightningSwordAirMoveLockHorizontalVelocity)
	{
		NewVelocity.X = 0.f;
		NewVelocity.Y = 0.f;
	}

	if (MoveMode == EWukongLightningSwordAirMoveMode::Dive)
	{
		MovementComponent->GravityScale = LightningSwordAirMoveDiveGravityScale;
		MovementComponent->SetMovementMode(MOVE_Falling);
		NewVelocity.Z = -LightningSwordAirMoveDiveSpeed;
	}
	else
	{
		MovementComponent->GravityScale = 0.f;
		MovementComponent->SetMovementMode(MOVE_Flying);
		NewVelocity.Z = 0.f;
	}

	MovementComponent->Velocity = NewVelocity;
}

void AWukongBoss::EndLightningSwordAirMoveWindow(EWukongLightningSwordAirMoveMode MoveMode)
{
	if (!bLightningSwordAirMoveActive || LightningSwordAirMoveMode != MoveMode)
	{
		return;
	}

	StopLightningSwordAirMove(true);
}

void AWukongBoss::StartCodeDrivenAttackMove(EWukongNormalAttackType AttackType)
{
	const FWukongNormalAttackData* AttackData = GetNormalAttackData(AttackType);
	if (!CurrentTarget || !AttackData || AttackData->MoveSpeed <= 0.f)
	{
		StopCodeDrivenAttackMove(true, !CurrentTarget ? EWukongCodeMoveStopReason::LostTarget : EWukongCodeMoveStopReason::InvalidMovement);
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::InvalidMovement);
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::InvalidMovement);
		return;
	}

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	const float DistanceToTarget = ToTarget.Size();
	const float StopDistance = FMath::Max(AttackData->MoveStandOffDistance, AttackData->CodeMoveStopDistance);
	if (DistanceToTarget <= StopDistance)
	{
		StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::TooClose);
		return;
	}

	const float GroundMotionOverrideDuration = AttackData->bUseAnimNotifyTiming
		? -1.f
		: AttackData->GroundMotionOverrideDuration;
	ApplyAttackGroundMotionOverride(GroundMotionOverrideDuration);

	bPendingCodeDrivenAttackMove = false;
	bCodeDrivenAttackMoveActive = true;
	CodeDrivenAttackMoveType = AttackType;
	CodeDrivenAttackMoveDelayRemainTime = 0.f;
	CodeDrivenAttackMoveStartLocation = GetActorLocation();
	CacheCodeDrivenAttackMoveDebug(EWukongCodeMoveStopReason::Started);

	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.X = MoveDirection.X * AttackData->MoveSpeed;
	NewVelocity.Y = MoveDirection.Y * AttackData->MoveSpeed;
	MovementComponent->Velocity = NewVelocity;
}

void AWukongBoss::UpdateCodeDrivenAttackMove(float DeltaTime)
{
	if (!bCodeDrivenAttackMoveActive)
	{
		return;
	}

	const FWukongNormalAttackData* AttackData = GetNormalAttackData(CodeDrivenAttackMoveType);
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!CurrentTarget || !AttackData || !MovementComponent)
	{
		StopCodeDrivenAttackMove(true, !CurrentTarget ? EWukongCodeMoveStopReason::LostTarget : EWukongCodeMoveStopReason::InvalidMovement);
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	const float DistanceToTarget = ToTarget.Size();
	const float StopDistance = FMath::Max(AttackData->MoveStandOffDistance, AttackData->CodeMoveStopDistance);
	if (DistanceToTarget <= StopDistance)
	{
		StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::TooClose);
		return;
	}

	const float MaxDistance = AttackData->CodeMoveMaxDistance;
	if (MaxDistance > 0.f &&
		FVector::Dist2D(CodeDrivenAttackMoveStartLocation, GetActorLocation()) >= MaxDistance)
	{
		StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::MaxDistance);
		return;
	}

	if (ToTarget.IsNearlyZero())
	{
		StopCodeDrivenAttackMove(true, EWukongCodeMoveStopReason::InvalidMovement);
		return;
	}

	FVector Velocity = MovementComponent->Velocity;
	FVector MoveDirection = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
	if (MoveDirection.IsNearlyZero() || AttackData->bTrackTargetDuringCodeMove)
	{
		MoveDirection = ToTarget.GetSafeNormal();
	}

	if (AttackData->bTrackTargetDuringCodeMove && AttackData->AirTrackInterpSpeed > 0.f)
	{
		const FVector CurrentHorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
		const FVector TargetHorizontalVelocity = ToTarget.GetSafeNormal() * AttackData->MoveSpeed;
		const FVector NewHorizontalVelocity = FMath::VInterpTo(
			CurrentHorizontalVelocity,
			TargetHorizontalVelocity,
			DeltaTime,
			AttackData->AirTrackInterpSpeed
		);
		Velocity.X = NewHorizontalVelocity.X;
		Velocity.Y = NewHorizontalVelocity.Y;
	}
	else
	{
		Velocity.X = MoveDirection.X * AttackData->MoveSpeed;
		Velocity.Y = MoveDirection.Y * AttackData->MoveSpeed;
	}

	MovementComponent->Velocity = Velocity;
}

void AWukongBoss::StopCodeDrivenAttackMove(bool bClearVelocity, EWukongCodeMoveStopReason StopReason)
{
	CacheCodeDrivenAttackMoveDebug(StopReason);
	bPendingCodeDrivenAttackMove = false;
	bCodeDrivenAttackMoveActive = false;
	CodeDrivenAttackMoveType = EWukongNormalAttackType::None;
	CodeDrivenAttackMoveDelayRemainTime = 0.f;
	CodeDrivenAttackMoveStartLocation = FVector::ZeroVector;

	if (bClearVelocity)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			FVector NewVelocity = MovementComponent->Velocity;
			NewVelocity.X = 0.f;
			NewVelocity.Y = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}

	RestoreAttackGroundMotionOverride();
}

void AWukongBoss::CacheCodeDrivenAttackMoveDebug(EWukongCodeMoveStopReason StopReason)
{
	LastCodeDrivenAttackMoveStopReason = StopReason;

	const FWukongNormalAttackData* AttackData = GetNormalAttackData(CodeDrivenAttackMoveType);
	LastCodeDrivenAttackMoveStopDistance = AttackData
		? FMath::Max(AttackData->MoveStandOffDistance, AttackData->CodeMoveStopDistance)
		: 0.f;
	LastCodeDrivenAttackMoveMaxDistance = AttackData ? AttackData->CodeMoveMaxDistance : 0.f;
	LastCodeDrivenAttackMoveTravelDistance = CodeDrivenAttackMoveStartLocation.IsNearlyZero()
		? 0.f
		: FVector::Dist2D(CodeDrivenAttackMoveStartLocation, GetActorLocation());

	if (CurrentTarget)
	{
		LastCodeDrivenAttackMoveTargetDistance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	}
	else
	{
		LastCodeDrivenAttackMoveTargetDistance = 0.f;
	}
}

void AWukongBoss::UpdateLightningSwordAirMove(float DeltaTime)
{
	if (!bLightningSwordAirMoveActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		StopLightningSwordAirMove(false);
		return;
	}

	FVector NewVelocity = MovementComponent->Velocity;
	if (bLightningSwordAirMoveLockHorizontalVelocity)
	{
		NewVelocity.X = 0.f;
		NewVelocity.Y = 0.f;
	}

	switch (LightningSwordAirMoveMode)
	{
	case EWukongLightningSwordAirMoveMode::LiftAndHover:
	{
		if (MovementComponent->MovementMode != MOVE_Flying)
		{
			MovementComponent->SetMovementMode(MOVE_Flying);
		}

		const float ZDelta = LightningSwordAirMoveTargetZ - GetActorLocation().Z;
		if (ZDelta > 1.f && DeltaTime > KINDA_SMALL_NUMBER)
		{
			NewVelocity.Z = FMath::Min(LightningSwordAirMoveRiseSpeed, ZDelta / DeltaTime);
		}
		else
		{
			NewVelocity.Z = 0.f;
		}
		break;
	}
	case EWukongLightningSwordAirMoveMode::Hover:
	{
		if (MovementComponent->MovementMode != MOVE_Flying)
		{
			MovementComponent->SetMovementMode(MOVE_Flying);
		}

		NewVelocity.Z = 0.f;
		break;
	}
	case EWukongLightningSwordAirMoveMode::Dive:
	{
		if (MovementComponent->IsMovingOnGround())
		{
			StopLightningSwordAirMove(true);
			return;
		}

		if (MovementComponent->MovementMode != MOVE_Falling)
		{
			MovementComponent->SetMovementMode(MOVE_Falling);
		}

		NewVelocity.Z = FMath::Min(NewVelocity.Z, -LightningSwordAirMoveDiveSpeed);
		break;
	}
	default:
		break;
	}

	MovementComponent->Velocity = NewVelocity;
}

void AWukongBoss::StopLightningSwordAirMove(bool bClearVelocity)
{
	if (!bLightningSwordAirMoveActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	bLightningSwordAirMoveActive = false;
	LightningSwordAirMoveMode = EWukongLightningSwordAirMoveMode::LiftAndHover;
	LightningSwordAirMoveTargetZ = 0.f;

	if (MovementComponent)
	{
		MovementComponent->GravityScale = LightningSwordAirMovePreviousGravityScale;

		if (MovementComponent->IsMovingOnGround())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
		else
		{
			MovementComponent->SetMovementMode(
				LightningSwordAirMovePreviousMovementMode,
				LightningSwordAirMovePreviousCustomMovementMode);
		}

		if (bClearVelocity)
		{
			FVector NewVelocity = MovementComponent->Velocity;
			if (bLightningSwordAirMoveLockHorizontalVelocity)
			{
				NewVelocity.X = 0.f;
				NewVelocity.Y = 0.f;
			}
			NewVelocity.Z = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}

	LightningSwordAirMovePreviousMovementMode = MOVE_Walking;
	LightningSwordAirMovePreviousCustomMovementMode = 0;
	LightningSwordAirMovePreviousGravityScale = 1.f;
}

void AWukongBoss::ApplyAttackGroundMotionOverride(float Duration)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || bAttackGroundMotionOverrideActive)
	{
		return;
	}

	DefaultGroundFriction = MovementComponent->GroundFriction;
	DefaultBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
	DefaultBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;

	MovementComponent->GroundFriction = 0.f;
	MovementComponent->BrakingDecelerationWalking = 0.f;
	MovementComponent->BrakingFrictionFactor = 0.f;
	bAttackGroundMotionOverrideActive = true;
	AttackGroundMotionOverrideRemainTime = Duration;
}

void AWukongBoss::RestoreAttackGroundMotionOverride()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !bAttackGroundMotionOverrideActive)
	{
		return;
	}

	MovementComponent->GroundFriction = DefaultGroundFriction;
	MovementComponent->BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	MovementComponent->BrakingFrictionFactor = DefaultBrakingFrictionFactor;
	bAttackGroundMotionOverrideActive = false;
	AttackGroundMotionOverrideRemainTime = 0.f;
}

void AWukongBoss::RefreshPlannedAttackType()
{
	if (!CurrentTarget)
	{
		PlannedAttackType = EWukongPlannedAttackType::None;
		PlannedNormalAttackType = EWukongNormalAttackType::None;
		PlannedComboSet = EWukongComboSet::None;
		PlannedComboLength = 0;
		return;
	}

	TArray<EWukongPlannedAttackType> AttackCandidates;
	const float TargetDistance = GetTargetDistance2D();
	auto AddWeightedAttackCandidate = [&AttackCandidates](EWukongPlannedAttackType AttackType, int32 Weight)
	{
		const int32 ClampedWeight = FMath::Max(1, Weight);
		for (int32 Index = 0; Index < ClampedWeight; ++Index)
		{
			AttackCandidates.Add(AttackType);
		}
	};

	TArray<EWukongNormalAttackType> NormalAttackCandidates;
	CollectNormalAttackCandidates(TargetDistance, false, NormalAttackCandidates);

	if (NormalAttackCandidates.Num() > 0)
	{
		AddWeightedAttackCandidate(EWukongPlannedAttackType::NormalAttack, NormalAttackCandidates.Num());
	}

	if (AttackCandidates.Num() == 0)
	{
		NormalAttackCandidates.Reset();
		CollectNormalAttackCandidates(TargetDistance, true, NormalAttackCandidates);

		if (NormalAttackCandidates.Num() > 0)
		{
			AddWeightedAttackCandidate(EWukongPlannedAttackType::NormalAttack, NormalAttackCandidates.Num());
		}
	}

	if (AttackCandidates.Num() <= 0)
	{
		ResetPlannedCombatPlan();
		return;
	}

	PlannedAttackType = AttackCandidates[FMath::RandRange(0, AttackCandidates.Num() - 1)];
	PlannedNormalAttackType = EWukongNormalAttackType::None;
	PlannedComboSet = EWukongComboSet::None;
	PlannedComboLength = 0;

	if (PlannedAttackType == EWukongPlannedAttackType::NormalAttack)
	{
		if (NormalAttackCandidates.Num() <= 0)
		{
			PlannedAttackType = EWukongPlannedAttackType::None;
			return;
		}

		PlannedNormalAttackType = NormalAttackCandidates[FMath::RandRange(0, NormalAttackCandidates.Num() - 1)];
		return;
	}

	if (PlannedAttackType != EWukongPlannedAttackType::ComboAttack)
	{
		return;
	}
}

void AWukongBoss::RefreshPlannedCombatPlan()
{
	ResetPlannedCombatPlan();

	const bool bShouldCloseAfterBow = ShouldForceCloseAfterBowAttack();

	if (bShouldCloseAfterBow && !HasAnyNonBowAttackCandidateForCurrentDistance())
	{
		bPostBowCloseRangeApproachInProgress = true;
		BeginNoAttackCandidateApproach();
		PlannedActionType = EWukongPlannedActionType::None;
		return;
	}

	if (IsFartherThanAllAttackCandidates())
	{
		BeginNoAttackCandidateApproach();
		PlannedActionType = EWukongPlannedActionType::None;
		return;
	}

	if (!HasAnyAttackCandidateForCurrentDistance())
	{
		if (bUseNonAttackFallbackUntilAttackCandidateAppears)
		{
			PlannedActionType = EWukongPlannedActionType::NonAttack;
			RefreshNoAttackFallbackMovementType();
			return;
		}

		BeginNoAttackCandidateApproach();
		PlannedActionType = EWukongPlannedActionType::None;
		return;
	}

	ClearNoAttackCandidateApproach();
	bUseNonAttackFallbackUntilAttackCandidateAppears = false;
	bShouldStartNoAttackFallbackWithStrafe = true;

	if (bShouldCloseAfterBow)
	{
		bPostBowCloseRangeApproachInProgress = true;
		PlannedActionType = EWukongPlannedActionType::Attack;
		RefreshPlannedAttackType();
		if (PlannedAttackType == EWukongPlannedAttackType::None)
		{
			BeginNoAttackCandidateApproach();
			PlannedActionType = EWukongPlannedActionType::None;
		}
		return;
	}

	if (FMath::FRand() <= AttackPlanWeight)
	{
		PlannedActionType = EWukongPlannedActionType::Attack;
		RefreshPlannedAttackType();
		if (PlannedAttackType == EWukongPlannedAttackType::None)
		{
			PlannedActionType = EWukongPlannedActionType::NonAttack;
			RefreshNoAttackFallbackMovementType();
		}
		return;
	}

	PlannedActionType = EWukongPlannedActionType::NonAttack;
	RefreshPlannedNonAttackType();
}

void AWukongBoss::RefreshPlannedNonAttackType()
{
	PlannedMovementDirection = ChooseStrafeDirectionAvoidingImmediateReverse();
	PlannedNonAttackType = EWukongPlannedNonAttackType::Strafe;
	PlannedMovementDuration = StrafeMinDuration;
}

void AWukongBoss::RefreshNoAttackFallbackMovementType()
{
	if (bShouldStartNoAttackFallbackWithStrafe)
	{
		bShouldStartNoAttackFallbackWithStrafe = false;
		PlannedNonAttackType = EWukongPlannedNonAttackType::Strafe;
		PlannedMovementDirection = ChooseStrafeDirectionAvoidingImmediateReverse();
		PlannedMovementDuration = StrafeMinDuration;
		return;
	}

	TArray<EWukongPlannedNonAttackType> NonAttackCandidates;
	NonAttackCandidates.Add(EWukongPlannedNonAttackType::Strafe);

	if (NonAttackCandidates.Num() <= 0)
	{
		BeginNoAttackCandidateApproach();
		PlannedNonAttackType = EWukongPlannedNonAttackType::None;
		PlannedMovementDuration = 0.f;
		return;
	}

	PlannedNonAttackType = NonAttackCandidates[FMath::RandRange(0, NonAttackCandidates.Num() - 1)];
	PlannedMovementDirection = ChooseStrafeDirectionAvoidingImmediateReverse();

	if (PlannedNonAttackType == EWukongPlannedNonAttackType::Strafe)
	{
		PlannedMovementDuration = StrafeMinDuration;
		return;
	}

	if (UAnimMontage* NonAttackMontage = GetPlannedNonAttackMontage())
	{
		PlannedMovementDuration = NonAttackMontage->GetPlayLength();
		return;
	}

	BeginNoAttackCandidateApproach();
	PlannedNonAttackType = EWukongPlannedNonAttackType::None;
	PlannedMovementDuration = 0.f;
}

bool AWukongBoss::TryPlanReactiveBackwardEvade()
{
	const FWukongReactiveActionTuningData* ReactiveEvadeTuningData =
		GetReactiveActionTuningData(EWukongReactiveActionType::EvadeBackward);
	if (!ReactiveEvadeTuningData)
	{
		return false;
	}

	const bool bUsedDashRecently = WasRecentlyUsed(EWukongActionType::Dash, 1);
	const bool bUsedDodgeRecently = WasRecentlyUsed(EWukongActionType::Dodge, 1);

	const bool bCanUseDash = DashBackwardMontage && !bUsedDashRecently;
	const bool bCanUseDodge = DodgeBackwardMontage && !bUsedDodgeRecently;
	const bool bCanFallbackDash = DashBackwardMontage != nullptr;
	const bool bCanFallbackDodge = DodgeBackwardMontage != nullptr;

	EWukongPlannedNonAttackType SelectedEvadeType = EWukongPlannedNonAttackType::None;
	const EWukongPlannedNonAttackType PrimaryNonAttackType = ReactiveEvadeTuningData->PrimaryNonAttackType;
	const EWukongPlannedNonAttackType SecondaryNonAttackType = ReactiveEvadeTuningData->SecondaryNonAttackType;
	const bool bPreferPrimaryType = FMath::FRand() <= ReactiveEvadeTuningData->PrimaryNonAttackTypeChance;
	const EWukongPlannedNonAttackType PreferredType = bPreferPrimaryType ? PrimaryNonAttackType : SecondaryNonAttackType;
	const EWukongPlannedNonAttackType FallbackType = bPreferPrimaryType ? SecondaryNonAttackType : PrimaryNonAttackType;

	auto CanUseNonAttackType = [bCanUseDash, bCanUseDodge](EWukongPlannedNonAttackType NonAttackType)
	{
		switch (NonAttackType)
		{
		case EWukongPlannedNonAttackType::Dash:
			return bCanUseDash;
		case EWukongPlannedNonAttackType::Dodge:
			return bCanUseDodge;
		default:
			return false;
		}
	};

	auto CanFallbackNonAttackType = [bCanFallbackDash, bCanFallbackDodge](EWukongPlannedNonAttackType NonAttackType)
	{
		switch (NonAttackType)
		{
		case EWukongPlannedNonAttackType::Dash:
			return bCanFallbackDash;
		case EWukongPlannedNonAttackType::Dodge:
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

	if (SelectedEvadeType == EWukongPlannedNonAttackType::None)
	{
		return false;
	}

	ActiveReactiveActionData.NonAttackType = SelectedEvadeType;
	ActiveReactiveActionData.MovementDirection = ReactiveEvadeTuningData->MovementDirection;
	ActiveReactiveActionData.Duration = 0.f;
	return true;
}
