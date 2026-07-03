#include "Character/Monster/Boss/JunBoss.h"
#include "Character/Monster/Boss/BossAttackSelectionStrategy.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

namespace
{
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

bool AJunBoss::HasAnyAttackCandidateForCurrentDistance() const
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

	for (const EBossNormalAttackType NormalAttackType : AllBossNormalAttackTypes)
	{
		if (IsBowNormalAttackType(NormalAttackType) && ShouldSuppressBowAttackCandidate())
		{
			continue;
		}

		if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FBossNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (NormalAttackData &&
			NormalAttackData->Montage &&
			IsWithinRange(NormalAttackData->MinRange, NormalAttackData->CandidateMaxRange))
		{
			return true;
		}
	}

	return false;
}

bool AJunBoss::HasValidPlannedAttack() const
{
	switch (CurrentCombatPlan.AttackType)
	{
	case EBossPlannedAttackType::ComboAttack:
		return GetPlannedComboMontage() != nullptr;
	case EBossPlannedAttackType::NormalAttack:
		{
			const FBossNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData && NormalAttackData->Montage != nullptr;
		}
	case EBossPlannedAttackType::None:
	default:
		return false;
	}
}

bool AJunBoss::HasValidPlannedMovement() const
{
	if (CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Strafe)
	{
		return true;
	}

	if (CurrentCombatPlan.NonAttackType == EBossPlannedNonAttackType::Hold)
	{
		return false;
	}

	if (IsMobilityNonAttackType(CurrentCombatPlan.NonAttackType))
	{
		return GetPlannedNonAttackMontage() != nullptr;
	}

	return false;
}

bool AJunBoss::IsMobilityNonAttackType(EBossPlannedNonAttackType NonAttackType) const
{
	return NonAttackType == EBossPlannedNonAttackType::Strafe ||
		NonAttackType == EBossPlannedNonAttackType::Dash ||
		NonAttackType == EBossPlannedNonAttackType::Dodge;
}

bool AJunBoss::IsComboSetEnabledByTestFilter(EBossComboSet ComboSet) const
{
	if (!bUseAttackPatternTestFilter)
	{
		return IsComboSetAllowedInCurrentPhase(ComboSet);
	}

	bool bEnabled = false;
	switch (ComboSet)
	{
	case EBossComboSet::ComboA:
		bEnabled = bTestEnableComboA;
		break;
	case EBossComboSet::ComboB:
		bEnabled = bTestEnableComboB;
		break;
	case EBossComboSet::None:
	default:
		return false;
	}

	return bEnabled && IsComboSetAllowedInCurrentPhase(ComboSet);
}

bool AJunBoss::IsNormalAttackEnabledByTestFilter(EBossNormalAttackType NormalAttackType) const
{
	if (!bUseAttackPatternTestFilter)
	{
		return IsNormalAttackAllowedInCurrentPhase(NormalAttackType);
	}

	bool bEnabled = false;
	switch (NormalAttackType)
	{
	case EBossNormalAttackType::JumpAttack:
		bEnabled = bTestEnableJumpAttack;
		break;
	case EBossNormalAttackType::ChargeAttack:
		bEnabled = bTestEnableChargeAttack;
		break;
	case EBossNormalAttackType::DodgeAttack:
		bEnabled = bTestEnableDodgeAttack;
		break;
	case EBossNormalAttackType::NinjaA:
		bEnabled = bTestEnableNinjaA;
		break;
	case EBossNormalAttackType::NinjaB:
		bEnabled = bTestEnableNinjaB;
		break;
	case EBossNormalAttackType::Execution:
		bEnabled = bTestEnableExecution;
		break;
	case EBossNormalAttackType::FastComeSlash:
		bEnabled = bTestEnableFastComeSlash;
		break;
	case EBossNormalAttackType::SlowComeSlash:
		bEnabled = bTestEnableSlowComeSlash;
		break;
	case EBossNormalAttackType::FakeDownSlash:
		bEnabled = bTestEnableFakeDownSlash;
		break;
	case EBossNormalAttackType::LionSlash:
		bEnabled = bTestEnableLionSlash;
		break;
	case EBossNormalAttackType::SpinSlash:
		bEnabled = bTestEnableSpinSlash;
		break;
	case EBossNormalAttackType::LightningSword:
		bEnabled = bTestEnableLightningSword;
		break;
	case EBossNormalAttackType::Bow4Combo:
		bEnabled = bTestEnableBow4Combo;
		break;
	case EBossNormalAttackType::BowBackDashAttack:
		bEnabled = bTestEnableBowBackDashAttack;
		break;
	case EBossNormalAttackType::BowHeavyAttack:
		bEnabled = bTestEnableBowHeavyAttack;
		break;
	case EBossNormalAttackType::None:
	default:
		return false;
	}

	return bEnabled && IsNormalAttackAllowedInCurrentPhase(NormalAttackType);
}

