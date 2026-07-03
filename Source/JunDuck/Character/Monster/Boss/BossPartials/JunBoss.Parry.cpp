#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "System/JunCombatVFXSubsystem.h"

FBossParryDecision AJunBoss::EvaluateParryDecision(
	EHitReactType HitType,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	FBossParryDecision Decision;
	Decision.HitType = HitType;

	if (!DefenseRuleData.bCanBeParried)
	{
		Decision.Result = EBossParryDecisionResult::NotParryable;
		return Decision;
	}

	if (!IsParryableHitType(HitType))
	{
		Decision.Result = EBossParryDecisionResult::InvalidHitType;
		return Decision;
	}

	if (!DamageCauser)
	{
		Decision.Result = EBossParryDecisionResult::InvalidTarget;
		return Decision;
	}

	const bool bCanChainParryFromParrySuccess =
		CurrentCombatSubState == EBossCombatState::ParrySuccess;

	if (CurrentState != EMonsterState::Combat ||
		CurrentAttackRuntime.ActionType == EBossActionType::ParryCounter ||
		bBeingExecuted ||
		bExecutionReady)
	{
		Decision.Result = EBossParryDecisionResult::InvalidState;
		return Decision;
	}

	if (!bCanChainParryFromParrySuccess &&
		(!CanBeInterruptedBy(HitType) || !ShouldStartHitReact(HitType)))
	{
		Decision.Result = EBossParryDecisionResult::InvalidState;
		return Decision;
	}

	if (!IsDamageCauserInParryFrontCone(DamageCauser))
	{
		Decision.Result = EBossParryDecisionResult::OutsideFrontCone;
		return Decision;
	}

	const ECharacterHitReactDirection HitDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	if (HitDirection != ECharacterHitReactDirection::Front_F &&
		HitDirection != ECharacterHitReactDirection::Front_FL &&
		HitDirection != ECharacterHitReactDirection::Front_FR)
	{
		Decision.Result = EBossParryDecisionResult::InvalidHitDirection;
		return Decision;
	}

	Decision.PerfectParryChance = GetPerfectParryChanceForHitType(HitType);
	Decision.NormalParryChance = GetNormalParryChanceForHitType(HitType);
	if (FMath::FRand() <= Decision.PerfectParryChance)
	{
		Decision.Result = EBossParryDecisionResult::PerfectParry;
	}
	else if (FMath::FRand() <= Decision.NormalParryChance)
	{
		Decision.Result = EBossParryDecisionResult::NormalParry;
	}
	else
	{
		Decision.Result = EBossParryDecisionResult::ChanceFailed;
	}

	return Decision;
}

bool AJunBoss::ApplyParryDecision(
	const FBossParryDecision& Decision,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	switch (Decision.Result)
	{
	case EBossParryDecisionResult::PerfectParry:
		ResetParryExchangePerfectChance(Decision.HitType);
		PlayDefenseSound(EJunDefenseSoundType::PerfectParry);
		StartParrySuccessAgainstIncomingHit(
			DamageCauser,
			SwingDirection,
			DefenseKnockbackData,
			EJunCombatDefenseVFX::PerfectParry);
		return true;

	case EBossParryDecisionResult::NormalParry:
		AccumulateParryExchangePerfectChance(Decision.HitType);
		PlayDefenseSound(EJunDefenseSoundType::NormalParry);
		AddPosture(NormalParryPostureGain);
		if (bExecutionReady || bBeingExecuted)
		{
			return true;
		}

		{
			const EBossInitiativeState SavedInitiativeState = InitiativeState;
			StartParrySuccessAgainstIncomingHit(
				DamageCauser,
				SwingDirection,
				DefenseKnockbackData,
				EJunCombatDefenseVFX::NormalParry);
			InitiativeState = SavedInitiativeState;
		}
		ParryExchangeState.bCounterStarted = true;
		ParryExchangeState.bCounterPerfectParried = true;
		return true;

	case EBossParryDecisionResult::ChanceFailed:
		ParryExchangeState.bPendingChanceGain = true;
		ParryExchangeState.PendingHitType = Decision.HitType;
		return false;

	default:
		return false;
	}
}

bool AJunBoss::IsDamageCauserInParryFrontCone(const AActor* DamageCauser) const
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

bool AJunBoss::IsParryableHitType(EHitReactType HitType) const
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

float AJunBoss::GetPerfectParryChanceForHitType(EHitReactType HitType) const
{
	return FMath::Clamp(
		HitType == EHitReactType::LightHit ? ParryExchangeState.PerfectParryChance : ParryExchangeState.StrongPerfectParryChance,
		0.f,
		1.f);
}

