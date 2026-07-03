#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunCombatVFXSubsystem.h"
#include "Weapon/ArrowProjectile.h"

EJunDefenseOutcome AJunBoss::TryHandleIncomingHitBeforeDamage(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	const FBossParryDecision Decision = EvaluateParryDecision(
		HitType,
		DamageCauser,
		SwingDirection,
		DefenseRuleData);

	if (Decision.Result != EBossParryDecisionResult::NotParryable)
	{
		ParryExchangeState.ClearPendingChanceGain();
	}

	const bool bParryApplied = ApplyParryDecision(
		Decision,
		DamageCauser,
		SwingDirection,
		DefenseKnockbackData);
	if (!bParryApplied)
	{
		return EJunDefenseOutcome::None;
	}

	return Decision.Result == EBossParryDecisionResult::PerfectParry
		? EJunDefenseOutcome::PerfectParried
		: EJunDefenseOutcome::NormalParried;
}

void AJunBoss::NotifyAttackParriedBy(
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
		CurrentAttackRuntime.ActionType == EBossActionType::ParryCounter &&
		InitiativeState == EBossInitiativeState::BossParryCounter;

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
	CurrentAttackRuntime.ActionType = EBossActionType::None;
	CurrentAttackRuntime.ComboSet = EBossComboSet::None;
	CurrentAttackRuntime.ComboLength = 0;
	CurrentAttackRuntime.NormalAttackType = EBossNormalAttackType::None;
	AttackFacingRemainTime = 0.f;

	StartPlayerPressure();
	ParryExchangeState.bCounterPerfectParried = true;
}

bool AJunBoss::TryStartPerfectParryRebound(AJunPlayer* Parrier, const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!DefenseRuleData.bCanReboundOnPerfectParry ||
		!Parrier ||
		bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::Attack ||
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

	FMonsterAttackSelection ReboundSelection;
	ReboundSelection.Montage = ReboundMontage;
	ReboundSelection.AttackRange = 0.f;
	ReboundSelection.bFaceTargetDuringAttack = true;
	ReboundSelection.FacingDuration = 0.f;
	ReboundSelection.FacingInterpSpeed = AttackFacingInterpSpeed;
	ReboundSelection.PlayRate = 1.f;
	ReboundSelection.bTryTurnAfterAttack = false;

	FBossSpecialReactionExecutionRequest ReactionRequest;
	ReactionRequest.Type = EBossSpecialReactionType::PerfectParryRebound;
	ReactionRequest.Selection = ReboundSelection;
	ReactionRequest.InitiativeState = EBossInitiativeState::PlayerPressure;
	ReactionRequest.BlendInTime = DefenseRuleData.ReboundBlendTime;
	if (!ProcessBossSpecialReactionExecution(ReactionRequest))
	{
		return false;
	}

	StartPlayerPressure();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->bOrientRotationToMovement = false;
	}

	return true;
}

UAnimMontage* AJunBoss::GetPerfectParryReboundMontage(const AJunPlayer* Parrier) const
{
	if (Parrier && Parrier->DidLastParrySuccessUseLeftSide())
	{
		return PerfectParryReboundLMontage ? PerfectParryReboundLMontage.Get() : PerfectParryReboundRMontage.Get();
	}

	return PerfectParryReboundRMontage ? PerfectParryReboundRMontage.Get() : PerfectParryReboundLMontage.Get();
}

bool AJunBoss::NotifyMikiriCounteredBy(AJunPlayer* CounterPlayer)
{
	if (!CounterPlayer ||
		bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::Attack ||
		CurrentAttackRuntime.ActionType != EBossActionType::NormalAttack ||
		CurrentAttackRuntime.NormalAttackType != EBossNormalAttackType::NinjaB ||
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
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ClearDelayedNormalAttackRangeLink();
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	AttackFacingRemainTime = 0.f;

	CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, MikiriCounterBlendOutTime), InterruptedMontage);

	FMonsterAttackSelection CounteredSelection = CurrentAttackSelection;
	CounteredSelection.Montage = CounterMontage;
	CounteredSelection.PlayRate = MikiriCounterPlayRate;
	CounteredSelection.bFaceTargetDuringAttack = false;
	CounteredSelection.FacingDuration = 0.f;

	FBossSpecialReactionExecutionRequest ReactionRequest;
	ReactionRequest.Type = EBossSpecialReactionType::MikiriCountered;
	ReactionRequest.Selection = CounteredSelection;
	ReactionRequest.InitiativeState = InitiativeState;
	ReactionRequest.BlendInTime = MikiriCounterBlendInTime;
	if (!ProcessBossSpecialReactionExecution(ReactionRequest))
	{
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

float AJunBoss::GetHitReactDuration(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
		return BossLightHitDuration;
	case EHitReactType::LargeHit_Short:
		return BossLargeHitDuration;
	case EHitReactType::LargeHit_Long:
		return BossLargeHitLongDuration;
	default:
		return Super::GetHitReactDuration(HitType);
	}
}

