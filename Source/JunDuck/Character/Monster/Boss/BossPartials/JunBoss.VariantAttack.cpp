#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "System/JunCombatVFXSubsystem.h"
#include "Weapon/ArrowProjectile.h"

bool AJunBoss::TryStartVariantNormalAttack(EBossNormalAttackType AttackType, const FBossNormalAttackData& AttackData)
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

	VariantAttackState.Activate(AttackType, GetVariantNormalAttackRequiredDashCount(AttackType));
	bPostBowCloseRangeApproachInProgress = false;
	CurrentAttackRuntime.Reset();
	CurrentAttackRuntime.ActionType = EBossActionType::NormalAttack;
	CurrentAttackRuntime.NormalAttackType = AttackType;

	return PlayVariantNormalAttackDashStep();
}

bool AJunBoss::AdvanceVariantNormalAttackFromNotify()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::Attack ||
		!VariantAttackState.bActive ||
		VariantAttackState.TargetAttackType == EBossNormalAttackType::None ||
		IsInHitReact())
	{
		return false;
	}

	if (VariantAttackState.HasRemainingDash())
	{
		return PlayVariantNormalAttackDashStep();
	}

	return PlayVariantNormalAttackFinalMontage();
}

bool AJunBoss::PlayVariantNormalAttackDashStep()
{
	if (bExecutionReady || bBeingExecuted)
	{
		ClearVariantNormalAttackState();
		return false;
	}

	UAnimMontage* DashMontage = GetVariantNormalAttackDashMontage(VariantAttackState.TargetAttackType);
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

	FMonsterAttackSelection DashSelection;
	DashSelection.Montage = DashMontage;
	DashSelection.AttackRange = 0.f;
	DashSelection.bFaceTargetDuringAttack = bVariantDashFaceTarget;
	DashSelection.FacingDuration = VariantDashFacingDuration;
	DashSelection.FacingInterpSpeed = VariantDashFacingInterpSpeed;
	DashSelection.PlayRate = VariantDashPlayRate;
	DashSelection.bTryTurnAfterAttack = false;
	DashSelection.PostAttackTurnStartAngle = TurnStartAngle;

	FBossAttackExecutionRequest ExecutionRequest;
	ExecutionRequest.Selection = DashSelection;
	ExecutionRequest.RuntimeState = CurrentAttackRuntime;
	ExecutionRequest.InitiativeState = EBossInitiativeState::BossPattern;
	ExecutionRequest.BlendInTime = VariantDashBlendInTime;
	if (!ProcessBossAttackExecution(ExecutionRequest))
	{
		ClearVariantNormalAttackState();
		FinishAttack();
		return false;
	}

	VariantAttackState.AdvanceDash();
	return true;
}

bool AJunBoss::PlayVariantNormalAttackFinalMontage()
{
	if (bExecutionReady || bBeingExecuted)
	{
		ClearVariantNormalAttackState();
		return false;
	}

	const EBossNormalAttackType AttackType = VariantAttackState.TargetAttackType;
	const FBossNormalAttackData* AttackData = GetNormalAttackData(AttackType);
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

	TransitionBossCombatState(
		EBossCombatState::Attack,
		EBossCombatStateChangeReason::AttackStarted,
		EBossCombatStateTransitionMode::SynchronizeOnly);
	FBossAttackRuntimeState NextAttackRuntime;
	NextAttackRuntime.ActionType = EBossActionType::NormalAttack;
	NextAttackRuntime.NormalAttackType = AttackType;
	LastStartedNormalAttackType = AttackType;
	CurrentCombatPlan.AttackType = EBossPlannedAttackType::NormalAttack;
	CurrentCombatPlan.NormalAttackType = AttackType;

	FMonsterAttackSelection FinalAttackSelection;
	FinalAttackSelection.Montage = TargetAttackMontage;
	FinalAttackSelection.AttackRange = AttackData->MaxRange;
	FinalAttackSelection.bFaceTargetDuringAttack = AttackData->bFaceTargetDuringAttack;
	FinalAttackSelection.FacingDuration = AttackData->bUseAnimNotifyTiming ? 0.f : AttackData->FacingDuration;
	FinalAttackSelection.FacingInterpSpeed = AttackData->FacingInterpSpeed;
	FinalAttackSelection.PlayRate = AttackData->PlayRate;
	FinalAttackSelection.bTryTurnAfterAttack = AttackData->bTryTurnAfterAttack;
	FinalAttackSelection.PostAttackTurnStartAngle = AttackData->PostAttackTurnStartAngle;
	bPostBowCloseRangeApproachInProgress = false;

	if (!AttackData->bUseAnimNotifyTiming)
	{
		if (AttackData->MoveSpeed > 0.f)
		{
			const float MoveStartTime = AttackData->MoveStartTimeAtPlayRateOne;
			QueueCodeDrivenAttackMove(AttackType, MoveStartTime / FMath::Max(AttackData->PlayRate, KINDA_SMALL_NUMBER));
		}

		ApplyTimedMontageSuperArmor(TargetAttackMontage->GetPlayLength() / FMath::Max(AttackData->PlayRate, KINDA_SMALL_NUMBER));
	}

	FBossAttackExecutionRequest ExecutionRequest;
	ExecutionRequest.Selection = FinalAttackSelection;
	ExecutionRequest.RuntimeState = NextAttackRuntime;
	ExecutionRequest.InitiativeState = EBossInitiativeState::BossPattern;
	ExecutionRequest.BlendInTime = VariantDashBlendInTime;
	if (!ProcessBossAttackExecution(ExecutionRequest))
	{
		FinishAttack();
		return false;
	}

	return true;
}