float AJunBoss::GetNormalParryChanceForHitType(EHitReactType HitType) const
{
	return FMath::Clamp(
		HitType == EHitReactType::LightHit ? ParryExchangeState.NormalParryChance : ParryExchangeState.StrongNormalParryChance,
		0.f,
		1.f);
}

void AJunBoss::ResetParryExchangeCycle()
{
	ParryExchangeState.ResetExchangeProgress();
}

void AJunBoss::ResetParryExchangePerfectChance(EHitReactType HitType)
{
	if (HitType == EHitReactType::LightHit)
	{
		ParryExchangeState.PerfectParryChance = 0.f;
	}
	else
	{
		ParryExchangeState.StrongPerfectParryChance = 0.f;
	}

	ParryExchangeState.IdleTime = 0.f;
}

void AJunBoss::AccumulateParryExchangeNormalChance(EHitReactType HitType)
{
	if (HitType == EHitReactType::LightHit)
	{
		ParryExchangeState.NormalParryChance = FMath::Clamp(
			ParryExchangeState.NormalParryChance + FMath::Clamp(NormalParryChanceGainPerIncomingHit, 0.f, 1.f),
			0.f,
			1.f);
	}
	else
	{
		ParryExchangeState.StrongNormalParryChance = FMath::Clamp(
			ParryExchangeState.StrongNormalParryChance + FMath::Clamp(StrongNormalParryChanceGainPerIncomingHit, 0.f, 1.f),
			0.f,
			1.f);
	}

	ParryExchangeState.IdleTime = 0.f;
}

void AJunBoss::AccumulateParryExchangePerfectChance(EHitReactType HitType)
{
	if (HitType == EHitReactType::LightHit)
	{
		ParryExchangeState.PerfectParryChance = FMath::Clamp(
			ParryExchangeState.PerfectParryChance + FMath::Clamp(PerfectParryChanceGainPerNormalParry, 0.f, 1.f),
			0.f,
			1.f);
	}
	else
	{
		ParryExchangeState.StrongPerfectParryChance = FMath::Clamp(
			ParryExchangeState.StrongPerfectParryChance + FMath::Clamp(StrongPerfectParryChanceGainPerNormalParry, 0.f, 1.f),
			0.f,
			1.f);
	}

	++ParryExchangeState.NormalParryCount;
	ParryExchangeState.IdleTime = 0.f;
}

void AJunBoss::UpdateParryExchangeDecay(float DeltaTime)
{
	if (ParryExchangeState.NormalParryChance <= 0.f &&
		ParryExchangeState.PerfectParryChance <= 0.f &&
		ParryExchangeState.StrongNormalParryChance <= 0.f &&
		ParryExchangeState.StrongPerfectParryChance <= 0.f)
	{
		ParryExchangeState.NormalParryChance = 0.f;
		ParryExchangeState.PerfectParryChance = 0.f;
		ParryExchangeState.StrongNormalParryChance = 0.f;
		ParryExchangeState.StrongPerfectParryChance = 0.f;
		ParryExchangeState.NormalParryCount = 0;
		ParryExchangeState.IdleTime = 0.f;
		return;
	}

	if (CurrentCombatSubState == EBossCombatState::ParrySuccess ||
		(InitiativeState == EBossInitiativeState::BossParryCounter &&
			CurrentCombatSubState == EBossCombatState::Attack))
	{
		ParryExchangeState.IdleTime = 0.f;
		return;
	}

	ParryExchangeState.IdleTime += DeltaTime;
	if (ParryExchangeState.IdleTime < ParryExchangeDecayDelay)
	{
		return;
	}

	const float DecayAmount = FMath::Max(0.f, ParryExchangeChanceDecayPerSecond) * DeltaTime;
	ParryExchangeState.NormalParryChance = FMath::Max(0.f, ParryExchangeState.NormalParryChance - DecayAmount);
	ParryExchangeState.PerfectParryChance = FMath::Max(0.f, ParryExchangeState.PerfectParryChance - DecayAmount);
	ParryExchangeState.StrongNormalParryChance = FMath::Max(0.f, ParryExchangeState.StrongNormalParryChance - DecayAmount);
	ParryExchangeState.StrongPerfectParryChance = FMath::Max(0.f, ParryExchangeState.StrongPerfectParryChance - DecayAmount);

	if (ParryExchangeState.NormalParryChance <= 0.f &&
		ParryExchangeState.PerfectParryChance <= 0.f &&
		ParryExchangeState.StrongNormalParryChance <= 0.f &&
		ParryExchangeState.StrongPerfectParryChance <= 0.f)
	{
		ParryExchangeState.NormalParryCount = 0;
	}
}