bool AJunBoss::IsPhaseTwoActive() const
{
	return GetCurrentLifeCount() < GetMaxLifeCount();
}

bool AJunBoss::IsNormalAttackAllowedByCurrentPhase(EBossNormalAttackType NormalAttackType) const
{
	return IsNormalAttackAllowedInCurrentPhase(NormalAttackType);
}

bool AJunBoss::IsComboSetAllowedInCurrentPhase(EBossComboSet ComboSet) const
{
	bool bRequiresPhaseTwo = false;
	switch (ComboSet)
	{
	case EBossComboSet::ComboA:
		bRequiresPhaseTwo = bTestComboARequiresPhaseTwo;
		break;
	case EBossComboSet::ComboB:
		bRequiresPhaseTwo = bTestComboBRequiresPhaseTwo;
		break;
	case EBossComboSet::None:
	default:
		return false;
	}

	return !bRequiresPhaseTwo || IsPhaseTwoActive();
}

bool AJunBoss::IsNormalAttackAllowedInCurrentPhase(EBossNormalAttackType NormalAttackType) const
{
	bool bRequiresPhaseTwo = false;
	switch (NormalAttackType)
	{
	case EBossNormalAttackType::JumpAttack:
		bRequiresPhaseTwo = bTestJumpAttackRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::ChargeAttack:
		bRequiresPhaseTwo = bTestChargeAttackRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::DodgeAttack:
		bRequiresPhaseTwo = bTestDodgeAttackRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::NinjaA:
		bRequiresPhaseTwo = bTestNinjaARequiresPhaseTwo;
		break;
	case EBossNormalAttackType::NinjaB:
		bRequiresPhaseTwo = bTestNinjaBRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::Execution:
		bRequiresPhaseTwo = bTestExecutionRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::FastComeSlash:
		bRequiresPhaseTwo = bTestFastComeSlashRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::SlowComeSlash:
		bRequiresPhaseTwo = bTestSlowComeSlashRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::FakeDownSlash:
		bRequiresPhaseTwo = bTestFakeDownSlashRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::LionSlash:
		bRequiresPhaseTwo = bTestLionSlashRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::SpinSlash:
		bRequiresPhaseTwo = bTestSpinSlashRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::LightningSword:
		bRequiresPhaseTwo = bTestLightningSwordRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::Bow4Combo:
		bRequiresPhaseTwo = bTestBow4ComboRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::BowBackDashAttack:
		bRequiresPhaseTwo = bTestBowBackDashAttackRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::BowHeavyAttack:
		bRequiresPhaseTwo = bTestBowHeavyAttackRequiresPhaseTwo;
		break;
	case EBossNormalAttackType::None:
	default:
		return false;
	}

	return !bRequiresPhaseTwo || IsPhaseTwoActive();
}

float AJunBoss::GetMaxEnabledComboCandidateRange() const
{
	float MaxCandidateRange = 0.f;
	if (IsComboSetEnabledByTestFilter(EBossComboSet::ComboA) && IsComboAttackUsable(ComboAttackA))
	{
		MaxCandidateRange = FMath::Max(MaxCandidateRange, ComboAttackA.CandidateRange);
	}

	if (IsComboSetEnabledByTestFilter(EBossComboSet::ComboB) && IsComboAttackUsable(ComboAttackB))
	{
		MaxCandidateRange = FMath::Max(MaxCandidateRange, ComboAttackB.CandidateRange);
	}

	return MaxCandidateRange;
}

