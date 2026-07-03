#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunCombatVFXSubsystem.h"
#include "Weapon/ArrowProjectile.h"

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

	const TCHAR* GetBossCodeMoveStopReasonDebugText(EBossCodeMoveStopReason StopReason)
	{
		switch (StopReason)
		{
		case EBossCodeMoveStopReason::Started:
			return TEXT("Started");
		case EBossCodeMoveStopReason::TooClose:
			return TEXT("TooClose");
		case EBossCodeMoveStopReason::MaxDistance:
			return TEXT("MaxDistance");
		case EBossCodeMoveStopReason::DurationEnd:
			return TEXT("DurationEnd");
		case EBossCodeMoveStopReason::LostTarget:
			return TEXT("LostTarget");
		case EBossCodeMoveStopReason::InvalidMovement:
			return TEXT("InvalidMovement");
		case EBossCodeMoveStopReason::Interrupted:
			return TEXT("Interrupted");
		case EBossCodeMoveStopReason::None:
		default:
			return TEXT("None");
		}
	}

	constexpr EBossNormalAttackType AllBossNormalAttackTypes[] = {
		EBossNormalAttackType::JumpAttack,
		EBossNormalAttackType::ChargeAttack,
		EBossNormalAttackType::DodgeAttack,
		EBossNormalAttackType::NinjaA,
		EBossNormalAttackType::NinjaB,
		EBossNormalAttackType::Execution,
		EBossNormalAttackType::Bow4Combo,
		EBossNormalAttackType::BowBackDashAttack,
		EBossNormalAttackType::BowHeavyAttack,
		EBossNormalAttackType::FastComeSlash,
		EBossNormalAttackType::SlowComeSlash,
		EBossNormalAttackType::FakeDownSlash,
		EBossNormalAttackType::LionSlash,
		EBossNormalAttackType::SpinSlash,
		EBossNormalAttackType::LightningSword
	};
}

AJunBoss::AJunBoss()
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

void AJunBoss::EnterCombatState()
{
	Super::EnterCombatState();
	ResetPlannedCombatPlan();
	ResetCurrentAttackRuntimeState();
	ResetParryExchangeCycle();
	RecentActionHistory.Reset();
	LastStartedNormalAttackType = EBossNormalAttackType::None;
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetReactiveEvadePressure();
	ClearPlayerPressure();
	ClearNoAttackCandidateApproach();
	bUseNonAttackFallbackUntilAttackCandidateAppears = false;
	bPostBowCloseRangeApproachInProgress = false;
	CloseRangeBackDashCooldownRemainTime = 0.f;
	TransitionBossCombatState(EBossCombatState::Approach, EBossCombatStateChangeReason::CombatEntered);
}

void AJunBoss::EnterPlayerDeathWait()
{
	Super::EnterPlayerDeathWait();
	ResetPlannedCombatPlan();
	ResetParryExchangeCycle();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	ResetPendingAttackMotionState();
	TransitionBossCombatState(
		EBossCombatState::Idle,
		EBossCombatStateChangeReason::PlayerDeathWait,
		EBossCombatStateTransitionMode::SynchronizeOnly);
}

void AJunBoss::ResumeAfterPlayerFakeDeath()
{
	Super::ResumeAfterPlayerFakeDeath();
	ResetPlannedCombatPlan();
	ResetParryExchangeCycle();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::PlayerDeathReset);
}

void AJunBoss::ResetAfterPlayerRealDeath()
{
	Super::ResetAfterPlayerRealDeath();
	ResetPlannedCombatPlan();
	ResetParryExchangeCycle();
	ResetReactiveActionState();
	ResetActiveReactiveActionState();
	RecentActionHistory.Reset();
	LastStartedNormalAttackType = EBossNormalAttackType::None;
	TransitionBossCombatState(
		EBossCombatState::Idle,
		EBossCombatStateChangeReason::PlayerDeathReset,
		EBossCombatStateTransitionMode::SynchronizeOnly);
}

