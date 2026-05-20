#include "Character/WukongBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/JunPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Player/JunPlayerController.h"
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
		EWukongNormalAttackType::SpinSlash
	};
}

AWukongBoss::AWukongBoss()
{
	MaxLifeCount = 3;
	MaxPosture = 700.f;
	bDisablePostureGain = false;
	bStartWithCutsceneWait = true;
	bStartWithPatrol = false;

	WalkMoveSpeed = 200.f;
	RunMoveSpeed = 700.f;

	ComboAttackA.PlayRate = 0.8f;
	ComboAttackB.PlayRate = 0.8f;

	JumpAttack.MinRange = 0.f;
	JumpAttack.MaxRange = 700.f;
	JumpAttack.CandidateMaxRange = 900.f;
	JumpAttack.bFaceTargetDuringAttack = true;
	JumpAttack.PlayRate = 1.4f;
	JumpAttack.MoveStartTimeAtPlayRateOne = 0.14f;
	JumpAttack.MoveSpeed = 1200.f;
	JumpAttack.MoveStandOffDistance = 20.f;
	JumpAttack.GroundMotionOverrideDuration = 0.8f;
	JumpAttack.AirTrackInterpSpeed = 8.f;
	JumpAttack.CodeMoveStopDistance = 20.f;
	JumpAttack.CodeMoveMaxDistance = 700.f;
	JumpAttack.bTryTurnAfterAttack = true;
	JumpAttack.PostAttackTurnStartAngle = TurnStartAngle;

	ChargeAttack.MinRange = 250.f;
	ChargeAttack.MaxRange = 700.f;
	ChargeAttack.CandidateMaxRange = 900.f;
	ChargeAttack.bFaceTargetDuringAttack = true;
	ChargeAttack.bUseAnimNotifyTiming = true;
	ChargeAttack.MoveStartTimeAtPlayRateOne = 0.28f;
	ChargeAttack.MoveSpeed = 1350.f;
	ChargeAttack.MoveStandOffDistance = 120.f;
	ChargeAttack.GroundMotionOverrideDuration = 0.45f;
	ChargeAttack.AirTrackInterpSpeed = 10.f;
	ChargeAttack.CodeMoveStopDistance = 120.f;
	ChargeAttack.CodeMoveMaxDistance = 600.f;
	ChargeAttack.bTryTurnAfterAttack = true;
	ChargeAttack.PostAttackTurnStartAngle = 45.f;

	DodgeAttack.MinRange = 450.f;
	DodgeAttack.MaxRange = 700.f;
	DodgeAttack.CandidateMaxRange = 900.f;
	DodgeAttack.bFaceTargetDuringAttack = true;
	DodgeAttack.PlayRate = 1.3f;
	DodgeAttack.bTryTurnAfterAttack = true;
	DodgeAttack.PostAttackTurnStartAngle = 45.f;
	DodgeAttack.SelectionWeight = 1;

	NinjaBAttack.bUseAnimNotifyTiming = true;
	NinjaBAttack.MoveStartTimeAtPlayRateOne = 0.35f;
	NinjaBAttack.MoveSpeed = 1150.f;
	NinjaBAttack.MoveStandOffDistance = 120.f;
	NinjaBAttack.GroundMotionOverrideDuration = 0.5f;
	NinjaBAttack.AirTrackInterpSpeed = 8.f;
	NinjaBAttack.CodeMoveStopDistance = 120.f;
	NinjaBAttack.CodeMoveMaxDistance = 500.f;

	BowBackDashAttack.MinRange = 100.f;
	BowBackDashAttack.MaxRange = 500.f;
	BowBackDashAttack.CandidateMaxRange = 800.f;
	BowBackDashAttack.bFaceTargetDuringAttack = true;
	BowBackDashAttack.FacingDuration = 0.35f;
	BowBackDashAttack.FacingInterpSpeed = 18.f;
	BowBackDashAttack.SelectionWeight = 1;
	BowBackDashAttack.bTryTurnAfterAttack = true;
	BowBackDashAttack.PostAttackTurnStartAngle = 45.f;

	Bow4ComboAttack.MinRange = 500.f;
	Bow4ComboAttack.MaxRange = 1600.f;
	Bow4ComboAttack.CandidateMaxRange = 1800.f;
	Bow4ComboAttack.bFaceTargetDuringAttack = true;
	Bow4ComboAttack.FacingDuration = 0.5f;
	Bow4ComboAttack.FacingInterpSpeed = 16.f;
	Bow4ComboAttack.SelectionWeight = 1;
	Bow4ComboAttack.bTryTurnAfterAttack = true;
	Bow4ComboAttack.PostAttackTurnStartAngle = 45.f;

	BowHeavyAttack.MinRange = 100.f;
	BowHeavyAttack.MaxRange = 1600.f;
	BowHeavyAttack.CandidateMaxRange = 1800.f;
	BowHeavyAttack.bFaceTargetDuringAttack = true;
	BowHeavyAttack.FacingDuration = 0.65f;
	BowHeavyAttack.FacingInterpSpeed = 16.f;
	BowHeavyAttack.SelectionWeight = 1;
	BowHeavyAttack.bTryTurnAfterAttack = true;
	BowHeavyAttack.PostAttackTurnStartAngle = 45.f;

	FastComeSlashAttack.MinRange = 200.f;
	FastComeSlashAttack.MaxRange = 700.f;
	FastComeSlashAttack.CandidateMaxRange = 900.f;
	FastComeSlashAttack.bFaceTargetDuringAttack = true;
	FastComeSlashAttack.FacingDuration = 0.35f;
	FastComeSlashAttack.FacingInterpSpeed = 18.f;
	FastComeSlashAttack.SelectionWeight = 1;
	FastComeSlashAttack.bTryTurnAfterAttack = true;
	FastComeSlashAttack.PostAttackTurnStartAngle = 45.f;

	SlowComeSlashAttack.MinRange = 120.f;
	SlowComeSlashAttack.MaxRange = 520.f;
	SlowComeSlashAttack.CandidateMaxRange = 700.f;
	SlowComeSlashAttack.bFaceTargetDuringAttack = true;
	SlowComeSlashAttack.FacingDuration = 0.35f;
	SlowComeSlashAttack.FacingInterpSpeed = 16.f;
	SlowComeSlashAttack.SelectionWeight = 1;
	SlowComeSlashAttack.bTryTurnAfterAttack = true;
	SlowComeSlashAttack.PostAttackTurnStartAngle = 45.f;

	FakeDownSlashAttack.MinRange = 180.f;
	FakeDownSlashAttack.MaxRange = 650.f;
	FakeDownSlashAttack.CandidateMaxRange = 850.f;
	FakeDownSlashAttack.bFaceTargetDuringAttack = true;
	FakeDownSlashAttack.FacingDuration = 0.4f;
	FakeDownSlashAttack.FacingInterpSpeed = 16.f;
	FakeDownSlashAttack.SelectionWeight = 1;
	FakeDownSlashAttack.bTryTurnAfterAttack = true;
	FakeDownSlashAttack.PostAttackTurnStartAngle = 45.f;

	LionSlashAttack.MinRange = 150.f;
	LionSlashAttack.MaxRange = 600.f;
	LionSlashAttack.CandidateMaxRange = 800.f;
	LionSlashAttack.bFaceTargetDuringAttack = true;
	LionSlashAttack.FacingDuration = 0.35f;
	LionSlashAttack.FacingInterpSpeed = 18.f;
	LionSlashAttack.SelectionWeight = 1;
	LionSlashAttack.bTryTurnAfterAttack = true;
	LionSlashAttack.PostAttackTurnStartAngle = 45.f;

	SpinSlashAttack.MinRange = 220.f;
	SpinSlashAttack.MaxRange = 750.f;
	SpinSlashAttack.CandidateMaxRange = 950.f;
	SpinSlashAttack.bFaceTargetDuringAttack = true;
	SpinSlashAttack.FacingDuration = 0.35f;
	SpinSlashAttack.FacingInterpSpeed = 18.f;
	SpinSlashAttack.SelectionWeight = 1;
	SpinSlashAttack.bTryTurnAfterAttack = true;
	SpinSlashAttack.PostAttackTurnStartAngle = 45.f;
}