float AJunBoss::GetHitReactControlLockDuration(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::HeavyHit_A:
	case EHitReactType::HeavyHit_B:
	case EHitReactType::HeavyHit_C:
		return BossHeavyHitControlLockDuration;
	default:
		return Super::GetHitReactControlLockDuration(HitType);
	}
}

void AJunBoss::OnHitReactStarted(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection)
{
	Super::OnHitReactStarted(NewHitReact, NewHitDirection);
	if (ParryExchangeState.bPendingChanceGain && ParryExchangeState.PendingHitType == NewHitReact)
	{
		if (GetHitReactMontage(NewHitReact, NewHitDirection))
		{
			AccumulateParryExchangeNormalChance(NewHitReact);
		}

		ParryExchangeState.ClearPendingChanceGain();
	}

	EndBowAttackPresentation();
	LastHitReactDirectionForTurn = NewHitDirection;

	if (CurrentState == EMonsterState::Combat)
	{
		TransitionBossCombatState(EBossCombatState::Hit, EBossCombatStateChangeReason::HitReactStarted);
	}
}

void AJunBoss::OnIncomingHitResolvedWithoutHitReact(EHitReactType HitType)
{
	Super::OnIncomingHitResolvedWithoutHitReact(HitType);
	if (ParryExchangeState.bPendingChanceGain && ParryExchangeState.PendingHitType == HitType)
	{
		ParryExchangeState.ClearPendingChanceGain();
	}
}

bool AJunBoss::CanBeInterruptedBy(EHitReactType IncomingHitReact) const
{
	// Once a hit reaction has started, stale or overlapping armor windows must not
	// prevent another valid hit from blending into its own reaction.
	if (IsInHitReact())
	{
		return IncomingHitReact != EHitReactType::None;
	}

	return Super::CanBeInterruptedBy(IncomingHitReact);
}

bool AJunBoss::ShouldStartHitReact(EHitReactType IncomingHitReact) const
{
	if (bAirborneHitReactLockWindowActive)
	{
		return false;
	}

	if (CurrentCombatSubState == EBossCombatState::ParrySuccess &&
		InitiativeState == EBossInitiativeState::BossParryCounter)
	{
		return false;
	}

	return Super::ShouldStartHitReact(IncomingHitReact);
}

ECharacterHitReactDirection AJunBoss::AdjustHitReactDirection(
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

void AJunBoss::OnHitReactEnded(EHitReactType EndedHitReact)
{
	Super::OnHitReactEnded(EndedHitReact);

	if (bHitReactTurnTransitionInProgress)
	{
		return;
	}

	if (CurrentState == EMonsterState::Combat &&
		CurrentCombatSubState == EBossCombatState::Hit)
	{
		ResetPlannedCombatPlan();

		if (ShouldTurnAfterHitReact(LastHitReactDirectionForTurn))
		{
			StopAIMovement();
			CombatMoveInput = FVector2D::ZeroVector;
			SetDesiredMoveAxes(0.f, 0.f);
			bRunLocomotionRequested = false;
			TransitionBossCombatState(EBossCombatState::Turn, EBossCombatStateChangeReason::TurnStarted);
			return;
		}

		if (TryReactToHitDirection(LastHitReactDirectionForTurn))
		{
			TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::HitReactEnded);
			return;
		}

		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::HitReactEnded);
	}
}

void AJunBoss::OnExecutionReadyStarted()
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
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
		if (ParryExchangeState.SuccessMontage)
		{
			AnimInstance->Montage_Stop(0.05f, ParryExchangeState.SuccessMontage);
		}
	}
	ParryExchangeState.SuccessMontage = nullptr;
	ParryExchangeState.SuccessFallbackRemainTime = 0.f;
	ParryExchangeState.ResetCounterProgress();
	InitiativeState = EBossInitiativeState::Neutral;
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;

	if (CurrentState == EMonsterState::Combat)
	{
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::ExecutionReadyStarted);
	}
}

void AJunBoss::OnExecutionReadyEnded(bool bMissedExecution)
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
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::ExecutionReadyEnded);
	}
}

void AJunBoss::FinishExecutionRecovery()
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

	TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::ExecutionReadyEnded);
}

