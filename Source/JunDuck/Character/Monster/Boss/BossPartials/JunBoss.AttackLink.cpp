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

const TCHAR* LexToString(EBossNormalAttackLinkResult Result)
{
	switch (Result)
	{
	case EBossNormalAttackLinkResult::Validated: return TEXT("Validated");
	case EBossNormalAttackLinkResult::Started: return TEXT("Started");
	case EBossNormalAttackLinkResult::Delayed: return TEXT("Delayed");
	case EBossNormalAttackLinkResult::MovingToRange: return TEXT("MovingToRange");
	case EBossNormalAttackLinkResult::TriggerFailed: return TEXT("TriggerFailed");
	case EBossNormalAttackLinkResult::NoCandidate: return TEXT("NoCandidate");
	case EBossNormalAttackLinkResult::NoPendingRequest: return TEXT("NoPendingRequest");
	case EBossNormalAttackLinkResult::BlockedByExecution: return TEXT("BlockedByExecution");
	case EBossNormalAttackLinkResult::InvalidTarget: return TEXT("InvalidTarget");
	case EBossNormalAttackLinkResult::InvalidState: return TEXT("InvalidState");
	case EBossNormalAttackLinkResult::AttackNotAllowed: return TEXT("AttackNotAllowed");
	case EBossNormalAttackLinkResult::MissingAttackData: return TEXT("MissingAttackData");
	case EBossNormalAttackLinkResult::MissingAnimation: return TEXT("MissingAnimation");
	case EBossNormalAttackLinkResult::OutOfRange: return TEXT("OutOfRange");
	case EBossNormalAttackLinkResult::MontageFailed: return TEXT("MontageFailed");
	default: return TEXT("Unknown");
	}
}

bool AJunBoss::ShouldContinueComboAttack(const FBossComboAttackData& ComboAttackData, int32 CurrentComboLength) const
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

void AJunBoss::HandleComboDecisionPoint()
{
	if (CurrentAttackRuntime.ActionType != EBossActionType::ComboAttack)
	{
		return;
	}

	const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentAttackRuntime.ComboSet);
	UAnimMontage* ComboMontage = ComboAttackData ? ComboAttackData->Montage.Get() : nullptr;
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!ComboAttackData || !ComboMontage || !AnimInstance)
	{
		return;
	}

	const int32 CurrentComboLength = FMath::Max(1, CurrentAttackRuntime.ComboLength);
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

	CurrentAttackRuntime.ComboLength = NextComboLength;
	CurrentCombatPlan.ComboLength = NextComboLength;
	AnimInstance->Montage_JumpToSection(NextSectionName, ComboMontage);
}