void AWukongBoss::EnterCombatState()
{
	Super::EnterCombatState();
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

void AWukongBoss::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	Super::HandleGameplayEventNotify(EventTag);

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
		InitiativeState = EWukongInitiativeState::BossPattern;
		if (IsBowNormalAttackType(CurrentAttackNormalAttackType))
		{
			BeginBowAttackPresentation();
			bPostBowCloseRangeApproachInProgress = bForceApproachAfterBowAttack;
		}
		else
		{
			bPostBowCloseRangeApproachInProgress = false;
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
			bPostBowCloseRangeApproachInProgress && !HasAnyNonBowAttackCandidateForCurrentDistance();
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
	const EWukongMovementDirection ResolvedEvadeDirection =
		bUsingReactiveEvade ? ActiveReactiveActionData.MovementDirection : PlannedMovementDirection;
	const float ResolvedEvadeDuration =
		bUsingReactiveEvade ? ActiveReactiveActionData.Duration : PlannedMovementDuration;
	const bool bIsBackwardDodgeEvade =
		ResolvedEvadeType == EWukongPlannedNonAttackType::Dodge &&
		ResolvedEvadeDirection == EWukongMovementDirection::Backward;

	if (CurrentTarget && !bIsBackwardDodgeEvade)
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

	const bool bCanUseCombo = HasAnyComboMontage();
	const float MaxComboCandidateRange = GetMaxEnabledComboCandidateRange();
	if (bCanUseCombo && IsWithinRange(0.f, MaxComboCandidateRange))
	{
		return true;
	}

	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
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
		return true;
	}

	switch (ComboSet)
	{
	case EWukongComboSet::ComboA:
		return bTestEnableComboA;
	case EWukongComboSet::ComboB:
		return bTestEnableComboB;
	case EWukongComboSet::None:
	default:
		return false;
	}
}