void AJunBoss::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	Super::HandleGameplayEventNotify(EventTag);

	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_ParryCounter))
	{
		TryStartParryCounterDecision();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_ParryCounterFollowUpDecision))
	{
		TryStartParryCounterFollowUp();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_BowAttackRestoreWeapon))
	{
		EndBowAttackPresentation();
	}
}

void AJunBoss::ApplyTimedMontageSuperArmor(float MontageDuration, float DurationScale)
{
	const float ResolvedDurationScale = DurationScale >= 0.f ? DurationScale : MontageSuperArmorDurationScale;
	const float SuperArmorDuration = FMath::Max(0.f, MontageDuration * ResolvedDurationScale);
	TimedMontageSuperArmorRemainTime = SuperArmorDuration;

	if (TimedMontageSuperArmorRemainTime > 0.f)
	{
		AddGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
	}
}

void AJunBoss::UpdateTimedMontageSuperArmor(float DeltaTime)
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

void AJunBoss::ClearTimedMontageSuperArmor()
{
	TimedMontageSuperArmorRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
}

void AJunBoss::BeginAttackSuperArmorWindow()
{
	TimedMontageSuperArmorRemainTime = 0.f;
	AddGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
}

void AJunBoss::EndAttackSuperArmorWindow()
{
	TimedMontageSuperArmorRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
}

void AJunBoss::BeginAirborneHitReactLockWindow()
{
	bAirborneHitReactLockWindowActive = true;
}

void AJunBoss::EndAirborneHitReactLockWindow()
{
	bAirborneHitReactLockWindowActive = false;
}

FVector AJunBoss::GetLockOnTargetPoint() const
{
	const FVector DefaultTargetPoint = Super::GetLockOnTargetPoint();

	if (CurrentAttackRuntime.ActionType != EBossActionType::NormalAttack ||
		CurrentAttackRuntime.NormalAttackType != EBossNormalAttackType::JumpAttack)
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

bool AJunBoss::ShouldReduceFootPlacementAlpha() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	const bool bPlayingBackwardRoll =
		DodgeBackwardMontage && AnimInstance->Montage_IsActive(DodgeBackwardMontage);
	const bool bPlayingDodgeAttack =
		CurrentAttackRuntime.NormalAttackType == EBossNormalAttackType::DodgeAttack &&
		CurrentAttackMontage && AnimInstance->Montage_IsActive(CurrentAttackMontage);

	return bPlayingBackwardRoll || bPlayingDodgeAttack;
}

FString AJunBoss::GetMonsterDebugExtraText() const
{
	FString RecentTransitions;
	const int32 FirstIndex = FMath::Max(0, CombatStateTransitionHistory.Num() - 3);
	for (int32 Index = FirstIndex; Index < CombatStateTransitionHistory.Num(); ++Index)
	{
		const FBossCombatStateTransitionRecord& Record = CombatStateTransitionHistory[Index];
		if (!RecentTransitions.IsEmpty())
		{
			RecentTransitions += TEXT(" <- ");
		}

		RecentTransitions += FString::Printf(
			TEXT("%s>%s(%s%s)"),
			GetBossCombatStateDebugText(Record.FromState),
			GetBossCombatStateDebugText(Record.ToState),
			Record.bEnteredState ? TEXT("") : TEXT("Sync:"),
			GetBossCombatStateChangeReasonDebugText(Record.Reason));
	}

	if (RecentTransitions.IsEmpty())
	{
		RecentTransitions = TEXT("None");
	}

	return FString::Printf(
		TEXT("Initiative: %s | Pressure: %.2f | LastBossStateReason: %s | BossStateHistory: %s"),
		GetBossInitiativeDebugText(InitiativeState),
		PlayerPressureRemainTime,
		GetBossCombatStateChangeReasonDebugText(LastCombatSubStateChangeReason),
		*RecentTransitions
	);
}