float AJunBoss::GetMaxEnabledAttackCandidateRange() const
{
	float MaxCandidateRange = 0.f;
	for (const EBossNormalAttackType NormalAttackType : AllBossNormalAttackTypes)
	{
		if (IsBowNormalAttackType(NormalAttackType) && ShouldSuppressBowAttackCandidate())
		{
			continue;
		}

		if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FBossNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (NormalAttackData && NormalAttackData->Montage)
		{
			MaxCandidateRange = FMath::Max(MaxCandidateRange, NormalAttackData->CandidateMaxRange);
		}
	}

	return MaxCandidateRange;
}

bool AJunBoss::IsFartherThanAllAttackCandidates() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	const float MaxCandidateRange = GetMaxEnabledAttackCandidateRange();
	return MaxCandidateRange > 0.f && GetTargetDistance2D() > MaxCandidateRange;
}

bool AJunBoss::HasAnyNonBowAttackCandidateForCurrentDistance() const
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

	for (const EBossNormalAttackType NormalAttackType : AllBossNormalAttackTypes)
	{
		if (IsBowNormalAttackType(NormalAttackType) || !IsNormalAttackEnabledByTestFilter(NormalAttackType))
		{
			continue;
		}

		const FBossNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
		if (NormalAttackData &&
			NormalAttackData->Montage &&
			IsWithinRange(NormalAttackData->MinRange, NormalAttackData->CandidateMaxRange))
		{
			return true;
		}
	}

	return false;
}

EBossMovementDirection AJunBoss::ChooseStrafeDirectionAvoidingImmediateReverse() const
{
	EBossMovementDirection ChosenDirection = FMath::RandBool()
		? EBossMovementDirection::Left
		: EBossMovementDirection::Right;

	if (LastCompletedNonAttackType == EBossPlannedNonAttackType::Strafe &&
		IsOppositeStrafeDirection(ChosenDirection, LastCompletedStrafeDirection))
	{
		ChosenDirection = LastCompletedStrafeDirection;
	}

	return ChosenDirection;
}

bool AJunBoss::IsOppositeStrafeDirection(EBossMovementDirection A, EBossMovementDirection B) const
{
	return (A == EBossMovementDirection::Left && B == EBossMovementDirection::Right) ||
		(A == EBossMovementDirection::Right && B == EBossMovementDirection::Left);
}

void AJunBoss::UpdatePlannedCombatPlanAge(float DeltaTime)
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

	if (CurrentCombatPlan.ActionType == EBossPlannedActionType::Attack &&
		CurrentCombatSubState != EBossCombatState::Attack)
	{
		CurrentCombatPlan.AttackWaitTime += DeltaTime;
		if (IsPlannedAttackExpired())
		{
			ResetPlannedCombatPlan();
		}
	}
	else if (CurrentCombatSubState == EBossCombatState::Attack ||
		CurrentCombatPlan.ActionType != EBossPlannedActionType::Attack)
	{
		CurrentCombatPlan.AttackWaitTime = 0.f;
	}
}

bool AJunBoss::IsPlannedAttackExpired() const
{
	return PlannedAttackMaxWaitTime > 0.f &&
		CurrentCombatPlan.AttackWaitTime >= PlannedAttackMaxWaitTime;
}

bool AJunBoss::ShouldForceCloseAfterBowAttack() const
{
	return bForceApproachAfterBowAttack &&
		bPostBowCloseRangeApproachInProgress &&
		(PostBowCloseRangeForceMaxTime <= 0.f ||
			PostBowCloseRangeForceElapsedTime < PostBowCloseRangeForceMaxTime);
}

bool AJunBoss::ShouldSuppressBowAttackCandidate() const
{
	return ShouldForceCloseAfterBowAttack();
}