bool AWukongBoss::IsNormalAttackEnabledByTestFilter(EWukongNormalAttackType NormalAttackType) const
{
	if (!bUseAttackPatternTestFilter)
	{
		return true;
	}

	switch (NormalAttackType)
	{
	case EWukongNormalAttackType::JumpAttack:
		return bTestEnableJumpAttack;
	case EWukongNormalAttackType::ChargeAttack:
		return bTestEnableChargeAttack;
	case EWukongNormalAttackType::DodgeAttack:
		return bTestEnableDodgeAttack;
	case EWukongNormalAttackType::NinjaA:
		return bTestEnableNinjaA;
	case EWukongNormalAttackType::NinjaB:
		return bTestEnableNinjaB;
	case EWukongNormalAttackType::Execution:
		return bTestEnableExecution;
	case EWukongNormalAttackType::FastComeSlash:
		return bTestEnableFastComeSlash;
	case EWukongNormalAttackType::SlowComeSlash:
		return bTestEnableSlowComeSlash;
	case EWukongNormalAttackType::FakeDownSlash:
		return bTestEnableFakeDownSlash;
	case EWukongNormalAttackType::LionSlash:
		return bTestEnableLionSlash;
	case EWukongNormalAttackType::SpinSlash:
		return bTestEnableSpinSlash;
	case EWukongNormalAttackType::Bow4Combo:
		return bTestEnableBow4Combo;
	case EWukongNormalAttackType::BowBackDashAttack:
		return bTestEnableBowBackDashAttack;
	case EWukongNormalAttackType::BowHeavyAttack:
		return bTestEnableBowHeavyAttack;
	case EWukongNormalAttackType::None:
	default:
		return false;
	}
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
	float MaxCandidateRange = GetMaxEnabledComboCandidateRange();
	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
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

	if (HasAnyComboMontage() && IsWithinRange(0.f, GetMaxEnabledComboCandidateRange()))
	{
		return true;
	}

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

void AWukongBoss::CollectNormalAttackCandidates(float TargetDistance, bool bIgnoreRepeat, TArray<EWukongNormalAttackType>& OutCandidates) const
{
	auto IsWithinRange = [TargetDistance](float MinRange, float MaxRange)
	{
		return TargetDistance >= MinRange && TargetDistance <= MaxRange;
	};

	for (const EWukongNormalAttackType NormalAttackType : AllWukongNormalAttackTypes)
	{
		const bool bIsBowAttack = IsBowNormalAttackType(NormalAttackType);
		if (bIsBowAttack && (bPostBowCloseRangeApproachInProgress || WasRecentlyUsedBowNormalAttack(1)))
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
	bool bUseTestFilter)
{
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
		CurrentCombatSubState == EWukongCombatState::Recovery;

	if (CurrentState != EMonsterState::Combat ||
		!bCanLinkFromCurrentState ||
		IsInHitReact() ||
		NextAttackType == EWukongNormalAttackType::None ||
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
		return false;
	}

	const bool bNextAttackIsBow = IsBowNormalAttackType(NextAttackType);
	if (!bNextAttackIsBow && IsBowNormalAttackType(CurrentAttackNormalAttackType))
	{
		EndBowAttackPresentation();
	}

	if (CurrentAttackMontage)
	{
		CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, BlendOutTime), CurrentAttackMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();

	CurrentCombatSubState = EWukongCombatState::Attack;
	CombatSubStateElapsedTime = 0.f;
	CurrentAttackActionType = EWukongActionType::NormalAttack;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = NextAttackType;
	PlannedAttackType = EWukongPlannedAttackType::NormalAttack;
	PlannedNormalAttackType = NextAttackType;

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

bool AWukongBoss::TryStartParryBackJump()
{
	if (CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::ParrySuccess ||
		bParryCounterStarted ||
		IsInHitReact() ||
		!ParryBackJumpMontage)
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
	CurrentParrySuccessMontage = ParryBackJumpMontage.Get();
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
	}
}

bool AWukongBoss::WasRecentlyUsed(EWukongActionType ActionType, int32 Depth) const
{
	const int32 NumToCheck = FMath::Min(Depth, RecentActionHistory.Num());
	for (int32 Index = RecentActionHistory.Num() - 1; Index >= RecentActionHistory.Num() - NumToCheck; --Index)
	{
		if (RecentActionHistory[Index].ActionType == ActionType)
		{
			return true;
		}
	}

	return false;
}

bool AWukongBoss::WasRecentlyUsed(EWukongComboSet ComboSet, int32 ComboLength, int32 Depth) const
{
	const int32 NumToCheck = FMath::Min(Depth, RecentActionHistory.Num());
	for (int32 Index = RecentActionHistory.Num() - 1; Index >= RecentActionHistory.Num() - NumToCheck; --Index)
	{
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
	const int32 NumToCheck = FMath::Min(Depth, RecentActionHistory.Num());
	for (int32 Index = RecentActionHistory.Num() - 1; Index >= RecentActionHistory.Num() - NumToCheck; --Index)
	{
		if (RecentActionHistory[Index].NormalAttackType == AttackType)
		{
			return true;
		}
	}

	return false;
}

bool AWukongBoss::WasRecentlyUsedBowNormalAttack(int32 Depth) const
{
	const int32 NumToCheck = FMath::Min(Depth, RecentActionHistory.Num());
	for (int32 Index = RecentActionHistory.Num() - 1; Index >= RecentActionHistory.Num() - NumToCheck; --Index)
	{
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
}

void AWukongBoss::ResetCurrentAttackRuntimeState()
{
	EndBowAttackPresentation();
	Super::ResetCurrentAttackRuntimeState();
	CurrentAttackActionType = EWukongActionType::None;
	CurrentAttackComboSet = EWukongComboSet::None;
	CurrentAttackComboLength = 0;
	CurrentAttackNormalAttackType = EWukongNormalAttackType::None;
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
	if (CurrentState != EMonsterState::Combat ||
		!IsParryableHitType(HitType) ||
		!DamageCauser ||
		CurrentAttackActionType == EWukongActionType::ParryCounter ||
		IsInHitReact() ||
		bBeingExecuted ||
		bExecutionReady ||
		!CanBeInterruptedBy(HitType) ||
		!ShouldStartHitReact(HitType))
	{
		return false;
	}

	const ECharacterHitReactDirection HitDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	if (HitDirection == ECharacterHitReactDirection::Back_B)
	{
		return false;
	}

	return true;
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
		HitType == EHitReactType::LightHit ? LightHitPerfectParryChance : StrongHitPerfectParryChance,
		0.f,
		1.f);
}

float AWukongBoss::GetNormalParryChanceForHitType(EHitReactType HitType) const
{
	const float PerfectChance = GetPerfectParryChanceForHitType(HitType);
	const float RawNormalChance = HitType == EHitReactType::LightHit ? LightHitNormalParryChance : StrongHitNormalParryChance;
	return FMath::Clamp(RawNormalChance, 0.f, 1.f - PerfectChance);
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
	const bool bUseUp = FMath::RandBool();

	UAnimMontage* SelectedMontage = nullptr;
	if (bUseLeft)
	{
		SelectedMontage = bUseUp
			? (ParrySuccessLUpMontage ? ParrySuccessLUpMontage.Get() : ParrySuccessLDownMontage.Get())
			: (ParrySuccessLDownMontage ? ParrySuccessLDownMontage.Get() : ParrySuccessLUpMontage.Get());
	}
	else
	{
		SelectedMontage = bUseUp
			? (ParrySuccessRUpMontage ? ParrySuccessRUpMontage.Get() : ParrySuccessRDownMontage.Get())
			: (ParrySuccessRDownMontage ? ParrySuccessRDownMontage.Get() : ParrySuccessRUpMontage.Get());
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

	CachedParryParticleComponent->Activate(true);

	if (bAutoDeactivateParryParticle && GetWorld())
	{
		TWeakObjectPtr<UNiagaraComponent> WeakComponent = CachedParryParticleComponent;
		FTimerHandle DeactivateTimerHandle;
		GetWorldTimerManager().SetTimer(
			DeactivateTimerHandle,
			[WeakComponent]()
			{
				if (UNiagaraComponent* NiagaraComponent = WeakComponent.Get())
				{
					NiagaraComponent->Deactivate();
				}
			},
			ParryParticleAutoDeactivateDelay,
			false
		);
	}
}

void AWukongBoss::StartParrySuccessAgainstIncomingHit(
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
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
	if (bParryCounterStarted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EWukongCombatState::ParrySuccess)
	{
		return false;
	}

	const float CounterChance = FMath::Clamp(ParryCounterDecisionCounterChance, 0.f, 1.f);
	const bool bChooseCounter = !ParryBackJumpMontage || FMath::FRand() <= CounterChance;
	if (bChooseCounter)
	{
		return TryStartParryCounterAttack();
	}

	if (TryStartParryBackJump())
	{
		return true;
	}

	return TryStartParryCounterAttack();
}

bool AWukongBoss::TryStartParryCounterAttack()
{
	if (bParryCounterStarted ||
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
	if (bParryCounterFollowUpStarted ||
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

void AWukongBoss::FinishParrySuccessState()
{
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWukongBoss::OnParrySuccessMontageEnded);
	}

	CurrentParrySuccessMontage = nullptr;
	ParrySuccessFallbackRemainTime = 0.f;
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
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	if (!CanParryIncomingHit(HitType, DamageCauser, SwingDirection))
	{
		return false;
	}

	const float PerfectChance = GetPerfectParryChanceForHitType(HitType);
	const float NormalChance = GetNormalParryChanceForHitType(HitType);
	const float Roll = FMath::FRand();

	if (Roll <= PerfectChance)
	{
		SetPendingDefenseSoundType(EJunDefenseSoundType::PerfectParry, bPlayDefenseSoundImmediately);
		StartParrySuccessAgainstIncomingHit(DamageCauser, SwingDirection, DefenseKnockbackData);
		return true;
	}

	if (Roll <= PerfectChance + NormalChance)
	{
		SetPendingDefenseSoundType(EJunDefenseSoundType::NormalParry, bPlayDefenseSoundImmediately);
		AddPosture(NormalParryPostureGain);
		const EWukongInitiativeState SavedInitiativeState = InitiativeState;
		StartParrySuccessAgainstIncomingHit(DamageCauser, SwingDirection, DefenseKnockbackData);
		InitiativeState = SavedInitiativeState;
		bParryCounterStarted = true;
		bCurrentParryCounterPerfectParried = true;
		return true;
	}

	return false;
}

void AWukongBoss::NotifyAttackParriedBy(AJunPlayer* Parrier, float PostureScale)
{
	Super::NotifyAttackParriedBy(Parrier, PostureScale);

	// 일반 패턴 공격은 플레이어가 패리해도 공격권을 넘기지 않는다.
	// 보스가 플레이어 공격을 완벽 패리해서 얻은 반격만 다시 빼앗길 수 있다.
	if (CurrentAttackActionType != EWukongActionType::ParryCounter ||
		InitiativeState != EWukongInitiativeState::BossParryCounter)
	{
		return;
	}

	StartPlayerPressure();
	bCurrentParryCounterPerfectParried = true;
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
	EndBowAttackPresentation();
	LastHitReactDirectionForTurn = NewHitDirection;

	if (CurrentState == EMonsterState::Combat)
	{
		SetWukongCombatState(EWukongCombatState::Hit);
	}
}

bool AWukongBoss::ShouldStartHitReact(EHitReactType IncomingHitReact) const
{
	if (CurrentCombatSubState == EWukongCombatState::ParrySuccess &&
		InitiativeState == EWukongInitiativeState::BossParryCounter)
	{
		return false;
	}

	if (PostHeavyHitSuperArmorRemainTime > 0.f)
	{
		return false;
	}

	return Super::ShouldStartHitReact(IncomingHitReact);
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
	PlannedActionType = EWukongPlannedActionType::None;
	PlannedAttackType = EWukongPlannedAttackType::None;
	PlannedNormalAttackType = EWukongNormalAttackType::None;
	PlannedNonAttackType = EWukongPlannedNonAttackType::None;
	PlannedMovementDirection = EWukongMovementDirection::None;
	PlannedMovementDuration = 0.f;
	PlannedComboSet = EWukongComboSet::None;
	PlannedComboLength = 0;
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
	auto IsWithinRange = [TargetDistance](float MinRange, float MaxRange)
	{
		return TargetDistance >= MinRange && TargetDistance <= MaxRange;
	};

	const bool bCanUseCombo = HasAnyComboMontage();
	const int32 ComboAttackSelectionWeight = GetComboAttackSelectionWeight();
	const float MaxComboCandidateRange = GetMaxEnabledComboCandidateRange();
	if (bCanUseCombo && IsWithinRange(0.f, MaxComboCandidateRange) && !WasRecentlyUsed(EWukongActionType::ComboAttack, 1))
	{
		AddWeightedAttackCandidate(EWukongPlannedAttackType::ComboAttack, ComboAttackSelectionWeight);
	}

	TArray<EWukongNormalAttackType> NormalAttackCandidates;
	CollectNormalAttackCandidates(TargetDistance, false, NormalAttackCandidates);

	if (NormalAttackCandidates.Num() > 0)
	{
		AddWeightedAttackCandidate(EWukongPlannedAttackType::NormalAttack, NormalAttackCandidates.Num());
	}

	if (AttackCandidates.Num() == 0)
	{
		if (bCanUseCombo && IsWithinRange(0.f, MaxComboCandidateRange))
		{
			AddWeightedAttackCandidate(EWukongPlannedAttackType::ComboAttack, ComboAttackSelectionWeight);
		}

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

	TArray<EWukongComboSet> ComboSetCandidates;
	if (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboA) && IsComboAttackUsable(ComboAttackA))
	{
		for (int32 WeightIndex = 0; WeightIndex < FMath::Max(1, ComboAttackA.SelectionWeight); ++WeightIndex)
		{
			ComboSetCandidates.Add(EWukongComboSet::ComboA);
		}
	}
	if (IsComboSetEnabledByTestFilter(EWukongComboSet::ComboB) && IsComboAttackUsable(ComboAttackB))
	{
		for (int32 WeightIndex = 0; WeightIndex < FMath::Max(1, ComboAttackB.SelectionWeight); ++WeightIndex)
		{
			ComboSetCandidates.Add(EWukongComboSet::ComboB);
		}
	}

	if (ComboSetCandidates.Num() <= 0)
	{
		PlannedAttackType = EWukongPlannedAttackType::None;
		return;
	}

	PlannedComboSet = ComboSetCandidates[FMath::RandRange(0, ComboSetCandidates.Num() - 1)];

	const FWukongComboAttackData* SelectedComboAttackData = GetComboAttackData(PlannedComboSet);
	if (!SelectedComboAttackData)
	{
		PlannedAttackType = EWukongPlannedAttackType::None;
		PlannedComboSet = EWukongComboSet::None;
		return;
	}

	PlannedComboLength = 1;
}

void AWukongBoss::RefreshPlannedCombatPlan()
{
	ResetPlannedCombatPlan();

	const bool bShouldCloseAfterBow =
		bPostBowCloseRangeApproachInProgress || WasRecentlyUsedBowNormalAttack(1);

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
