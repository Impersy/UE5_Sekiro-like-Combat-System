#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

float AJunBoss::GetTargetYawAbsDelta() const
{
	return FMath::Abs(GetCombatTargetYawDelta());
}

bool AJunBoss::CanStartBossTurn() const
{
	return CanStartCombatTurn() && GetTargetYawAbsDelta() >= TurnStartAngle;
}

bool AJunBoss::TryStartOrWaitForBossTurn(float YawThreshold)
{
	if (GetTargetYawAbsDelta() < YawThreshold)
	{
		return false;
	}

	if (CanStartBossTurn())
	{
		TransitionBossCombatState(EBossCombatState::Turn, EBossCombatStateChangeReason::TurnStarted);
		return true;
	}

	// Preserve the large-angle wait until movement inertia allows the turn montage.
	StopMovementForBossTurn();
	return true;
}

AJunBoss::FBossFacingDecision AJunBoss::EvaluateFacingTowardsTargetWithoutTurn() const
{
	FBossFacingDecision Decision;
	if (!CurrentTarget ||
		CodeFacingLockRemainTime > 0.f ||
		CurrentCombatSubState == EBossCombatState::Idle ||
		CurrentCombatSubState == EBossCombatState::ParrySuccess)
	{
		return Decision;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return Decision;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = ToTarget.Rotation();
	const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw));
	const bool bMovementFacingState =
		CurrentCombatSubState == EBossCombatState::Approach ||
		CurrentCombatSubState == EBossCombatState::Reposition ||
		CurrentCombatSubState == EBossCombatState::Evade;
	if (!bMovementFacingState && YawDelta > MaxCodeFacingAngle)
	{
		if (CanStartBossTurn())
		{
			Decision.Type = EBossFacingDecisionType::StartTurn;
		}
		return Decision;
	}

	Decision.Type = EBossFacingDecisionType::Rotate;
	Decision.TargetRotation = TargetRotation;
	return Decision;
}

void AJunBoss::ApplyFacingDecision(
	const FBossFacingDecision& Decision,
	float DeltaTime,
	float InterpSpeed)
{
	switch (Decision.Type)
	{
	case EBossFacingDecisionType::Rotate:
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Decision.TargetRotation, DeltaTime, InterpSpeed));
		break;
	case EBossFacingDecisionType::StartTurn:
		TransitionBossCombatState(EBossCombatState::Turn, EBossCombatStateChangeReason::TurnStarted);
		break;
	case EBossFacingDecisionType::None:
	default:
		break;
	}
}

void AJunBoss::UpdateFacingTowardsTargetWithoutTurn(float DeltaTime, float InterpSpeed)
{
	ApplyFacingDecision(EvaluateFacingTowardsTargetWithoutTurn(), DeltaTime, InterpSpeed);
}

void AJunBoss::StopMovementForBossTurn()
{
	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	bRunLocomotionRequested = false;
}

bool AJunBoss::ShouldTurnAfterHitReact(ECharacterHitReactDirection HitDirection) const
{
	if (!CurrentTarget || GetTargetYawAbsDelta() < TurnStartAngle)
	{
		return false;
	}

	return HitDirection == ECharacterHitReactDirection::Left_L ||
		HitDirection == ECharacterHitReactDirection::Right_R ||
		HitDirection == ECharacterHitReactDirection::Back_B;
}

bool AJunBoss::TryStartHitReactTurnTransition(bool bStopHitReactMontage)
{
	if (CurrentState != EMonsterState::Combat ||
		CurrentCombatSubState != EBossCombatState::Hit ||
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

	StopMovementForBossTurn();

	bHitReactTurnTransitionInProgress = true;
	EndHitReact();
	bHitReactTurnTransitionInProgress = false;

	TransitionBossCombatState(EBossCombatState::Turn, EBossCombatStateChangeReason::TurnStarted);
	return true;
}

void AJunBoss::EnterTurnState()
{
	bRunLocomotionRequested = false;
	TryStartCombatTurnWithThreshold(TurnStartAngle);
}

void AJunBoss::UpdateTurnState(float DeltaTime)
{
	if (!IsCombatTurnPlaying())
	{
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::TurnEnded);
		return;
	}

	if (bEarlyBlendOutCombatTurnOnTargetYaw &&
		FMath::Abs(GetCombatTargetYawDelta()) <= CombatTurnEarlyBlendOutYawTolerance)
	{
		FinishCombatTurnEarly(CombatTurnEarlyBlendOutTime);
		TransitionBossCombatState(EBossCombatState::Idle, EBossCombatStateChangeReason::TurnEnded);
	}
}