EBossNormalAttackLinkResult AJunBoss::ProcessNormalAttackLinkRequest(const FBossNormalAttackLinkRequest& Request)
{
	TArray<FBossNormalAttackLinkCandidate> AttackCandidates;
	const EBossNormalAttackType CurrentAttackType = GetCurrentNormalAttackType();

	auto AddCandidateIfAllowed = [this, &Request, CurrentAttackType, &AttackCandidates](
		const FBossNormalAttackLinkCandidate& Candidate)
	{
		if (Candidate.AttackType == EBossNormalAttackType::None ||
			!IsNormalAttackAllowedByCurrentPhase(Candidate.AttackType))
		{
			return;
		}

		if (Request.bExcludeCurrentAttack && Candidate.AttackType == CurrentAttackType)
		{
			return;
		}

		if (Request.bAvoidRecentlyUsedAttack && WasNormalAttackRecentlyUsed(Candidate.AttackType, 1))
		{
			return;
		}

		AttackCandidates.Add(Candidate);
	};

	auto MakeDefaultCandidate = [&Request](EBossNormalAttackType AttackType)
	{
		FBossNormalAttackLinkCandidate Candidate;
		Candidate.AttackType = AttackType;
		Candidate.BlendOutTime = Request.DefaultBlendOutTime;
		Candidate.BlendInTime = Request.DefaultBlendInTime;
		Candidate.bRequireRange = Request.bDefaultRequireRange;
		Candidate.bMoveToRangeWhenOutOfRange = Request.bDefaultMoveToRangeWhenOutOfRange;
		Candidate.bUseTestFilter = Request.bDefaultUseTestFilter;
		return Candidate;
	};

	AddCandidateIfAllowed(MakeDefaultCandidate(Request.DefaultAttackType));
	for (const EBossNormalAttackType LegacyAttackType : Request.LegacyAdditionalAttackTypes)
	{
		AddCandidateIfAllowed(MakeDefaultCandidate(LegacyAttackType));
	}

	for (const FBossNormalAttackLinkCandidate& Candidate : Request.AdditionalCandidates)
	{
		AddCandidateIfAllowed(Candidate);
	}

	EBossNormalAttackLinkResult Result = EBossNormalAttackLinkResult::NoCandidate;
	bool bTriggerPassed = false;
	EBossNormalAttackType SelectedAttackType = EBossNormalAttackType::None;
	const float ClampedTriggerChance = FMath::Clamp(Request.TriggerChance, 0.f, 1.f);
	if (!AttackCandidates.IsEmpty() && ClampedTriggerChance > 0.f)
	{
		bTriggerPassed = FMath::FRand() <= ClampedTriggerChance;
		if (bTriggerPassed)
		{
			const FBossNormalAttackLinkCandidate& SelectedCandidate =
				AttackCandidates[FMath::RandRange(0, AttackCandidates.Num() - 1)];
			SelectedAttackType = SelectedCandidate.AttackType;
			Result = TryStartNormalAttackLinkFromNotify(
				SelectedAttackType,
				1.f,
				SelectedCandidate.BlendOutTime,
				SelectedCandidate.BlendInTime,
				SelectedCandidate.bRequireRange,
				SelectedCandidate.bMoveToRangeWhenOutOfRange,
				Request.bDelayMoveToRangeWhenOutOfRange,
				SelectedCandidate.bUseTestFilter);
		}
		else if (Request.bForceAttackLinkWhenTriggerChanceFails)
		{
			Result = TryForceNormalAttackLinkWhenTriggerChanceFails(
				Request.DefaultBlendOutTime,
				Request.DefaultBlendInTime);
		}
		else
		{
			Result = EBossNormalAttackLinkResult::TriggerFailed;
		}
	}
	else if (!AttackCandidates.IsEmpty())
	{
		Result = EBossNormalAttackLinkResult::TriggerFailed;
	}

	if (Request.bDebugLog)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossAttackLink] Result=%s TriggerPassed=%s CurrentAttack=%d SelectedAttack=%d Chance=%.2f Candidates=%d DelayMove=%s ForceOnFail=%s ExcludeCurrent=%s AvoidRecent=%s"),
			LexToString(Result),
			bTriggerPassed ? TEXT("true") : TEXT("false"),
			static_cast<int32>(CurrentAttackType),
			static_cast<int32>(SelectedAttackType),
			ClampedTriggerChance,
			AttackCandidates.Num(),
			Request.bDelayMoveToRangeWhenOutOfRange ? TEXT("true") : TEXT("false"),
			Request.bForceAttackLinkWhenTriggerChanceFails ? TEXT("true") : TEXT("false"),
			Request.bExcludeCurrentAttack ? TEXT("true") : TEXT("false"),
			Request.bAvoidRecentlyUsedAttack ? TEXT("true") : TEXT("false"));
	}

	return Result;
}

