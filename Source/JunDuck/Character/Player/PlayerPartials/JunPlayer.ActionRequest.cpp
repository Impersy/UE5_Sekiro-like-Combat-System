#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "JunGameplayTags.h"

#pragma region Action Request Runtime
// ============================================================================
// Action Request Runtime
// ============================================================================

bool AJunPlayer::CanProcessPendingRecoveryActionRequest(const FJunPendingActionRequest& Request) const
{
	if (!Request.IsSet())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	if (!TryGetRecoveryCancelRule(Request, CancelRule))
	{
		return false;
	}

	return IsPendingActionCancelRuleOpen(Request, CancelRule) &&
		CanRequestResolvedBufferedAction(CancelRule.ToAction);
}

bool AJunPlayer::ShouldCancelCurrentActionForPendingRecoveryRequest(
	const FJunPendingActionRequest& Request) const
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
		return bIsBasicAttacking && CanCancelBasicAttackIntoRecoveryAction(Request.RecoveryAction);
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
		return bIsHeavyAttacking;
	case EJunPendingActionRequestSource::JigenRecovery:
		return bIsJigenAttacking;
	default:
		return false;
	}
}

float AJunPlayer::GetPendingRecoveryCancelBlendOutTime(const FJunPendingActionRequest& Request) const
{
	FJunPlayerCancelRule CancelRule;
	if (TryGetRecoveryCancelRule(Request, CancelRule))
	{
		return CancelRule.BlendOutTime;
	}

	return 0.f;
}

void AJunPlayer::CancelCurrentActionForPendingRecoveryRequest(
	const FJunPendingActionRequest& Request,
	float BlendOutTime)
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
		CancelBasicAttackForRecoveryTransition(BlendOutTime);
		break;
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
		CancelHeavyAttackForRecoveryTransition(BlendOutTime);
		break;
	case EJunPendingActionRequestSource::JigenRecovery:
		CancelJigenForRecoveryTransition(BlendOutTime);
		break;
	default:
		break;
	}
}

void AJunPlayer::ProcessPendingRecoveryActionRequest(const FJunPendingActionRequest& Request)
{
	switch (Request.RecoveryAction)
	{
	case EJunBufferedRecoveryAction::Dodge:
		StartDodge();
		break;
	case EJunBufferedRecoveryAction::Jump:
		Jump();
		break;
	case EJunBufferedRecoveryAction::Defense:
		BeginDefenseFromBufferedInput();
		break;
	default:
		break;
	}
}

bool AJunPlayer::TryProcessPendingRecoveryActionRequest(FJunPendingActionRequest& Request)
{
	return TryProcessPendingActionRequest(Request);
}

void AJunPlayer::CompleteGuardBlockReleaseIntoGuardEnd()
{
	GetWorldTimerManager().ClearTimer(GuardBlockReleaseIntoGuardEndTimerHandle);

	if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		FinishPlayerHitState();
	}
}

void AJunPlayer::TryProcessBufferedDefenseTransitionCancelAction()
{
	if (DefenseComponent)
	{
		DefenseComponent->TryProcessBufferedDefenseTransitionCancelAction();
	}
}

void AJunPlayer::ProcessBufferedDefenseTransitionCancelAction()
{
	if (DefenseComponent)
	{
		DefenseComponent->ProcessBufferedDefenseTransitionCancelAction();
		return;
	}
}

bool AJunPlayer::CanProcessPendingDefenseTransitionActionRequest(const FJunPendingActionRequest& Request) const
{
	if (Request.Source != EJunPendingActionRequestSource::DefenseTransition)
	{
		return false;
	}

	const EJunDefenseState CurrentDefenseState = GetDefenseState();
	const bool bCanProcessWindow = DefenseComponent &&
		(CurrentDefenseState == EJunDefenseState::Starting ||
		 CurrentDefenseState == EJunDefenseState::Ending ||
		 DefenseComponent->GetPostDefenseTransitionCancelBufferRemainTime() > 0.f);
	if (!bCanProcessWindow)
	{
		return false;
	}

	const EJunBufferedDefenseCancelAction PendingAction = Request.DefenseCancelAction;
	if (PendingAction == EJunBufferedDefenseCancelAction::Move)
	{
		return CanCancelDefenseTransitionIntoMove();
	}

	return CanRequestResolvedBufferedAction(ResolveBufferedDefenseCancelActionState(PendingAction));
}

float AJunPlayer::GetPendingDefenseTransitionCancelBlendOutTime(
	const FJunPendingActionRequest& Request) const
{
	return Request.DefenseCancelAction == EJunBufferedDefenseCancelAction::Move
		? (DefenseComponent
			? DefenseComponent->GetDefenseMoveCancelBlendOutTime()
			: DefenseMoveCancelBlendOutTime)
		: -1.f;
}

void AJunPlayer::ProcessPendingDefenseTransitionActionRequest(const FJunPendingActionRequest& Request)
{
	const EJunBufferedDefenseCancelAction PendingAction = Request.DefenseCancelAction;
	switch (PendingAction)
	{
	case EJunBufferedDefenseCancelAction::BasicAttack:
		CancelDefenseForCancelTransition();
		BasicAttack();
		break;
	case EJunBufferedDefenseCancelAction::HeavyAttack:
	{
		const float PreservedHeavyAttackHoldTime = HeavyAttackInputHoldTime;
		CancelDefenseForCancelTransition();
		bHeavyAttackInputHeld = true;
		HeavyAttackInputHoldTime = FMath::Max(PreservedHeavyAttackHoldTime, 0.f);
		HeavyAttackChargeLoopElapsedTime = 0.f;
		if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
		{
			StartHeavyAttackCharge();
		}
		break;
	}
	case EJunBufferedDefenseCancelAction::Jump:
		CancelDefenseForCancelTransition();
		Jump();
		break;
	case EJunBufferedDefenseCancelAction::Dodge:
		CancelDefenseForCancelTransition();
		StartDodge();
		break;
	case EJunBufferedDefenseCancelAction::Move:
		CancelDefenseForCancelTransition(GetPendingActionCancelBlendOutTime(Request));
		break;
	default:
		break;
	}
}

