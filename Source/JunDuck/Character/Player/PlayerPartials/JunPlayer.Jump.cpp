#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#pragma region Jump / Landing
// ============================================================================
// Jump / Landing
// ============================================================================

void AJunPlayer::Jump()
{
	if (bIsDodgeAttacking)
	{
		return;
	}

	if (bIsHeavyAttacking)
	{
		if (!CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Jump))
		{
			BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction::Jump);
			return;
		}

		CancelHeavyAttackForRecoveryTransition(HeavyAttackJumpCancelBlendOutTime);
	}

	if (bIsJigenAttacking)
	{
		if (!CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction::Jump))
		{
			BufferJigenRecoveryAction(EJunBufferedRecoveryAction::Jump);
			return;
		}

		CancelJigenForRecoveryTransition(JigenJumpCancelBlendOutTime);
	}

	if (CurrentHitState == EJunPlayerHitState::HitReact)
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Block_Jump))
	{
		return;
	}

	if (!CanJump())
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::Jump),
		[this]()
		{
			ProcessJumpStart();
		});
}

void AJunPlayer::ProcessJumpStart()
{
	CancelLockOnTurn();
	JumpStartAnimTriggerRemainTime = JumpStartAnimTriggerDuration;

	const FVector2D MoveInput(DesiredMoveForward, DesiredMoveRight);
	LaunchCharacter(BuildJumpLaunchVelocity(MoveInput), true, true);
	SetLocomotionState(EJunPlayerLocomotionState::Airborne);
	ClearActionStateIf(EJunPlayerActionState::Jump, EJunActionTransitionReason::System);
}

void AJunPlayer::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	LocomotionLandingRemainTime = FMath::Max(0.f, LocomotionLandingStateDuration);
	SetLocomotionState(EJunPlayerLocomotionState::Landing);

	if (bJumpCounterStompFollowUpActive)
	{
		FinishJumpCounterStompFollowUp(false);
	}

	const bool bAirParryActive = DefenseComponent
		? DefenseComponent->IsAirParrySequenceActive()
		: false;
	if (bAirParryActive)
	{
		const float ResolvedAirGuardLandCancelBlendOutTime = DefenseComponent
			? DefenseComponent->GetAirGuardLandCancelBlendOutTime()
			: AirGuardLandCancelBlendOutTime;

		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

			if (AirGuardStartMontage)
			{
				AnimInstance->Montage_Stop(ResolvedAirGuardLandCancelBlendOutTime, AirGuardStartMontage);
			}

			if (AirGuardEndMontage)
			{
				AnimInstance->Montage_Stop(ResolvedAirGuardLandCancelBlendOutTime, AirGuardEndMontage);
			}
		}

		FinishDefense();
	}

	if (CurrentHitState == EJunPlayerHitState::HitReact &&
		(AirHeavyHitStage == EJunAirHeavyHitStage::Launch ||
			AirHeavyHitStage == EJunAirHeavyHitStage::Down))
	{
		StartAirHeavyHitLandStage();
		bJumpCounterFollowUpDefenseBypassAvailable = false;
		bJumpCounterStompFollowUpAvailable = false;
		JumpCounterStompTarget = nullptr;
		return;
	}

	if (CurrentHitState == EJunPlayerHitState::GuardBreak && bAirGuardBreakLandMontagePending)
	{
		StartAirGuardBreakLandMontage();
		bJumpCounterFollowUpDefenseBypassAvailable = false;
		bJumpCounterStompFollowUpAvailable = false;
		JumpCounterStompTarget = nullptr;
		return;
	}

	if (bIsJumpAttacking)
	{
		RequestJumpAttackEnd();
		return;
	}

	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	JumpCounterStompTarget = nullptr;
}

#pragma endregion
