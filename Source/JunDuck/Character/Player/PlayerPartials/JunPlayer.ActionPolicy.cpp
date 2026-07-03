#include "Character/Player/JunPlayer.h"

#include "Character/Player/PlayerComponent/JunPlayerActionStateComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerPotionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#pragma region Action Request / Cancel Policy
// ============================================================================
// Action Request / Cancel Policy
// ============================================================================

bool AJunPlayer::CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction Action) const
{
	const FJunPendingActionRequest Request = MakeRecoveryPendingActionRequest(
		EJunPendingActionRequestSource::BasicAttackRecovery,
		Action);
	FJunPlayerCancelRule CancelRule;
	return TryGetRecoveryCancelRule(Request, CancelRule) &&
		IsPendingActionCancelRuleOpen(Request, CancelRule) &&
		CanRequestResolvedBufferedAction(CancelRule.ToAction);
}

bool AJunPlayer::CanCancelBasicAttackIntoHeavyAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelBasicAttackIntoHeavyAttack()
{
	if (!CanCancelBasicAttackIntoHeavyAttack())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule);
	CancelBasicAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelBasicAttackIntoJigen() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::Jigen,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelBasicAttackIntoJigen()
{
	if (!CanCancelBasicAttackIntoJigen())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::Jigen,
		CancelRule);
	CancelBasicAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanBufferDefenseTransitionCancel() const
{
	const EJunDefenseState CurrentDefenseState = GetDefenseState();
	const float PostBufferRemainTime = DefenseComponent
		? DefenseComponent->GetPostDefenseTransitionCancelBufferRemainTime()
		: 0.f;
	return CurrentDefenseState == EJunDefenseState::Starting ||
		CurrentDefenseState == EJunDefenseState::Ending ||
		PostBufferRemainTime > 0.f;
}

bool AJunPlayer::CanCancelDefenseTransitionIntoMove() const
{
	return GetDefenseState() == EJunDefenseState::Ending &&
		(DefenseComponent
			? DefenseComponent->GetDefenseTransitionElapsedTime()
			: 0.f) >= DefenseMoveCancelOpenTime;
}

EJunPlayerActionState AJunPlayer::ResolveBufferedRecoveryActionState(EJunBufferedRecoveryAction Action) const
{
	switch (Action)
	{
	case EJunBufferedRecoveryAction::Dodge:
		return EJunPlayerActionState::Dodge;
	case EJunBufferedRecoveryAction::Jump:
		return EJunPlayerActionState::Jump;
	case EJunBufferedRecoveryAction::Defense:
		return EJunPlayerActionState::Defense;
	default:
		return EJunPlayerActionState::None;
	}
}

EJunPlayerActionState AJunPlayer::ResolveBufferedDefenseCancelActionState(EJunBufferedDefenseCancelAction Action) const
{
	switch (Action)
	{
	case EJunBufferedDefenseCancelAction::BasicAttack:
		return EJunPlayerActionState::BasicAttack;
	case EJunBufferedDefenseCancelAction::HeavyAttack:
		return EJunPlayerActionState::HeavyAttack;
	case EJunBufferedDefenseCancelAction::Jump:
		return EJunPlayerActionState::Jump;
	case EJunBufferedDefenseCancelAction::Dodge:
		return EJunPlayerActionState::Dodge;
	case EJunBufferedDefenseCancelAction::Move:
	default:
		return EJunPlayerActionState::None;
	}
}

EJunPlayerActionState AJunPlayer::ResolveBufferedParrySuccessCancelActionState(
	EJunBufferedParrySuccessCancelAction Action) const
{
	switch (Action)
	{
	case EJunBufferedParrySuccessCancelAction::BasicAttack:
		return EJunPlayerActionState::BasicAttack;
	case EJunBufferedParrySuccessCancelAction::HeavyAttack:
		return EJunPlayerActionState::HeavyAttack;
	case EJunBufferedParrySuccessCancelAction::Jigen:
		return EJunPlayerActionState::Jigen;
	case EJunBufferedParrySuccessCancelAction::Defense:
		return EJunPlayerActionState::Defense;
	case EJunBufferedParrySuccessCancelAction::Jump:
		return EJunPlayerActionState::Jump;
	case EJunBufferedParrySuccessCancelAction::Dodge:
		return EJunPlayerActionState::Dodge;
	case EJunBufferedParrySuccessCancelAction::Move:
	default:
		return EJunPlayerActionState::None;
	}
}

bool AJunPlayer::CanRequestResolvedBufferedAction(EJunPlayerActionState ActionState) const
{
	return ActionState != EJunPlayerActionState::None && CanRequestActionState(ActionState);
}

FJunPendingActionRequest AJunPlayer::MakeRecoveryPendingActionRequest(
	EJunPendingActionRequestSource Source,
	EJunBufferedRecoveryAction Action) const
{
	FJunPendingActionRequest Request;
	if (Action == EJunBufferedRecoveryAction::None)
	{
		return Request;
	}

	Request.Source = Source;
	Request.RecoveryAction = Action;
	Request.ActionState = ResolveBufferedRecoveryActionState(Action);
	return Request;
}