EBossNormalAttackLinkResult AJunBoss::TryStartNormalAttackLinkFromNotify(
	EBossNormalAttackType NextAttackType,
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
		return EBossNormalAttackLinkResult::BlockedByExecution;
	}

	if (TriggerChance <= 0.f || FMath::FRand() > FMath::Clamp(TriggerChance, 0.f, 1.f))
	{
		return EBossNormalAttackLinkResult::TriggerFailed;
	}

	const FBossNormalAttackData* NextAttackData = nullptr;
	UAnimMontage* NextMontage = nullptr;
	UAnimInstance* CurrentAnimInstance = nullptr;
	bool bLinkingFromParrySuccess = false;
	const EBossNormalAttackLinkResult ValidationResult = ValidateNormalAttackLinkRequest(
		NextAttackType,
		bUseTestFilter,
		NextAttackData,
		NextMontage,
		CurrentAnimInstance,
		bLinkingFromParrySuccess);
	if (ValidationResult != EBossNormalAttackLinkResult::Validated)
	{
		return ValidationResult;
	}

	const float TargetDistance = GetTargetDistance2D();
	if (bRequireRange &&
		(TargetDistance < NextAttackData->MinRange || TargetDistance > NextAttackData->MaxRange))
	{
		return HandleOutOfRangeNormalAttackLink(
			NextAttackType,
			*NextAttackData,
			*CurrentAnimInstance,
			TargetDistance,
			BlendOutTime,
			BlendInTime,
			bMoveToRangeWhenOutOfRange,
			bDelayMoveToRangeWhenOutOfRange,
			bUseTestFilter,
			bLinkingFromParrySuccess);
	}

	ClearDelayedNormalAttackRangeLink();
	CleanupCurrentAttackForNormalAttackLink(
		NextAttackType,
		*CurrentAnimInstance,
		BlendOutTime,
		bLinkingFromParrySuccess,
		false);
	return StartImmediateNormalAttackLink(
		NextAttackType,
		*NextAttackData,
		*NextMontage,
		*CurrentAnimInstance,
		BlendInTime);
}

EBossNormalAttackLinkResult AJunBoss::ValidateNormalAttackLinkRequest(
	EBossNormalAttackType NextAttackType,
	bool bUseTestFilter,
	const FBossNormalAttackData*& OutNextAttackData,
	UAnimMontage*& OutNextMontage,
	UAnimInstance*& OutAnimInstance,
	bool& bOutLinkingFromParrySuccess) const
{
	OutNextAttackData = nullptr;
	OutNextMontage = nullptr;
	OutAnimInstance = nullptr;
	bOutLinkingFromParrySuccess = false;

	if (!CurrentTarget)
	{
		return EBossNormalAttackLinkResult::InvalidTarget;
	}

	if (const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(CurrentTarget))
	{
		if (TargetCharacter->Is_Dead())
		{
			return EBossNormalAttackLinkResult::InvalidTarget;
		}
	}

	if (const AJunPlayer* TargetPlayer = Cast<AJunPlayer>(CurrentTarget))
	{
		if (TargetPlayer->IsInDeathSequence())
		{
			return EBossNormalAttackLinkResult::InvalidTarget;
		}
	}

	const bool bCanLinkFromCurrentState =
		CurrentCombatSubState == EBossCombatState::Attack ||
		CurrentCombatSubState == EBossCombatState::Recovery ||
		CurrentCombatSubState == EBossCombatState::ParrySuccess;
	bOutLinkingFromParrySuccess = CurrentCombatSubState == EBossCombatState::ParrySuccess;
	const bool bNormalParrySuccessLinkAllowed =
		!bOutLinkingFromParrySuccess ||
		(ParryExchangeState.bCounterStarted && ParryExchangeState.bCounterPerfectParried && !ParryExchangeState.bCounterFollowUpStarted);

	if (CurrentState != EMonsterState::Combat ||
		!bCanLinkFromCurrentState ||
		!bNormalParrySuccessLinkAllowed ||
		IsInHitReact())
	{
		return EBossNormalAttackLinkResult::InvalidState;
	}

	if (NextAttackType == EBossNormalAttackType::None ||
		!IsNormalAttackAllowedInCurrentPhase(NextAttackType) ||
		(bUseTestFilter && !IsNormalAttackEnabledByTestFilter(NextAttackType)))
	{
		return EBossNormalAttackLinkResult::AttackNotAllowed;
	}

	OutNextAttackData = GetNormalAttackData(NextAttackType);
	if (!OutNextAttackData)
	{
		return EBossNormalAttackLinkResult::MissingAttackData;
	}

	OutNextMontage = OutNextAttackData->Montage.Get();
	OutAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	return OutNextMontage && OutAnimInstance
		? EBossNormalAttackLinkResult::Validated
		: EBossNormalAttackLinkResult::MissingAnimation;
}