void AJunBoss::CollectNormalAttackCandidates(
	float TargetDistance,
	const FBossNormalAttackCandidatePolicy& Policy,
	TArray<FBossWeightedNormalAttackCandidate>& OutCandidates) const
{
	for (const EBossNormalAttackType NormalAttackType : AllBossNormalAttackTypes)
	{
		const FBossNormalAttackData* NormalAttackData = nullptr;
		if (CanUseNormalAttackCandidate(NormalAttackType, TargetDistance, Policy, NormalAttackData))
		{
			OutCandidates.Add({NormalAttackType, FMath::Max(1, NormalAttackData->SelectionWeight)});
		}
	}
}

bool AJunBoss::CanUseNormalAttackCandidate(
	EBossNormalAttackType NormalAttackType,
	float TargetDistance,
	const FBossNormalAttackCandidatePolicy& Policy,
	const FBossNormalAttackData*& OutNormalAttackData) const
{
	OutNormalAttackData = nullptr;

	const bool bIsBowAttack = IsBowNormalAttackType(NormalAttackType);
	if (bIsBowAttack && ShouldSuppressBowAttackCandidate())
	{
		return false;
	}

	if (!IsNormalAttackEnabledByTestFilter(NormalAttackType))
	{
		return false;
	}

	const FBossNormalAttackData* NormalAttackData = GetNormalAttackData(NormalAttackType);
	if (!NormalAttackData || !NormalAttackData->Montage)
	{
		return false;
	}

	if (!Policy.bIgnoreRange)
	{
		const bool bWithinRange =
			TargetDistance >= NormalAttackData->MinRange &&
			TargetDistance <= NormalAttackData->CandidateMaxRange;
		if (!bWithinRange)
		{
			return false;
		}
	}

	if (!Policy.bIgnoreRepeat && WasRecentlyUsed(NormalAttackType, 1))
	{
		return false;
	}

	if (!Policy.bIgnoreRepeat && NormalAttackType == LastStartedNormalAttackType)
	{
		return false;
	}

	OutNormalAttackData = NormalAttackData;
	return true;
}

EBossNormalAttackType AJunBoss::SelectNormalAttackCandidate(
	const TArray<FBossWeightedNormalAttackCandidate>& Candidates) const
{
	static const FWeightedRandomBossAttackSelectionStrategy SelectionStrategy;
	return SelectionStrategy.SelectNormalAttack(Candidates);
}

int32 AJunBoss::GetComboAttackSelectionWeight() const
{
	int32 ComboAttackSelectionWeight = 0;
	if (IsComboSetEnabledByTestFilter(EBossComboSet::ComboA) && IsComboAttackUsable(ComboAttackA))
	{
		ComboAttackSelectionWeight += FMath::Max(1, ComboAttackA.SelectionWeight);
	}

	if (IsComboSetEnabledByTestFilter(EBossComboSet::ComboB) && IsComboAttackUsable(ComboAttackB))
	{
		ComboAttackSelectionWeight += FMath::Max(1, ComboAttackB.SelectionWeight);
	}

	return ComboAttackSelectionWeight;
}

bool AJunBoss::HasAnyComboMontage() const
{
	return (IsComboSetEnabledByTestFilter(EBossComboSet::ComboA) && IsComboAttackUsable(ComboAttackA)) ||
		(IsComboSetEnabledByTestFilter(EBossComboSet::ComboB) && IsComboAttackUsable(ComboAttackB));
}

const FBossComboAttackData* AJunBoss::GetComboAttackData(EBossComboSet ComboSet) const
{
	switch (ComboSet)
	{
	case EBossComboSet::ComboA:
		return &ComboAttackA;
	case EBossComboSet::ComboB:
		return &ComboAttackB;
	case EBossComboSet::None:
	default:
		return nullptr;
	}
}

bool AJunBoss::IsComboAttackUsable(const FBossComboAttackData& ComboAttackData) const
{
	return ComboAttackData.Montage != nullptr && GetComboAttackMaxLength(ComboAttackData) >= 2;
}