bool AJunPlayer::TryProcessPendingDefenseTransitionActionRequest(FJunPendingActionRequest& Request)
{
	return TryProcessPendingActionRequest(Request);
}

void AJunPlayer::TryProcessBufferedParrySuccessCancelAction()
{
	TryProcessPendingParrySuccessActionRequest(PendingParrySuccessActionRequest);
}

void AJunPlayer::CancelParrySuccessForCancelTransition(float BlendOutTime)
{
	if (AnimInstance && CurrentParrySuccessMontage)
	{
		AnimInstance->Montage_Stop(BlendOutTime, CurrentParrySuccessMontage);
	}

	FinishPlayerHitState();
}

bool AJunPlayer::CanProcessPendingParrySuccessActionRequest(const FJunPendingActionRequest& Request) const
{
	return Request.Source == EJunPendingActionRequestSource::ParrySuccess &&
		CanBufferParrySuccessCancel(Request.ParrySuccessCancelAction);
}

float AJunPlayer::GetPendingParrySuccessCancelBlendOutTime(const FJunPendingActionRequest& Request) const
{
	switch (Request.ParrySuccessCancelAction)
	{
	case EJunBufferedParrySuccessCancelAction::BasicAttack:
		return ParrySuccessBasicAttackCancelBlendOutTime;
	case EJunBufferedParrySuccessCancelAction::HeavyAttack:
	case EJunBufferedParrySuccessCancelAction::Jigen:
		return ParrySuccessHeavyAttackCancelBlendOutTime;
	case EJunBufferedParrySuccessCancelAction::Defense:
		return ParrySuccessDefenseCancelBlendOutTime;
	case EJunBufferedParrySuccessCancelAction::Jump:
		return ParrySuccessJumpCancelBlendOutTime;
	case EJunBufferedParrySuccessCancelAction::Dodge:
		return ParrySuccessDodgeCancelBlendOutTime;
	case EJunBufferedParrySuccessCancelAction::Move:
		return ParrySuccessMoveCancelBlendOutTime;
	default:
		return 0.f;
	}
}

void AJunPlayer::ProcessPendingParrySuccessActionRequest(const FJunPendingActionRequest& Request)
{
	const EJunBufferedParrySuccessCancelAction PendingAction = Request.ParrySuccessCancelAction;
	const float BlendOutTime = GetPendingActionCancelBlendOutTime(Request);

	switch (PendingAction)
	{
	case EJunBufferedParrySuccessCancelAction::BasicAttack:
		CancelParrySuccessForCancelTransition(BlendOutTime);
		BasicAttack();
		break;
	case EJunBufferedParrySuccessCancelAction::HeavyAttack:
	{
		const bool bShouldChargeFromHeldInput = bParrySuccessHeavyAttackInputHeld;
		const float PreservedHeavyAttackHoldTime = HeavyAttackInputHoldTime;
		CancelParrySuccessForCancelTransition(BlendOutTime);
		if (bShouldChargeFromHeldInput)
		{
			bHeavyAttackInputHeld = true;
			HeavyAttackInputHoldTime = FMath::Max(PreservedHeavyAttackHoldTime, 0.f);
			HeavyAttackChargeLoopElapsedTime = 0.f;
			if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
			{
				StartHeavyAttackCharge();
			}
		}
		else
		{
			StartHeavyAttackTap();
		}
		bParrySuccessHeavyAttackInputHeld = false;
		break;
	}
	case EJunBufferedParrySuccessCancelAction::Jigen:
		CancelParrySuccessForCancelTransition(BlendOutTime);
		StartJigen();
		break;
	case EJunBufferedParrySuccessCancelAction::Defense:
		CancelParrySuccessForCancelTransition(BlendOutTime);
		EnterGuardLoop();
		break;
	case EJunBufferedParrySuccessCancelAction::Jump:
		CancelParrySuccessForCancelTransition(BlendOutTime);
		Jump();
		break;
	case EJunBufferedParrySuccessCancelAction::Dodge:
		CancelParrySuccessForCancelTransition(BlendOutTime);
		StartDodge();
		break;
	case EJunBufferedParrySuccessCancelAction::Move:
		CancelParrySuccessForCancelTransition(BlendOutTime);
		break;
	default:
		break;
	}
}

bool AJunPlayer::TryProcessPendingParrySuccessActionRequest(FJunPendingActionRequest& Request)
{
	return TryProcessPendingActionRequest(Request);
}

void AJunPlayer::ProcessBufferedParrySuccessCancelAction()
{
	TryProcessPendingParrySuccessActionRequest(PendingParrySuccessActionRequest);
}

void AJunPlayer::CancelDefenseForCancelTransition(float BlendOutTime)
{
	if (DefenseComponent)
	{
		DefenseComponent->CancelDefenseForCancelTransition(BlendOutTime);
	}
}

#pragma endregion