bool AJunBoss::ShouldEscapeParryExchangeBeforeCounter() const
{
	return GetParryExchangeEscapeMontage() &&
		ParryExchangeState.NormalParryCount >= FMath::Max(0, ParryExchangeEscapeMinNormalParryCount) &&
		FMath::FRand() <= FMath::Clamp(ParryExchangeEscapeChance, 0.f, 1.f);
}

UAnimMontage* AJunBoss::GetParryExchangeEscapeMontage() const
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

UAnimMontage* AJunBoss::GetParrySuccessMontage(const FVector& SwingDirection)
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

UAnimMontage* AJunBoss::GetParryCounterMontage() const
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

UAnimMontage* AJunBoss::GetParryCounterFollowUpMontage() const
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

USceneComponent* AJunBoss::FindSceneComponentByName(FName ComponentName) const
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

void AJunBoss::PlayParryParticle(EJunCombatDefenseVFX EffectType)
{
	if (!CachedParryEffectLocationComponent)
	{
		CachedParryEffectLocationComponent = FindSceneComponentByName(ParryEffectLocationComponentName);
	}

	if (UJunCombatVFXSubsystem* VFXSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UJunCombatVFXSubsystem>() : nullptr)
	{
		const USceneComponent* SpawnLocationComponent = CachedParryEffectLocationComponent ? CachedParryEffectLocationComponent.Get() : GetMesh();
		VFXSubsystem->SpawnDefenseEffect(EffectType, SpawnLocationComponent);
	}
}

void AJunBoss::StartParrySuccessAgainstIncomingHit(
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	EJunCombatDefenseVFX EffectType)
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	EndBowAttackPresentation();
	PlayParryParticle(EffectType);

	ParryExchangeState.ResetCounterProgress();
	InitiativeState = EBossInitiativeState::BossParryCounter;
	ParryExchangeState.SuccessMontage = GetParrySuccessMontage(SwingDirection);

	if (CurrentCombatSubState == EBossCombatState::ParrySuccess)
	{
		CombatSubStateElapsedTime = 0.f;
		EnterParrySuccessState();
	}
	else
	{
		TransitionBossCombatState(EBossCombatState::ParrySuccess, EBossCombatStateChangeReason::ParrySucceeded);
	}

	ApplyHitReactKnockback(DamageCauser, DefenseKnockbackData.ParrySuccess, ParryExchangeState.SuccessMontage);

	if (ParryExchangeState.SuccessMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
			AnimInstance->OnMontageEnded.AddDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
		}

		const float PlayedDuration = PlayAnimMontage(ParryExchangeState.SuccessMontage);
		ParryExchangeState.SuccessFallbackRemainTime = PlayedDuration > 0.f ? 0.f : ParrySuccessFallbackDuration;
	}
	else
	{
		ParryExchangeState.SuccessFallbackRemainTime = ParrySuccessFallbackDuration;
	}
}

bool AJunBoss::TryStartParryCounterDecision()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		ParryExchangeState.bCounterStarted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::ParrySuccess)
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

bool AJunBoss::TryStartParryCounterAttack()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		ParryExchangeState.bCounterStarted ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::ParrySuccess)
	{
		return false;
	}

	UAnimMontage* CounterMontage = GetParryCounterMontage();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!CounterMontage || !AnimInstance)
	{
		return false;
	}

	ParryExchangeState.bCounterStarted = true;
	ParryExchangeState.bCounterPerfectParried = false;
	ParryExchangeState.bCounterFollowUpStarted = false;
	ParryExchangeState.SuccessFallbackRemainTime = 0.f;
	TransitionBossCombatState(
		EBossCombatState::Attack,
		EBossCombatStateChangeReason::AttackStarted,
		EBossCombatStateTransitionMode::SynchronizeOnly);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
	if (ParryExchangeState.SuccessMontage)
	{
		AnimInstance->Montage_Stop(ParryCounterBlendOutTime, ParryExchangeState.SuccessMontage);
	}

	ParryExchangeState.SuccessMontage = nullptr;
	ResetPendingAttackMotionState();
	ResetCurrentAttackRuntimeState();

	FMonsterAttackSelection CounterSelection;
	CounterSelection.Montage = CounterMontage;
	CounterSelection.AttackRange = ParryCounterAttackRange;
	CounterSelection.bFaceTargetDuringAttack = true;
	CounterSelection.FacingDuration = ParryCounterFacingDuration;
	CounterSelection.FacingInterpSpeed = ParryCounterFacingInterpSpeed;
	CounterSelection.PlayRate = ParryCounterPlayRate;
	CounterSelection.bTryTurnAfterAttack = true;
	CounterSelection.PostAttackTurnStartAngle = TurnStartAngle;

	FBossAttackExecutionRequest ExecutionRequest;
	ExecutionRequest.Selection = CounterSelection;
	ExecutionRequest.RuntimeState.ActionType = EBossActionType::ParryCounter;
	ExecutionRequest.InitiativeState = EBossInitiativeState::BossParryCounter;
	ExecutionRequest.BlendInTime = ParryCounterBlendInTime;
	if (!ProcessBossAttackExecution(ExecutionRequest))
	{
		FinishAttack();
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::ParryEnded);
		return false;
	}

	return true;
}