bool AJunBoss::IsVariantNormalAttackType(EBossNormalAttackType AttackType) const
{
	return AttackType == EBossNormalAttackType::ChargeAttack ||
		AttackType == EBossNormalAttackType::NinjaB;
}

float AJunBoss::GetVariantNormalAttackChance(EBossNormalAttackType AttackType) const
{
	switch (AttackType)
	{
	case EBossNormalAttackType::ChargeAttack:
		return ChargeVariantChance;
	case EBossNormalAttackType::NinjaB:
		return NinjaBVariantChance;
	default:
		return 0.f;
	}
}

bool AJunBoss::IsWithinVariantNormalAttackDistance(EBossNormalAttackType AttackType, const FBossNormalAttackData& AttackData) const
{
	float MinDistance = AttackData.MinRange;
	float MaxDistance = AttackData.CandidateMaxRange > 0.f ? AttackData.CandidateMaxRange : AttackData.MaxRange;

	switch (AttackType)
	{
	case EBossNormalAttackType::ChargeAttack:
		MinDistance = ChargeVariantMinDistance > 0.f ? ChargeVariantMinDistance : MinDistance;
		MaxDistance = ChargeVariantMaxDistance > 0.f ? ChargeVariantMaxDistance : MaxDistance;
		break;
	case EBossNormalAttackType::NinjaB:
		MinDistance = NinjaBVariantMinDistance > 0.f ? NinjaBVariantMinDistance : MinDistance;
		MaxDistance = NinjaBVariantMaxDistance > 0.f ? NinjaBVariantMaxDistance : MaxDistance;
		break;
	default:
		break;
	}

	const float TargetDistance = GetTargetDistance2D();
	return TargetDistance >= MinDistance && (MaxDistance <= 0.f || TargetDistance <= MaxDistance);
}

UAnimMontage* AJunBoss::GetVariantNormalAttackDashMontage(EBossNormalAttackType AttackType) const
{
	switch (AttackType)
	{
	case EBossNormalAttackType::ChargeAttack:
		if (VariantAttackState.bActive && VariantAttackState.CompletedDashCount > 0 && ChargeVariantFollowUpDashMontage)
		{
			return ChargeVariantFollowUpDashMontage.Get();
		}
		return ChargeVariantDashMontage.Get();
	case EBossNormalAttackType::NinjaB:
		if (VariantAttackState.bActive && VariantAttackState.CompletedDashCount > 0 && NinjaBVariantFollowUpDashMontage)
		{
			return NinjaBVariantFollowUpDashMontage.Get();
		}
		return NinjaBVariantDashMontage.Get();
	default:
		return nullptr;
	}
}

UAnimMontage* AJunBoss::GetVariantNormalAttackFinalMontage(EBossNormalAttackType AttackType, const FBossNormalAttackData& AttackData) const
{
	switch (AttackType)
	{
	case EBossNormalAttackType::ChargeAttack:
		return ChargeVariantAttackMontage ? ChargeVariantAttackMontage.Get() : AttackData.Montage.Get();
	case EBossNormalAttackType::NinjaB:
		return NinjaBVariantAttackMontage ? NinjaBVariantAttackMontage.Get() : AttackData.Montage.Get();
	default:
		return AttackData.Montage.Get();
	}
}

int32 AJunBoss::GetVariantNormalAttackRequiredDashCount(EBossNormalAttackType AttackType) const
{
	return AttackType == EBossNormalAttackType::NinjaB ? 2 : 1;
}

void AJunBoss::ClearVariantNormalAttackState()
{
	VariantAttackState.Reset();
}

bool AJunBoss::IsBowNormalAttackType(EBossNormalAttackType AttackType) const
{
	return AttackType == EBossNormalAttackType::Bow4Combo ||
		AttackType == EBossNormalAttackType::BowBackDashAttack ||
		AttackType == EBossNormalAttackType::BowHeavyAttack;
}

void AJunBoss::BeginBowAttackPresentation()
{
	bBowAttackPresentationActive = true;
	SetWeaponVisible(false);
	SetBowVisible(true);
}

void AJunBoss::EndBowAttackPresentation()
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