FJunPendingActionRequest AJunPlayer::MakeDefensePendingActionRequest(EJunBufferedDefenseCancelAction Action) const
{
	FJunPendingActionRequest Request;
	if (Action == EJunBufferedDefenseCancelAction::None)
	{
		return Request;
	}

	Request.Source = EJunPendingActionRequestSource::DefenseTransition;
	Request.DefenseCancelAction = Action;
	Request.ActionState = ResolveBufferedDefenseCancelActionState(Action);
	return Request;
}

FJunPendingActionRequest AJunPlayer::MakeParrySuccessPendingActionRequest(
	EJunBufferedParrySuccessCancelAction Action) const
{
	FJunPendingActionRequest Request;
	if (Action == EJunBufferedParrySuccessCancelAction::None)
	{
		return Request;
	}

	Request.Source = EJunPendingActionRequestSource::ParrySuccess;
	Request.ParrySuccessCancelAction = Action;
	Request.ActionState = ResolveBufferedParrySuccessCancelActionState(Action);
	return Request;
}

void AJunPlayer::QueuePendingActionRequest(const FJunPendingActionRequest& Request)
{
	if (!Request.IsSet())
	{
		return;
	}

	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
		PendingBasicAttackRecoveryActionRequest = Request;
		TryProcessPendingActionRequest(PendingBasicAttackRecoveryActionRequest);
		break;
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
		PendingHeavyAttackRecoveryActionRequest = Request;
		TryProcessPendingActionRequest(PendingHeavyAttackRecoveryActionRequest);
		break;
	case EJunPendingActionRequestSource::JigenRecovery:
		PendingJigenRecoveryActionRequest = Request;
		TryProcessPendingActionRequest(PendingJigenRecoveryActionRequest);
		break;
	case EJunPendingActionRequestSource::DefenseTransition:
		if (!DefenseComponent)
		{
			break;
		}
		DefenseComponent->SetBufferedDefenseTransitionCancelAction(Request.DefenseCancelAction);
		TryProcessBufferedDefenseTransitionCancelAction();
		break;
	case EJunPendingActionRequestSource::ParrySuccess:
		PendingParrySuccessActionRequest = Request;
		TryProcessPendingActionRequest(PendingParrySuccessActionRequest);
		break;
	default:
		break;
	}
}

bool AJunPlayer::TryProcessPendingActionRequest(FJunPendingActionRequest& Request)
{
	if (ShouldDiscardPendingActionRequest(Request))
	{
		ClearDiscardedPendingActionRequest(Request);
		Request.Reset();
		return false;
	}

	if (!CanProcessPendingActionRequest(Request))
	{
		return false;
	}

	FJunPendingActionRequest PendingRequest;
	if (!ConsumePendingActionRequest(Request, PendingRequest))
	{
		return false;
	}

	if (ShouldCancelCurrentActionForPendingActionRequest(PendingRequest))
	{
		CancelCurrentActionForPendingActionRequest(
			PendingRequest,
			GetPendingActionCancelBlendOutTime(PendingRequest));
	}

	ProcessPendingActionRequest(PendingRequest);
	return true;
}

bool AJunPlayer::ShouldDiscardPendingActionRequest(const FJunPendingActionRequest& Request) const
{
	if (!Request.IsSet())
	{
		return false;
	}

	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
	case EJunPendingActionRequestSource::ParrySuccess:
		return Is_Dead() || IsInDeathSequence();
	default:
		return false;
	}
}

void AJunPlayer::ClearDiscardedPendingActionRequest(const FJunPendingActionRequest& Request)
{
	if (Request.Source == EJunPendingActionRequestSource::ParrySuccess)
	{
		bParrySuccessHeavyAttackInputHeld = false;
	}
}

bool AJunPlayer::CanProcessPendingActionRequest(const FJunPendingActionRequest& Request) const
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
		return CanProcessPendingRecoveryActionRequest(Request);
	case EJunPendingActionRequestSource::DefenseTransition:
		return CanProcessPendingDefenseTransitionActionRequest(Request);
	case EJunPendingActionRequestSource::ParrySuccess:
		return CanProcessPendingParrySuccessActionRequest(Request);
	default:
		return false;
	}
}

float AJunPlayer::GetPendingActionCancelBlendOutTime(const FJunPendingActionRequest& Request) const
{
	FJunPlayerCancelRule CancelRule;
	if (TryGetPendingActionCancelRule(Request, CancelRule))
	{
		return CancelRule.BlendOutTime;
	}

	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
		return GetPendingRecoveryCancelBlendOutTime(Request);
	case EJunPendingActionRequestSource::DefenseTransition:
		return GetPendingDefenseTransitionCancelBlendOutTime(Request);
	case EJunPendingActionRequestSource::ParrySuccess:
		return GetPendingParrySuccessCancelBlendOutTime(Request);
	default:
		return 0.f;
	}
}

bool AJunPlayer::TryGetPendingActionCancelRule(
	const FJunPendingActionRequest& Request,
	FJunPlayerCancelRule& OutRule) const
{
	OutRule = FJunPlayerCancelRule();

	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
		return TryGetRecoveryCancelRule(Request, OutRule);
	default:
		return false;
	}
}