bool AJunBoss::TryStartParryCounterFollowUp()
{
	if (bExecutionReady ||
		bBeingExecuted ||
		ParryExchangeState.bCounterFollowUpStarted ||
		ParryExchangeState.bCounterPerfectParried ||
		CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::Attack ||
		CurrentAttackRuntime.ActionType != EBossActionType::ParryCounter ||
		InitiativeState != EBossInitiativeState::BossParryCounter)
	{
		return false;
	}

	UAnimMontage* FollowUpMontage = GetParryCounterFollowUpMontage();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!FollowUpMontage || !AnimInstance)
	{
		return false;
	}

	ParryExchangeState.bCounterFollowUpStarted = true;

	if (CurrentAttackMontage)
	{
		AnimInstance->Montage_Stop(ParryCounterFollowUpBlendOutTime, CurrentAttackMontage);
	}

	StopAllAttackTraces();
	ResetPendingAttackMotionState();

	FMonsterAttackSelection FollowUpSelection;
	FollowUpSelection.Montage = FollowUpMontage;
	FollowUpSelection.AttackRange = ParryCounterFollowUpAttackRange;
	FollowUpSelection.bFaceTargetDuringAttack = true;
	FollowUpSelection.FacingDuration = ParryCounterFollowUpFacingDuration;
	FollowUpSelection.FacingInterpSpeed = ParryCounterFollowUpFacingInterpSpeed;
	FollowUpSelection.PlayRate = ParryCounterFollowUpPlayRate;
	FollowUpSelection.bTryTurnAfterAttack = true;
	FollowUpSelection.PostAttackTurnStartAngle = TurnStartAngle;

	FBossAttackExecutionRequest ExecutionRequest;
	ExecutionRequest.Selection = FollowUpSelection;
	ExecutionRequest.RuntimeState = CurrentAttackRuntime;
	ExecutionRequest.InitiativeState = EBossInitiativeState::BossParryCounter;
	ExecutionRequest.BlendInTime = ParryCounterFollowUpBlendInTime;
	if (!ProcessBossAttackExecution(ExecutionRequest))
	{
		return false;
	}

	return true;
}

void AJunBoss::OnParrySuccessMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ParryExchangeState.SuccessMontage || Montage != ParryExchangeState.SuccessMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	FinishParrySuccessState();
}

void AJunBoss::FinishParrySuccessStateFromNotify()
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	FinishParrySuccessState();
}

void AJunBoss::FinishParrySuccessState()
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunBoss::OnParrySuccessMontageEnded);
	}

	ParryExchangeState.SuccessMontage = nullptr;
	ParryExchangeState.SuccessFallbackRemainTime = 0.f;
	ParryExchangeState.SuccessExitActionLockRemainTime = FMath::Max(0.f, ParrySuccessExitActionLockDuration);
	CodeFacingLockRemainTime = FMath::Max(CodeFacingLockRemainTime, ParrySuccessExitActionLockDuration);
	ParryExchangeState.ResetCounterProgress();

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	StopAllAttackTraces();
	ResetPendingAttackMotionState();
	RestoreAttackGroundMotionOverride();
	ClearTimedMontageSuperArmor();
	ResetCurrentAttackRuntimeState();

	if (CurrentState == EMonsterState::Combat &&
		CurrentCombatSubState == EBossCombatState::ParrySuccess)
	{
		if (InitiativeState == EBossInitiativeState::BossParryCounter)
		{
			InitiativeState = EBossInitiativeState::Neutral;
		}

		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::ParryEnded);
	}
}