int32 AJunBoss::GetComboAttackMaxLength(const FBossComboAttackData& ComboAttackData) const
{
	return FMath::Max(0, ComboAttackData.MaxComboLength);
}

FName AJunBoss::GetComboSectionName(int32 ComboLength) const
{
	return FName(*FString::FromInt(ComboLength));
}

const FBossReactiveActionTuningData* AJunBoss::GetReactiveActionTuningData(EBossReactiveActionType ReactiveActionType) const
{
	switch (ReactiveActionType)
	{
	case EBossReactiveActionType::EvadeBackward:
		return &ReactiveBackwardEvade;
	case EBossReactiveActionType::None:
	default:
		return nullptr;
	}
}

bool AJunBoss::IsWithinSelectionRange(const FMonsterAttackSelection& Selection) const
{
	if (!CurrentTarget || !Selection.Montage)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return DistSq <= FMath::Square(Selection.AttackRange);
}

const FBossNormalAttackData* AJunBoss::GetNormalAttackData(EBossNormalAttackType AttackType) const
{
	switch (AttackType)
	{
	case EBossNormalAttackType::JumpAttack:
		return &JumpAttack;
	case EBossNormalAttackType::ChargeAttack:
		return &ChargeAttack;
	case EBossNormalAttackType::DodgeAttack:
		return &DodgeAttack;
	case EBossNormalAttackType::NinjaA:
		return &NinjaAAttack;
	case EBossNormalAttackType::NinjaB:
		return &NinjaBAttack;
	case EBossNormalAttackType::Execution:
		return &ExecutionAttack;
	case EBossNormalAttackType::FastComeSlash:
		return &FastComeSlashAttack;
	case EBossNormalAttackType::SlowComeSlash:
		return &SlowComeSlashAttack;
	case EBossNormalAttackType::FakeDownSlash:
		return &FakeDownSlashAttack;
	case EBossNormalAttackType::LionSlash:
		return &LionSlashAttack;
	case EBossNormalAttackType::SpinSlash:
		return &SpinSlashAttack;
	case EBossNormalAttackType::LightningSword:
		return &LightningSwordAttack;
	case EBossNormalAttackType::Bow4Combo:
		return &Bow4ComboAttack;
	case EBossNormalAttackType::BowBackDashAttack:
		return &BowBackDashAttack;
	case EBossNormalAttackType::BowHeavyAttack:
		return &BowHeavyAttack;
	case EBossNormalAttackType::None:
	default:
		return nullptr;
	}
}

const FBossNormalAttackData* AJunBoss::GetPlannedNormalAttackData() const
{
	return GetNormalAttackData(CurrentCombatPlan.NormalAttackType);
}

UAnimMontage* AJunBoss::GetPlannedComboMontage() const
{
	const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentCombatPlan.ComboSet);

	if (!ComboAttackData || !ComboAttackData->Montage || CurrentCombatPlan.ComboLength <= 0)
	{
		return nullptr;
	}

	return CurrentCombatPlan.ComboLength <= GetComboAttackMaxLength(*ComboAttackData)
		? ComboAttackData->Montage.Get()
		: nullptr;
}

float AJunBoss::GetPlannedComboFacingDuration() const
{
	const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentCombatPlan.ComboSet);
	const TArray<float>* FacingDurations = ComboAttackData ? &ComboAttackData->FacingDurations : nullptr;

	if (!FacingDurations || CurrentCombatPlan.ComboLength <= 0)
	{
		return -1.f;
	}

	const int32 DurationIndex = CurrentCombatPlan.ComboLength - 1;
	return FacingDurations->IsValidIndex(DurationIndex)
		? (*FacingDurations)[DurationIndex]
		: -1.f;
}

void AJunBoss::ConfigurePlannedComboMontageSections()
{
	if (CurrentCombatPlan.AttackType != EBossPlannedAttackType::ComboAttack)
	{
		return;
	}

	const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentCombatPlan.ComboSet);
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