EBossNormalAttackLinkResult AJunBoss::HandleOutOfRangeNormalAttackLink(
	EBossNormalAttackType NextAttackType,
	const FBossNormalAttackData& NextAttackData,
	UAnimInstance& AnimInstance,
	float TargetDistance,
	float BlendOutTime,
	float BlendInTime,
	bool bMoveToRangeWhenOutOfRange,
	bool bDelayMoveToRangeWhenOutOfRange,
	bool bUseTestFilter,
	bool bLinkingFromParrySuccess)
{
	if (!bMoveToRangeWhenOutOfRange)
	{
		return EBossNormalAttackLinkResult::OutOfRange;
	}

	if (bDelayMoveToRangeWhenOutOfRange)
	{
		QueueDelayedNormalAttackRangeLink(
			NextAttackType,
			BlendOutTime,
			BlendInTime,
			bUseTestFilter);
		return EBossNormalAttackLinkResult::Delayed;
	}

	CleanupCurrentAttackForNormalAttackLink(
		NextAttackType,
		AnimInstance,
		BlendOutTime,
		bLinkingFromParrySuccess,
		true);

	bIsAttacking = false;
	bAttackInterruptedByHitReact = false;
	AttackTime = 0.f;
	AttackFacingRemainTime = 0.f;
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	CurrentCombatPlan.ActionType = EBossPlannedActionType::Attack;
	CurrentCombatPlan.AttackType = EBossPlannedAttackType::NormalAttack;
	CurrentCombatPlan.NormalAttackType = NextAttackType;
	CurrentCombatPlan.ComboSet = EBossComboSet::None;
	CurrentCombatPlan.ComboLength = 0;
	CurrentCombatPlan.AttackWaitTime = 0.f;

	TransitionBossCombatState(
		TargetDistance > NextAttackData.MaxRange
			? EBossCombatState::Approach
			: EBossCombatState::Reposition,
		EBossCombatStateChangeReason::TargetRangeChanged);
	return EBossNormalAttackLinkResult::MovingToRange;
}