bool AJunPlayer::TryGetRecoveryCancelRule(
	const FJunPendingActionRequest& Request,
	FJunPlayerCancelRule& OutRule) const
{
	OutRule = FJunPlayerCancelRule();

	const EJunPlayerActionState ToAction = ResolveBufferedRecoveryActionState(Request.RecoveryAction);
	if (ToAction == EJunPlayerActionState::None)
	{
		return false;
	}

	OutRule.ToAction = ToAction;
	OutRule.Reason = Request.Reason;

	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	{
		OutRule.FromAction = EJunPlayerActionState::BasicAttack;

		const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
		switch (Request.RecoveryAction)
		{
		case EJunBufferedRecoveryAction::Dodge:
			OutRule.OpenTime = BasicAttackDodgeCancelOpenTimes.IsValidIndex(ComboArrayIndex)
				? BasicAttackDodgeCancelOpenTimes[ComboArrayIndex]
				: 0.18f;
			OutRule.BlendOutTime = BasicAttackDodgeCancelBlendOutTime;
			return true;
		case EJunBufferedRecoveryAction::Jump:
			OutRule.OpenTime = BasicAttackJumpCancelOpenTimes.IsValidIndex(ComboArrayIndex)
				? BasicAttackJumpCancelOpenTimes[ComboArrayIndex]
				: (BasicAttackDodgeCancelOpenTimes.IsValidIndex(ComboArrayIndex)
					? BasicAttackDodgeCancelOpenTimes[ComboArrayIndex]
					: 0.18f);
			OutRule.BlendOutTime = BasicAttackJumpCancelBlendOutTime;
			return true;
		case EJunBufferedRecoveryAction::Defense:
			OutRule.OpenTime = BasicAttackDefenseCancelOpenTimes.IsValidIndex(ComboArrayIndex)
				? BasicAttackDefenseCancelOpenTimes[ComboArrayIndex]
				: (BasicAttackJumpCancelOpenTimes.IsValidIndex(ComboArrayIndex)
					? BasicAttackJumpCancelOpenTimes[ComboArrayIndex]
					: (BasicAttackDodgeCancelOpenTimes.IsValidIndex(ComboArrayIndex)
						? BasicAttackDodgeCancelOpenTimes[ComboArrayIndex]
						: 0.18f));
			OutRule.BlendOutTime = BasicAttackDefenseCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	}
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	{
		if (HeavyAttackState != EJunHeavyAttackState::Tap &&
			HeavyAttackState != EJunHeavyAttackState::ChargeEnd)
		{
			return false;
		}

		OutRule.FromAction = EJunPlayerActionState::HeavyAttack;
		switch (Request.RecoveryAction)
		{
		case EJunBufferedRecoveryAction::Dodge:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapDodgeCancelOpenTime
				: HeavyAttackChargeEndDodgeCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackDodgeCancelBlendOutTime;
			return true;
		case EJunBufferedRecoveryAction::Jump:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapJumpCancelOpenTime
				: HeavyAttackChargeEndJumpCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackJumpCancelBlendOutTime;
			return true;
		case EJunBufferedRecoveryAction::Defense:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapDefenseCancelOpenTime
				: HeavyAttackChargeEndDefenseCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackDefenseCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	}
	case EJunPendingActionRequestSource::JigenRecovery:
	{
		OutRule.FromAction = EJunPlayerActionState::Jigen;
		switch (Request.RecoveryAction)
		{
		case EJunBufferedRecoveryAction::Dodge:
			OutRule.OpenTime = JigenDodgeCancelOpenTime;
			OutRule.BlendOutTime = JigenDodgeCancelBlendOutTime;
			return true;
		case EJunBufferedRecoveryAction::Jump:
			OutRule.OpenTime = JigenJumpCancelOpenTime;
			OutRule.BlendOutTime = JigenJumpCancelBlendOutTime;
			return true;
		case EJunBufferedRecoveryAction::Defense:
			OutRule.OpenTime = JigenDefenseCancelOpenTime;
			OutRule.BlendOutTime = JigenDefenseCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

bool AJunPlayer::IsPendingActionCancelRuleOpen(
	const FJunPendingActionRequest& Request,
	const FJunPlayerCancelRule& Rule) const
{
	if (!Rule.IsValid())
	{
		return false;
	}

	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
		if (!bIsBasicAttacking)
		{
			switch (Request.RecoveryAction)
			{
			case EJunBufferedRecoveryAction::Dodge:
			case EJunBufferedRecoveryAction::Jump:
				return PostBasicAttackJumpDodgeBufferRemainTime > 0.f;
			case EJunBufferedRecoveryAction::Defense:
				return PostBasicAttackDefenseBufferRemainTime > 0.f;
			default:
				return false;
			}
		}
		return BasicAttackSectionElapsedTime >= Rule.OpenTime;
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
		return bIsHeavyAttacking && HeavyAttackSectionElapsedTime >= Rule.OpenTime;
	case EJunPendingActionRequestSource::JigenRecovery:
		return bIsJigenAttacking && JigenSectionElapsedTime >= Rule.OpenTime;
	default:
		return false;
	}
}

bool AJunPlayer::TryGetDirectActionCancelRule(
	EJunPlayerActionState FromAction,
	EJunPlayerActionState ToAction,
	FJunPlayerCancelRule& OutRule) const
{
	OutRule = FJunPlayerCancelRule();
	OutRule.FromAction = FromAction;
	OutRule.ToAction = ToAction;
	OutRule.Reason = EJunActionTransitionReason::NormalInput;

	switch (FromAction)
	{
	case EJunPlayerActionState::BasicAttack:
	{
		const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
		switch (ToAction)
		{
		case EJunPlayerActionState::None:
			OutRule.OpenTime = BasicAttackMoveCancelOpenTimes.IsValidIndex(ComboArrayIndex)
				? BasicAttackMoveCancelOpenTimes[ComboArrayIndex]
				: 0.18f;
			OutRule.BlendOutTime = BasicAttackMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::HeavyAttack:
			OutRule.OpenTime = BasicAttackHeavyAttackCancelOpenTimes.IsValidIndex(ComboArrayIndex)
				? BasicAttackHeavyAttackCancelOpenTimes[ComboArrayIndex]
				: 0.18f;
			OutRule.BlendOutTime = BasicAttackHeavyAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Jigen:
			OutRule.OpenTime = BasicAttackJigenCancelOpenTimes.IsValidIndex(ComboArrayIndex)
				? BasicAttackJigenCancelOpenTimes[ComboArrayIndex]
				: 0.f;
			OutRule.BlendOutTime = BasicAttackJigenCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	}
	case EJunPlayerActionState::HeavyAttack:
	{
		if (HeavyAttackState != EJunHeavyAttackState::Tap &&
			HeavyAttackState != EJunHeavyAttackState::ChargeEnd)
		{
			return false;
		}

		switch (ToAction)
		{
		case EJunPlayerActionState::None:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapMoveCancelOpenTime
				: HeavyAttackChargeEndMoveCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::BasicAttack:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapBasicAttackCancelOpenTime
				: HeavyAttackChargeEndBasicAttackCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackBasicAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::HeavyAttack:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapBasicAttackCancelOpenTime
				: HeavyAttackChargeEndBasicAttackCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackHeavyAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Jigen:
			OutRule.OpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
				? HeavyAttackTapJigenCancelOpenTime
				: HeavyAttackChargeEndJigenCancelOpenTime;
			OutRule.BlendOutTime = HeavyAttackJigenCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	}
	case EJunPlayerActionState::Jigen:
		switch (ToAction)
		{
		case EJunPlayerActionState::None:
			OutRule.OpenTime = JigenMoveCancelOpenTime;
			OutRule.BlendOutTime = JigenMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::BasicAttack:
			OutRule.OpenTime = JigenBasicAttackCancelOpenTime;
			OutRule.BlendOutTime = JigenBasicAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::HeavyAttack:
			OutRule.OpenTime = JigenHeavyAttackCancelOpenTime;
			OutRule.BlendOutTime = JigenHeavyAttackCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	case EJunPlayerActionState::JumpAttack:
		if (JumpAttackState != EJunJumpAttackState::End)
		{
			return false;
		}

		switch (ToAction)
		{
		case EJunPlayerActionState::None:
			OutRule.OpenTime = JumpAttackEndMoveCancelOpenTime;
			OutRule.BlendOutTime = JumpAttackEndMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Dodge:
			OutRule.OpenTime = JumpAttackEndDodgeCancelOpenTime;
			OutRule.BlendOutTime = JumpAttackEndDodgeCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Defense:
			OutRule.OpenTime = JumpAttackEndDefenseCancelOpenTime;
			OutRule.BlendOutTime = JumpAttackEndDefenseCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::BasicAttack:
			OutRule.OpenTime = JumpAttackEndBasicAttackCancelOpenTime;
			OutRule.BlendOutTime = JumpAttackEndBasicAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::HeavyAttack:
			OutRule.OpenTime = JumpAttackEndHeavyAttackCancelOpenTime;
			OutRule.BlendOutTime = JumpAttackEndHeavyAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Jigen:
			OutRule.OpenTime = JumpAttackEndJigenCancelOpenTime;
			OutRule.BlendOutTime = JumpAttackEndJigenCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	case EJunPlayerActionState::DodgeAttack:
		switch (ToAction)
		{
		case EJunPlayerActionState::None:
			OutRule.OpenTime = DodgeAttackMoveCancelOpenTime;
			OutRule.BlendOutTime = DodgeAttackMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Dodge:
			OutRule.OpenTime = DodgeAttackDodgeCancelOpenTime;
			OutRule.BlendOutTime = DodgeAttackDodgeCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Defense:
			OutRule.OpenTime = DodgeAttackDefenseCancelOpenTime;
			OutRule.BlendOutTime = DodgeAttackDefenseCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::BasicAttack:
			OutRule.OpenTime = DodgeAttackBasicAttackCancelOpenTime;
			OutRule.BlendOutTime = DodgeAttackBasicAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::HeavyAttack:
			OutRule.OpenTime = DodgeAttackHeavyAttackCancelOpenTime;
			OutRule.BlendOutTime = DodgeAttackHeavyAttackCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Jigen:
			OutRule.OpenTime = DodgeAttackJigenCancelOpenTime;
			OutRule.BlendOutTime = DodgeAttackJigenCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	case EJunPlayerActionState::Dodge:
		if (!HasGameplayTag(JunGameplayTags::State_Action_Dodge) || !CurrentDodgeMontage)
		{
			return false;
		}

		switch (ToAction)
		{
		case EJunPlayerActionState::None:
			OutRule.OpenTime = GetDodgeMoveCancelOpenTimeForMontage(CurrentDodgeMontage);
			OutRule.BlendOutTime = DashMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::BasicAttack:
		case EJunPlayerActionState::HeavyAttack:
		case EJunPlayerActionState::Jump:
		case EJunPlayerActionState::Defense:
			OutRule.OpenTime = GetDodgeRecoveryCancelOpenTimeForMontage(CurrentDodgeMontage);
			OutRule.BlendOutTime = DodgeRecoveryCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Jigen:
			OutRule.OpenTime = GetDodgeRecoveryCancelOpenTimeForMontage(CurrentDodgeMontage);
			OutRule.BlendOutTime = DodgeJigenCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	case EJunPlayerActionState::HitReact:
		switch (ToAction)
		{
		case EJunPlayerActionState::None:
		case EJunPlayerActionState::Jump:
			OutRule.OpenTime = 0.f;
			OutRule.BlendOutTime = HitReactMoveCancelBlendOutTime;
			return true;
		case EJunPlayerActionState::Defense:
			OutRule.OpenTime = 0.f;
			OutRule.BlendOutTime = HitReactDefenseCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	case EJunPlayerActionState::JumpCounterStomp:
		switch (ToAction)
		{
		case EJunPlayerActionState::JumpAttack:
			OutRule.OpenTime = 0.f;
			OutRule.BlendOutTime = JumpCounterStompJumpAttackCancelBlendOutTime;
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}

bool AJunPlayer::IsDirectActionCancelRuleOpen(const FJunPlayerCancelRule& Rule) const
{
	if (!Rule.IsValid())
	{
		return false;
	}

	switch (Rule.FromAction)
	{
	case EJunPlayerActionState::BasicAttack:
		return bIsBasicAttacking && BasicAttackSectionElapsedTime >= Rule.OpenTime;
	case EJunPlayerActionState::HeavyAttack:
		return bIsHeavyAttacking && HeavyAttackSectionElapsedTime >= Rule.OpenTime;
	case EJunPlayerActionState::Jigen:
		return bIsJigenAttacking && JigenSectionElapsedTime >= Rule.OpenTime;
	case EJunPlayerActionState::JumpAttack:
		return bIsJumpAttacking &&
			JumpAttackState == EJunJumpAttackState::End &&
			JumpAttackSectionElapsedTime >= Rule.OpenTime;
	case EJunPlayerActionState::DodgeAttack:
		return bIsDodgeAttacking && DodgeAttackElapsedTime >= Rule.OpenTime;
	case EJunPlayerActionState::Dodge:
		return HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
			CurrentDodgeMontage != nullptr &&
			DodgeElapsedTime >= Rule.OpenTime;
	case EJunPlayerActionState::HitReact:
		return CurrentHitState == EJunPlayerHitState::HitReact &&
			!HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	case EJunPlayerActionState::JumpCounterStomp:
		return bJumpCounterStompFollowUpActive &&
			bJumpCounterStompJumpAttackWindowOpen;
	default:
		return false;
	}
}

FJunPlayerActionStartRequest AJunPlayer::MakeActionStartRequest(
	EJunPlayerActionState ActionState,
	EJunActionTransitionReason Reason) const
{
	FJunPlayerActionStartRequest Request;
	Request.ActionState = ActionState;
	Request.Reason = Reason;
	return Request;
}

bool AJunPlayer::CanBeginActionStartRequest(const FJunPlayerActionStartRequest& Request) const
{
	return Request.IsSet() && CanRequestActionState(Request.ActionState, Request.Reason);
}

bool AJunPlayer::TryBeginActionStartRequest(const FJunPlayerActionStartRequest& Request)
{
	return Request.IsSet() && RequestActionState(Request.ActionState, Request.Reason);
}

bool AJunPlayer::TryProcessActionStartRequest(
	const FJunPlayerActionStartRequest& Request,
	TFunctionRef<void()> ProcessAction)
{
	if (!TryBeginActionStartRequest(Request))
	{
		return false;
	}

	ProcessAction();
	return true;
}

bool AJunPlayer::TryProcessActionStartRequestWithResult(
	const FJunPlayerActionStartRequest& Request,
	TFunctionRef<bool()> ProcessAction)
{
	if (!TryBeginActionStartRequest(Request))
	{
		return false;
	}

	if (!ProcessAction())
	{
		ClearActionStateIf(Request.ActionState, EJunActionTransitionReason::System);
		return false;
	}

	return true;
}

FJunPlayerForcedActionRequest AJunPlayer::MakeForcedActionRequest(
	EJunPlayerActionState ActionState,
	EJunActionTransitionReason Reason) const
{
	FJunPlayerForcedActionRequest Request;
	Request.ActionState = ActionState;
	Request.Reason = Reason;
	return Request;
}

bool AJunPlayer::CanApplyForcedActionRequest(const FJunPlayerForcedActionRequest& Request) const
{
	return Request.IsSet() && CanRequestActionState(Request.ActionState, Request.Reason);
}

bool AJunPlayer::TryApplyForcedActionRequest(const FJunPlayerForcedActionRequest& Request)
{
	return Request.IsSet() && RequestActionState(Request.ActionState, Request.Reason);
}

bool AJunPlayer::ConsumePendingActionRequest(
	FJunPendingActionRequest& Request,
	FJunPendingActionRequest& OutPendingRequest)
{
	if (!Request.IsSet())
	{
		OutPendingRequest.Reset();
		return false;
	}

	OutPendingRequest = Request;
	Request.Reset();
	ClearConsumedPendingActionRequest(OutPendingRequest);
	return true;
}

void AJunPlayer::ClearConsumedPendingActionRequest(const FJunPendingActionRequest& Request)
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
		PostBasicAttackJumpDodgeBufferRemainTime = 0.f;
		PostBasicAttackDefenseBufferRemainTime = 0.f;
		break;
	case EJunPendingActionRequestSource::DefenseTransition:
		if (DefenseComponent)
		{
			DefenseComponent->ConsumeBufferedDefenseTransitionRequest();
		}
		break;
	default:
		break;
	}
}

bool AJunPlayer::ShouldCancelCurrentActionForPendingActionRequest(
	const FJunPendingActionRequest& Request) const
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
		return ShouldCancelCurrentActionForPendingRecoveryRequest(Request);
	default:
		return false;
	}
}

void AJunPlayer::CancelCurrentActionForPendingActionRequest(
	const FJunPendingActionRequest& Request,
	float BlendOutTime)
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
		CancelCurrentActionForPendingRecoveryRequest(Request, BlendOutTime);
		break;
	default:
		break;
	}
}

void AJunPlayer::ProcessPendingActionRequest(const FJunPendingActionRequest& Request)
{
	switch (Request.Source)
	{
	case EJunPendingActionRequestSource::BasicAttackRecovery:
	case EJunPendingActionRequestSource::HeavyAttackRecovery:
	case EJunPendingActionRequestSource::JigenRecovery:
		ProcessPendingRecoveryActionRequest(Request);
		break;
	case EJunPendingActionRequestSource::DefenseTransition:
		ProcessPendingDefenseTransitionActionRequest(Request);
		break;
	case EJunPendingActionRequestSource::ParrySuccess:
		ProcessPendingParrySuccessActionRequest(Request);
		break;
	default:
		break;
	}
}

bool AJunPlayer::CanBufferParrySuccessCancel(EJunBufferedParrySuccessCancelAction Action) const
{
	if (CurrentHitState != EJunPlayerHitState::ParrySuccess)
	{
		return false;
	}

	const EJunPlayerActionState ActionState = ResolveBufferedParrySuccessCancelActionState(Action);
	switch (Action)
	{
	case EJunBufferedParrySuccessCancelAction::BasicAttack:
		return ParrySuccessElapsedTime >= ParrySuccessBasicAttackCancelOpenTime &&
			CanRequestResolvedBufferedAction(ActionState);
	case EJunBufferedParrySuccessCancelAction::HeavyAttack:
		return ParrySuccessElapsedTime >= ParrySuccessHeavyAttackCancelOpenTime &&
			CanRequestResolvedBufferedAction(ActionState);
	case EJunBufferedParrySuccessCancelAction::Jigen:
		return ParrySuccessElapsedTime >= ParrySuccessHeavyAttackCancelOpenTime &&
			CanRequestResolvedBufferedAction(ActionState);
	case EJunBufferedParrySuccessCancelAction::Defense:
		return ParrySuccessElapsedTime >= ParrySuccessDefenseCancelOpenTime &&
			CanRequestResolvedBufferedAction(ActionState);
	case EJunBufferedParrySuccessCancelAction::Jump:
		return ParrySuccessElapsedTime >= ParrySuccessJumpCancelOpenTime &&
			CanRequestResolvedBufferedAction(ActionState);
	case EJunBufferedParrySuccessCancelAction::Dodge:
		return ParrySuccessElapsedTime >= ParrySuccessDodgeCancelOpenTime &&
			CanRequestResolvedBufferedAction(ActionState);
	case EJunBufferedParrySuccessCancelAction::Move:
		return ParrySuccessElapsedTime >= ParrySuccessMoveCancelOpenTime;
	default:
		return false;
	}
}

bool AJunPlayer::ShouldUseGuardBase() const
{
	if (DefenseComponent && DefenseComponent->IsAirParrySequenceActive())
	{
		return false;
	}

	const EJunDefenseState CurrentDefenseState = GetDefenseState();
	const bool bKeepGuardBaseDuringEnd = DefenseComponent
		? DefenseComponent->ShouldKeepGuardBaseWhileEnding()
		: false;

	return CurrentDefenseState == EJunDefenseState::Starting ||
		CurrentDefenseState == EJunDefenseState::Guarding ||
		(CurrentDefenseState == EJunDefenseState::Ending && bKeepGuardBaseDuringEnd) ||
		CurrentHitState == EJunPlayerHitState::GuardBlock ||
		bGuardBlockReleasePending;
}

EJunDefenseState AJunPlayer::GetDefenseState() const
{
	return DefenseComponent ? DefenseComponent->GetDefenseState() : EJunDefenseState::None;
}

int32 AJunPlayer::GetGuardStartRestartSerial() const
{
	return DefenseComponent ? DefenseComponent->GetGuardStartRestartSerial() : 0;
}

bool AJunPlayer::IsParryWindowOpen() const
{
	return DefenseComponent && DefenseComponent->IsParryWindowOpen();
}

float AJunPlayer::GetDesiredMoveForward() const
{
	return DesiredMoveForward;
}

float AJunPlayer::GetDesiredMoveRight() const
{
	return DesiredMoveRight;
}

float AJunPlayer::GetGuardMoveBlendRemainTime() const
{
	return DefenseComponent ? DefenseComponent->GetGuardMoveBlendRemainTime() : 0.f;
}

bool AJunPlayer::ShouldDeferGuardMoveInput() const
{
	return bDeferGuardMoveInput;
}

float AJunPlayer::GetDesiredMaxWalkSpeed() const
{
	switch (GetMoveState())
	{
	case EJunMoveState::Walk:
		return WalkMoveSpeed;
	case EJunMoveState::Guard:
		return GuardMoveSpeed;
	case EJunMoveState::Sprint:
		return SprintMoveSpeed;
	case EJunMoveState::Run:
	default:
		return GetCurrentRunMoveSpeed();
	}
}

bool AJunPlayer::ShouldUseRunningAnim() const
{
	if (bWalkRequested)
	{
		return false;
	}

	return GetMoveState() == EJunMoveState::Run;
}

int32 AJunPlayer::GetCurrentDrinkPotionCharges() const
{
	return PotionComponent ? PotionComponent->GetCurrentCharges() : 0;
}

void AJunPlayer::BeginTutorialControlLock()
{
	bTutorialControlLocked = true;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::EndTutorialControlLock()
{
	if (!bTutorialControlLocked)
	{
		return;
	}

	bTutorialControlLocked = false;
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::ToggleWalkingState()
{
	SetWalkRequested(!bWalkRequested);
}

void AJunPlayer::SetWalkRequested(bool bNewWalkRequested)
{
	bWalkRequested = bNewWalkRequested;
	TargetMaxWalkSpeed = GetDesiredMaxWalkSpeed();
	ApplyRunningStateToAnimInstance(ShouldUseRunningAnim());
}

void AJunPlayer::SetDesiredMoveAxes(float NewForward, float NewRight)
{
	PendingMoveForward = FMath::Clamp(NewForward, -1.f, 1.f);
	PendingMoveRight = FMath::Clamp(NewRight, -1.f, 1.f);
	const bool bHasMoveInput =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);

	if (bHasMoveInput && IsLockOnTurnPlaying())
	{
		CancelLockOnTurn();
	}

	if (bIsJumpAttacking && JumpAttackState == EJunJumpAttackState::End)
	{
		if (bHasMoveInput && !TryCancelJumpAttackEndIntoMove())
		{
			UpdateMoveInputState();
			return;
		}
	}

	if (bIsDodgeAttacking)
	{
		if (bHasMoveInput && !TryCancelDodgeAttackIntoMove())
		{
			UpdateMoveInputState();
			return;
		}
	}

	if (bHasMoveInput &&
		bIsHeavyAttacking &&
		TryCancelHeavyAttackIntoMove())
	{
		DesiredMoveForward = PendingMoveForward;
		DesiredMoveRight = PendingMoveRight;
		UpdateMoveInputState();
		return;
	}

	if (bHasMoveInput &&
		bIsJigenAttacking &&
		TryCancelJigenIntoMove())
	{
		DesiredMoveForward = PendingMoveForward;
		DesiredMoveRight = PendingMoveRight;
		UpdateMoveInputState();
		return;
	}

	if (bDeferGuardMoveInput)
	{
		UpdateMoveInputState();
		return;
	}

	if (bHasMoveInput && TryCancelHitReactIntoMove())
	{
		DesiredMoveForward = PendingMoveForward;
		DesiredMoveRight = PendingMoveRight;
		UpdateMoveInputState();
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Block_Move))
	{
		DesiredMoveForward = 0.f;
		DesiredMoveRight = 0.f;
		UpdateMoveInputState();
		return;
	}

	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;
	UpdateMoveInputState();
}

void AJunPlayer::BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction Action)
{
	if (Is_Dead() || IsInDeathSequence())
	{
		PendingBasicAttackRecoveryActionRequest.Reset();
		return;
	}

	if (!CanCancelBasicAttackIntoRecoveryAction(Action))
	{
		return;
	}

	QueuePendingActionRequest(MakeRecoveryPendingActionRequest(
		EJunPendingActionRequestSource::BasicAttackRecovery,
		Action));
}

void AJunPlayer::BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction Action)
{
	if (Is_Dead() || IsInDeathSequence())
	{
		if (DefenseComponent)
		{
			DefenseComponent->SetBufferedDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::None);
		}
		return;
	}

	const bool bCanBufferFromParryState =
		GetDefenseState() == EJunDefenseState::Starting ||
		GetDefenseState() == EJunDefenseState::Ending ||
		(DefenseComponent &&
			DefenseComponent->GetPostDefenseTransitionCancelBufferRemainTime() > 0.f);

	if (!bCanBufferFromParryState)
	{
		return;
	}

	QueuePendingActionRequest(MakeDefensePendingActionRequest(Action));
}

void AJunPlayer::BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction Action)
{
	if (Is_Dead() || IsInDeathSequence())
	{
		PendingParrySuccessActionRequest.Reset();
		bParrySuccessHeavyAttackInputHeld = false;
		return;
	}

	if (CurrentHitState != EJunPlayerHitState::ParrySuccess)
	{
		return;
	}

	QueuePendingActionRequest(MakeParrySuccessPendingActionRequest(Action));
	if (Action == EJunBufferedParrySuccessCancelAction::HeavyAttack)
	{
		bParrySuccessHeavyAttackInputHeld = true;
	}
}

void AJunPlayer::ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->ReceiveHit(
			HitType,
			DamageAmount,
			DamageCauser,
			SwingDirection,
			DefenseKnockbackData,
			DefenseRuleData,
			bCanBuildAttackerPostureOnParry);
		return;
	}

	if (Is_Dead() || IsExecuting() || IsInDeathSequence())
	{
		return;
	}

	LastIncomingSwingDirection = SwingDirection;
	LastIncomingHitReactType = HitType;
	LastIncomingKnockbackDirection = DetermineKnockbackDirectionFromDamageCauser(DamageCauser);
	LastIncomingDefenseKnockbackData = DefenseKnockbackData;
	LastIncomingDefenseRuleData = DefenseRuleData;
	HitReactFacingTarget = DamageCauser;

	if (!TryHandleSpecialDefense(DamageCauser, DefenseRuleData) &&
		ResolveIncomingHitResult(HitType, DamageCauser) == EJunPlayerHitResolveResult::HitReact)
	{
		HandleDamageHitReact(HitType, DamageAmount, DamageCauser, SwingDirection, DefenseRuleData);
	}
}

bool AJunPlayer::RequestActionState(
	EJunPlayerActionState RequestedState,
	EJunActionTransitionReason Reason)
{
	return ActionStateComponent
		? ActionStateComponent->RequestAction(RequestedState, Reason)
		: true;
}

bool AJunPlayer::CanRequestActionState(
	EJunPlayerActionState RequestedState,
	EJunActionTransitionReason Reason) const
{
	return ActionStateComponent
		? ActionStateComponent->CanTransitionTo(RequestedState, Reason)
		: true;
}

void AJunPlayer::SetActionState(
	EJunPlayerActionState NewState,
	EJunActionTransitionReason Reason)
{
	if (ActionStateComponent)
	{
		ActionStateComponent->RequestAction(NewState, Reason);
	}
}

void AJunPlayer::ClearActionState(EJunActionTransitionReason Reason)
{
	if (ActionStateComponent)
	{
		ActionStateComponent->ClearActionState(Reason);
	}
}

void AJunPlayer::ClearActionStateIf(
	EJunPlayerActionState ExpectedState,
	EJunActionTransitionReason Reason)
{
	if (ActionStateComponent)
	{
		ActionStateComponent->ClearActionStateIf(ExpectedState, Reason);
	}
}

EJunPlayerActionState AJunPlayer::GetActionState() const
{
	return ActionStateComponent
		? ActionStateComponent->GetCurrentActionState()
		: EJunPlayerActionState::None;
}

void AJunPlayer::SetLocomotionState(EJunPlayerLocomotionState NewState)
{
	if (ActionStateComponent)
	{
		ActionStateComponent->SetLocomotionState(NewState);
	}
}

EJunPlayerLocomotionState AJunPlayer::GetLocomotionState() const
{
	return ActionStateComponent
		? ActionStateComponent->GetCurrentLocomotionState()
		: EJunPlayerLocomotionState::Grounded;
}

void AJunPlayer::SetMoveInputState(EJunPlayerMoveInputState NewState)
{
	if (ActionStateComponent)
	{
		ActionStateComponent->SetMoveInputState(NewState);
	}
}

EJunPlayerMoveInputState AJunPlayer::GetMoveInputState() const
{
	return ActionStateComponent
		? ActionStateComponent->GetCurrentMoveInputState()
		: EJunPlayerMoveInputState::None;
}

void AJunPlayer::UpdateLocomotionState()
{
	EJunPlayerLocomotionState NewState = EJunPlayerLocomotionState::Grounded;
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (DeathState != EJunPlayerDeathState::Alive ||
		IsExecuting() ||
		bTutorialControlLocked)
	{
		NewState = EJunPlayerLocomotionState::Disabled;
	}
	else if (MovementComponent && MovementComponent->IsFalling())
	{
		NewState = EJunPlayerLocomotionState::Airborne;
	}
	else if (LocomotionLandingRemainTime > 0.f)
	{
		NewState = EJunPlayerLocomotionState::Landing;
	}

	SetLocomotionState(NewState);
}

void AJunPlayer::UpdateMoveInputState()
{
	const bool bHasPendingMoveInput =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);
	const bool bHasDesiredMoveInput =
		!FMath::IsNearlyZero(DesiredMoveForward) ||
		!FMath::IsNearlyZero(DesiredMoveRight);

	if (DeathState != EJunPlayerDeathState::Alive ||
		IsExecuting() ||
		bTutorialControlLocked)
	{
		SetMoveInputState(EJunPlayerMoveInputState::Disabled);
		return;
	}

	if (bHasPendingMoveInput &&
		(bDeferGuardMoveInput || HasGameplayTag(JunGameplayTags::State_Block_Move)))
	{
		SetMoveInputState(EJunPlayerMoveInputState::Blocked);
		return;
	}

	if (bHasDesiredMoveInput)
	{
		SetMoveInputState(EJunPlayerMoveInputState::Active);
		return;
	}

	if (bHasPendingMoveInput)
	{
		SetMoveInputState(EJunPlayerMoveInputState::Requested);
		return;
	}

	SetMoveInputState(EJunPlayerMoveInputState::None);
}

#pragma endregion