UAnimMontage* AJunBoss::GetMobilityNonAttackMontage(EBossPlannedNonAttackType NonAttackType, EBossMovementDirection MovementDirection) const
{
	switch (NonAttackType)
	{
	case EBossPlannedNonAttackType::Dash:
		switch (MovementDirection)
		{
		case EBossMovementDirection::Forward:
			return DashForwardMontage.Get();
		case EBossMovementDirection::Backward:
			return DashBackwardMontage.Get();
		case EBossMovementDirection::Left:
			return DashLeftMontage.Get();
		case EBossMovementDirection::Right:
		default:
			return DashRightMontage.Get();
		}
	case EBossPlannedNonAttackType::Dodge:
		switch (MovementDirection)
		{
		case EBossMovementDirection::Forward:
			return DodgeForwardMontage.Get();
		case EBossMovementDirection::Backward:
			return DodgeBackwardMontage.Get();
		case EBossMovementDirection::Left:
			return DodgeLeftMontage.Get();
		case EBossMovementDirection::Right:
		default:
			return DodgeRightMontage.Get();
		}
	case EBossPlannedNonAttackType::Strafe:
	case EBossPlannedNonAttackType::Hold:
	case EBossPlannedNonAttackType::None:
	default:
		return nullptr;
	}
}

UAnimMontage* AJunBoss::GetPlannedNonAttackMontage() const
{
	if (IsMobilityNonAttackType(CurrentCombatPlan.NonAttackType))
	{
		return GetMobilityNonAttackMontage(CurrentCombatPlan.NonAttackType, CurrentCombatPlan.MovementDirection);
	}

	return nullptr;
}

UAnimMontage* AJunBoss::GetReactiveActionMontage() const
{
	return GetMobilityNonAttackMontage(ReactiveActionState.ActiveAction.NonAttackType, ReactiveActionState.ActiveAction.MovementDirection);
}

EBossCombatState AJunBoss::GetReactiveActionTargetState(EBossReactiveActionType ReactiveActionType) const
{
	switch (ReactiveActionType)
	{
	case EBossReactiveActionType::EvadeBackward:
		return EBossCombatState::Evade;
	case EBossReactiveActionType::None:
	default:
		return EBossCombatState::Idle;
	}
}

void AJunBoss::RecordAction(EBossActionType ActionType, EBossComboSet ComboSet, int32 ComboLength, EBossNormalAttackType NormalAttackType)
{
	FBossActionRecord Record;
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
		ActionType == EBossActionType::NormalAttack &&
		IsBowNormalAttackType(NormalAttackType))
	{
		bPostBowCloseRangeApproachInProgress = true;
		PostBowCloseRangeForceElapsedTime = 0.f;
	}
}

bool AJunBoss::WasRecentlyUsed(EBossActionType ActionType, int32 Depth) const
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

bool AJunBoss::WasRecentlyUsed(EBossComboSet ComboSet, int32 ComboLength, int32 Depth) const
{
	const int32 HistoryNum = RecentActionHistory.Num();
	const int32 NumToCheck = FMath::Clamp(Depth, 0, HistoryNum);
	for (int32 Offset = 0; Offset < NumToCheck; ++Offset)
	{
		const int32 Index = HistoryNum - 1 - Offset;
		const FBossActionRecord& Record = RecentActionHistory[Index];
		if (Record.ComboSet == ComboSet && Record.ComboLength == ComboLength)
		{
			return true;
		}
	}

	return false;
}

bool AJunBoss::WasRecentlyUsed(EBossNormalAttackType AttackType, int32 Depth) const
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

bool AJunBoss::WasNormalAttackRecentlyUsed(EBossNormalAttackType AttackType, int32 Depth) const
{
	return WasRecentlyUsed(AttackType, Depth);
}

bool AJunBoss::WasRecentlyUsedBowNormalAttack(int32 Depth) const
{
	const int32 HistoryNum = RecentActionHistory.Num();
	const int32 NumToCheck = FMath::Clamp(Depth, 0, HistoryNum);
	for (int32 Offset = 0; Offset < NumToCheck; ++Offset)
	{
		const int32 Index = HistoryNum - 1 - Offset;
		const FBossActionRecord& Record = RecentActionHistory[Index];
		if (Record.ActionType == EBossActionType::NormalAttack && IsBowNormalAttackType(Record.NormalAttackType))
		{
			return true;
		}
	}

	return false;
}

