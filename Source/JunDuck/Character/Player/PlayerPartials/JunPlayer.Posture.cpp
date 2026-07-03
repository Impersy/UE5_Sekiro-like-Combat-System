#include "Character/Player/JunPlayer.h"

#include "JunGameplayTags.h"

#pragma region Posture
// ============================================================================
// Posture
// ============================================================================

bool AJunPlayer::AddPosture(float Amount)
{
	if (GuardBreakVulnerableRemainTime > 0.f)
	{
		return IsPostureFull();
	}

	if (MaxPosture <= 0.f || Amount <= 0.f)
	{
		return IsPostureFull();
	}

	CurrentPosture = FMath::Clamp(CurrentPosture + Amount, 0.f, MaxPosture);
	PostureRecoveryDelayRemainTime = PostureRecoveryDelay;
	return IsPostureFull();
}

void AJunPlayer::ReducePosture(float Amount)
{
	if (MaxPosture <= 0.f || Amount <= 0.f)
	{
		return;
	}

	CurrentPosture = FMath::Max(0.f, CurrentPosture - Amount);
}

bool AJunPlayer::IsPostureFull() const
{
	return MaxPosture > 0.f && CurrentPosture >= MaxPosture;
}

void AJunPlayer::UpdatePostureRecovery(float DeltaTime)
{
	if (CurrentPosture <= 0.f || !CanRecoverPosture())
	{
		return;
	}

	if (PostureRecoveryDelayRemainTime > 0.f)
	{
		PostureRecoveryDelayRemainTime = FMath::Max(0.f, PostureRecoveryDelayRemainTime - DeltaTime);
		return;
	}

	const bool bGuardRecoveryBoost = GetDefenseState() == EJunDefenseState::Guarding;
	const bool bRunRecoveryPenalty = GetMoveState() == EJunMoveState::Run;
	const float RecoveryMultiplier = bGuardRecoveryBoost
		? GuardPostureRecoveryMultiplier
		: (bRunRecoveryPenalty ? RunPostureRecoveryMultiplier : 1.f);
	const float RecoveryAmount = PostureRecoveryRate * RecoveryMultiplier * DeltaTime;
	CurrentPosture = FMath::Max(0.f, CurrentPosture - RecoveryAmount);
}

bool AJunPlayer::CanRecoverPosture() const
{
	if (Is_Dead() || IsExecuting())
	{
		return false;
	}

	if (CurrentHitState != EJunPlayerHitState::None)
	{
		return false;
	}

	if (bIsBasicAttacking || bIsHeavyAttacking || bIsJigenAttacking || bIsJumpAttacking || bIsDodgeAttacking)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return false;
	}

	return true;
}

#pragma endregion