void AJunBoss::CleanupCurrentAttackForNormalAttackLink(
	EBossNormalAttackType NextAttackType,
	UAnimInstance& AnimInstance,
	float BlendOutTime,
	bool bLinkingFromParrySuccess,
	bool bResetAttackRuntime)
{
	if (!IsBowNormalAttackType(NextAttackType) && IsBowNormalAttackType(CurrentAttackRuntime.NormalAttackType))
	{
		EndBowAttackPresentation();
	}

	if (bLinkingFromParrySuccess)
	{
		AnimInstance.OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
		if (ParryExchangeState.SuccessMontage)
		{
			AnimInstance.Montage_Stop(FMath::Max(0.f, BlendOutTime), ParryExchangeState.SuccessMontage);
		}
		ParryExchangeState.SuccessMontage = nullptr;
		ParryExchangeState.SuccessFallbackRemainTime = 0.f;
	}

	if (CurrentAttackMontage)
	{
		AnimInstance.Montage_Stop(FMath::Max(0.f, BlendOutTime), CurrentAttackMontage);
	}

	if (!bAttackInterruptedByHitReact && CurrentAttackRuntime.ActionType != EBossActionType::None)
	{
		RecordAction(CurrentAttackRuntime.ActionType, CurrentAttackRuntime.ComboSet, CurrentAttackRuntime.ComboLength, CurrentAttackRuntime.NormalAttackType);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	if (bResetAttackRuntime)
	{
		ResetCurrentAttackRuntimeState();
	}

	ParryExchangeState.ResetCounterProgress();
}

EBossNormalAttackLinkResult AJunBoss::StartImmediateNormalAttackLink(
	EBossNormalAttackType NextAttackType,
	const FBossNormalAttackData& NextAttackData,
	UAnimMontage& NextMontage,
	UAnimInstance& AnimInstance,
	float BlendInTime)
{
	TransitionBossCombatState(
		EBossCombatState::Attack,
		EBossCombatStateChangeReason::AttackStarted,
		EBossCombatStateTransitionMode::SynchronizeOnly);
	FBossAttackRuntimeState NextAttackRuntime;
	NextAttackRuntime.ActionType = EBossActionType::NormalAttack;
	NextAttackRuntime.NormalAttackType = NextAttackType;
	LastStartedNormalAttackType = NextAttackType;
	CurrentCombatPlan.AttackType = EBossPlannedAttackType::NormalAttack;
	CurrentCombatPlan.NormalAttackType = NextAttackType;

	if (TryStartVariantNormalAttack(NextAttackType, NextAttackData))
	{
		return EBossNormalAttackLinkResult::Started;
	}

	FMonsterAttackSelection NextAttackSelection;
	NextAttackSelection.Montage = &NextMontage;
	NextAttackSelection.AttackRange = NextAttackData.MaxRange;
	NextAttackSelection.bFaceTargetDuringAttack = NextAttackData.bFaceTargetDuringAttack;
	NextAttackSelection.FacingDuration = NextAttackData.bUseAnimNotifyTiming ? 0.f : NextAttackData.FacingDuration;
	NextAttackSelection.FacingInterpSpeed = NextAttackData.FacingInterpSpeed;
	NextAttackSelection.PlayRate = NextAttackData.PlayRate;
	NextAttackSelection.bTryTurnAfterAttack = NextAttackData.bTryTurnAfterAttack;
	NextAttackSelection.PostAttackTurnStartAngle = NextAttackData.PostAttackTurnStartAngle;

	if (IsBowNormalAttackType(NextAttackType))
	{
		BeginBowAttackPresentation();
		bPostBowCloseRangeApproachInProgress = bForceApproachAfterBowAttack;
		PostBowCloseRangeForceElapsedTime = 0.f;
	}
	else
	{
		bPostBowCloseRangeApproachInProgress = false;
	}

	if (!NextAttackData.bUseAnimNotifyTiming)
	{
		if (NextAttackData.MoveSpeed > 0.f)
		{
			const float MoveStartTime = NextAttackData.MoveStartTimeAtPlayRateOne;
			QueueCodeDrivenAttackMove(
				NextAttackType,
				MoveStartTime / FMath::Max(NextAttackData.PlayRate, KINDA_SMALL_NUMBER));
		}

		ApplyTimedMontageSuperArmor(NextMontage.GetPlayLength() / FMath::Max(NextAttackData.PlayRate, KINDA_SMALL_NUMBER));
	}

	FBossAttackExecutionRequest ExecutionRequest;
	ExecutionRequest.Selection = NextAttackSelection;
	ExecutionRequest.RuntimeState = NextAttackRuntime;
	ExecutionRequest.InitiativeState = EBossInitiativeState::BossPattern;
	ExecutionRequest.BlendInTime = BlendInTime;
	if (!ProcessBossAttackExecution(ExecutionRequest))
	{
		FinishAttack();
		return EBossNormalAttackLinkResult::MontageFailed;
	}

	return EBossNormalAttackLinkResult::Started;
}

void AJunBoss::QueueDelayedNormalAttackRangeLink(
	EBossNormalAttackType AttackType,
	float BlendOutTime,
	float BlendInTime,
	bool bUseTestFilter)
{
	DelayedNormalAttackRangeLinkRequest.bPending = AttackType != EBossNormalAttackType::None;
	DelayedNormalAttackRangeLinkRequest.AttackType = AttackType;
	DelayedNormalAttackRangeLinkRequest.BlendOutTime = FMath::Max(0.f, BlendOutTime);
	DelayedNormalAttackRangeLinkRequest.BlendInTime = FMath::Max(0.f, BlendInTime);
	DelayedNormalAttackRangeLinkRequest.bUseTestFilter = bUseTestFilter;
}

bool AJunBoss::ConsumeDelayedNormalAttackRangeLink(FBossDelayedNormalAttackLinkRequest& OutRequest)
{
	if (!DelayedNormalAttackRangeLinkRequest.IsValid())
	{
		return false;
	}

	OutRequest = DelayedNormalAttackRangeLinkRequest;
	ClearDelayedNormalAttackRangeLink();
	return true;
}

void AJunBoss::ClearDelayedNormalAttackRangeLink()
{
	DelayedNormalAttackRangeLinkRequest.Reset();
}

EBossNormalAttackLinkResult AJunBoss::TryExecuteDelayedNormalAttackRangeLinkFromNotify(float BlendOutTimeOverride)
{
	if (bExecutionReady || bBeingExecuted)
	{
		ClearDelayedNormalAttackRangeLink();
		return EBossNormalAttackLinkResult::BlockedByExecution;
	}

	FBossDelayedNormalAttackLinkRequest DelayedRequest;
	if (!ConsumeDelayedNormalAttackRangeLink(DelayedRequest))
	{
		return EBossNormalAttackLinkResult::NoPendingRequest;
	}

	const float BlendOutTime = BlendOutTimeOverride >= 0.f
		? BlendOutTimeOverride
		: DelayedRequest.BlendOutTime;

	return TryStartNormalAttackLinkFromNotify(
		DelayedRequest.AttackType,
		1.f,
		BlendOutTime,
		DelayedRequest.BlendInTime,
		true,
		true,
		false,
		DelayedRequest.bUseTestFilter);
}

EBossNormalAttackLinkResult AJunBoss::TryForceNormalAttackLinkWhenTriggerChanceFails(float BlendOutTime, float BlendInTime)
{
	if (bExecutionReady || bBeingExecuted)
	{
		return EBossNormalAttackLinkResult::BlockedByExecution;
	}

	TArray<FBossWeightedNormalAttackCandidate> NormalAttackCandidates;
	FBossNormalAttackCandidatePolicy LinkFallbackPolicy;
	LinkFallbackPolicy.bIgnoreRange = true;
	CollectNormalAttackCandidates(0.f, LinkFallbackPolicy, NormalAttackCandidates);
	if (NormalAttackCandidates.Num() <= 0)
	{
		LinkFallbackPolicy.bIgnoreRepeat = true;
		CollectNormalAttackCandidates(0.f, LinkFallbackPolicy, NormalAttackCandidates);
	}

	if (NormalAttackCandidates.Num() <= 0)
	{
		return EBossNormalAttackLinkResult::NoCandidate;
	}

	const EBossNormalAttackType NextNormalAttackType = SelectNormalAttackCandidate(NormalAttackCandidates);
	if (NextNormalAttackType == EBossNormalAttackType::None)
	{
		return EBossNormalAttackLinkResult::NoCandidate;
	}

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

bool AJunBoss::TryStartParryBackJump()
{
	UAnimMontage* EscapeMontage = GetParryExchangeEscapeMontage();
	if (bExecutionReady ||
		bBeingExecuted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::ParrySuccess ||
		ParryExchangeState.bCounterStarted ||
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

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);

	if (ParryExchangeState.SuccessMontage)
	{
		AnimInstance->Montage_Stop(FMath::Max(0.f, ParryBackJumpBlendOutTime), ParryExchangeState.SuccessMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ResetCurrentAttackRuntimeState();

	ParryExchangeState.bCounterStarted = true;
	ParryExchangeState.bCounterFollowUpStarted = true;
	ParryExchangeState.bCounterPerfectParried = false;
	ParryExchangeState.SuccessFallbackRemainTime = 0.f;
	ParryExchangeState.SuccessMontage = EscapeMontage;
	TransitionBossCombatState(
		EBossCombatState::ParrySuccess,
		EBossCombatStateChangeReason::MobilityStarted,
		EBossCombatStateTransitionMode::SynchronizeOnly);
	InitiativeState = EBossInitiativeState::BossParryCounter;

	CombatMoveInput = FVector2D::ZeroVector;
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);

	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, ParryBackJumpBlendInTime));
	const float PlayedDuration = AnimInstance->Montage_PlayWithBlendSettings(
		ParryExchangeState.SuccessMontage,
		BlendInSettings,
		FMath::Max(ParryBackJumpPlayRate, KINDA_SMALL_NUMBER));
	if (PlayedDuration <= 0.f)
	{
		FinishParrySuccessState();
		return false;
	}

	ParryExchangeState.SuccessFallbackRemainTime = PlayedDuration;
	return true;
}