float AJunBoss::GetTargetDistance2D() const
{
	if (!CurrentTarget)
	{
		return 0.f;
	}

	return FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
}

float AJunBoss::GetPlannedAttackMinRange() const
{
	switch (CurrentCombatPlan.AttackType)
	{
	case EBossPlannedAttackType::NormalAttack:
		{
			const FBossNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData ? NormalAttackData->MinRange : 0.f;
		}
	case EBossPlannedAttackType::ComboAttack:
	case EBossPlannedAttackType::None:
	default:
		return 0.f;
	}
}

float AJunBoss::GetPlannedAttackMaxRange() const
{
	switch (CurrentCombatPlan.AttackType)
	{
	case EBossPlannedAttackType::NormalAttack:
		{
			const FBossNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData ? NormalAttackData->MaxRange : 0.f;
		}
	case EBossPlannedAttackType::ComboAttack:
		{
			const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentCombatPlan.ComboSet);
			return ComboAttackData ? ComboAttackData->AttackRange : 250.f;
		}
	case EBossPlannedAttackType::None:
	default:
		return 0.f;
	}
}

float AJunBoss::GetAttackCandidateMaxRange(EBossPlannedAttackType AttackType) const
{
	switch (AttackType)
	{
	case EBossPlannedAttackType::NormalAttack:
		{
			const FBossNormalAttackData* NormalAttackData = GetPlannedNormalAttackData();
			return NormalAttackData ? NormalAttackData->CandidateMaxRange : 0.f;
		}
	case EBossPlannedAttackType::ComboAttack:
		{
			const FBossComboAttackData* ComboAttackData = GetComboAttackData(CurrentCombatPlan.ComboSet);
			if (ComboAttackData)
			{
				return ComboAttackData->CandidateRange;
			}

			return FMath::Max(ComboAttackA.CandidateRange, ComboAttackB.CandidateRange);
		}
	case EBossPlannedAttackType::None:
	default:
		return 0.f;
	}
}

bool AJunBoss::IsTargetTooFarForPlannedAttack() const
{
	if (!CurrentTarget || !HasValidPlannedAttack())
	{
		return false;
	}

	return GetTargetDistance2D() > GetPlannedAttackMaxRange();
}

bool AJunBoss::IsTargetTooCloseForPlannedAttack() const
{
	if (!CurrentTarget || !HasValidPlannedAttack())
	{
		return false;
	}

	return GetTargetDistance2D() < GetPlannedAttackMinRange();
}

bool AJunBoss::CanTriggerCloseRangeBackDash() const
{
	return CloseRangeBackDashCooldownRemainTime <= 0.f && FMath::FRand() <= CloseRangeBackDashChance;
}

void AJunBoss::StartCloseRangeBackDashCooldown()
{
	CloseRangeBackDashCooldownRemainTime = FMath::Max(0.f, CloseRangeBackDashCooldown);
}

void AJunBoss::ResetReactiveActionState()
{
	ReactiveActionState.ResetQueuedAction();
}

void AJunBoss::ResetActiveReactiveActionState()
{
	ReactiveActionState.ResetActiveAction();
}

void AJunBoss::ResetReactiveEvadePressure()
{
	ReactiveActionState.ResetPressure();
}

void AJunBoss::ResetPendingAttackMotionState()
{
	StopCodeDrivenAttackMove(false);
	StopLightningSwordAirMove(false);
}

void AJunBoss::ResetCurrentAttackRuntimeState()
{
	ClearDelayedNormalAttackRangeLink();
	ClearVariantNormalAttackState();
	EndBowAttackPresentation();
	bAirborneHitReactLockWindowActive = false;
	Super::ResetCurrentAttackRuntimeState();
	CurrentAttackRuntime.Reset();
	ActiveSpecialReaction = EBossSpecialReactionType::None;
}
