#include "Character/Player/PlayerComponent/JunPlayerActionStateComponent.h"

#include "Character/Player/JunPlayer.h"

UJunPlayerActionStateComponent::UJunPlayerActionStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BuildDefaultTransitionPolicy();
}

void UJunPlayerActionStateComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayer = Cast<AJunPlayer>(GetOwner());
}

bool UJunPlayerActionStateComponent::RequestAction(
	EJunPlayerActionState RequestedState,
	EJunActionTransitionReason Reason)
{
	const EJunActionTransitionReason ResolvedReason =
		ResolveTransitionReason(CurrentActionState, RequestedState, Reason);

	if (!CanTransitionTo(RequestedState, ResolvedReason))
	{
		LogDeniedTransition(CurrentActionState, RequestedState, ResolvedReason);
		return false;
	}

	SetActionState(RequestedState, ResolvedReason);
	return true;
}

bool UJunPlayerActionStateComponent::CanTransitionTo(
	EJunPlayerActionState RequestedState,
	EJunActionTransitionReason Reason) const
{
	const EJunActionTransitionReason ResolvedReason =
		ResolveTransitionReason(CurrentActionState, RequestedState, Reason);
	return CanTransition(CurrentActionState, RequestedState, ResolvedReason);
}

bool UJunPlayerActionStateComponent::CanTransition(
	EJunPlayerActionState FromState,
	EJunPlayerActionState ToState,
	EJunActionTransitionReason Reason) const
{
	if (FromState == ToState)
	{
		return true;
	}

	for (const FJunPlayerActionTransitionRule& Rule : TransitionPolicyRules)
	{
		if (DoesRuleMatch(Rule, FromState, ToState, Reason))
		{
			return true;
		}
	}

	return false;
}

void UJunPlayerActionStateComponent::SetActionState(
	EJunPlayerActionState NewState,
	EJunActionTransitionReason Reason)
{
	if (CurrentActionState == NewState)
	{
		LastTransitionReason = Reason;
		return;
	}

	const EJunPlayerActionState OldState = CurrentActionState;
	PreviousActionState = OldState;
	CurrentActionState = NewState;
	LastTransitionReason = Reason;
	LogTransition(OldState, NewState, Reason);
}

void UJunPlayerActionStateComponent::ClearActionState(EJunActionTransitionReason Reason)
{
	SetActionState(EJunPlayerActionState::None, Reason);
}

void UJunPlayerActionStateComponent::ClearActionStateIf(
	EJunPlayerActionState ExpectedState,
	EJunActionTransitionReason Reason)
{
	if (CurrentActionState == ExpectedState)
	{
		ClearActionState(Reason);
	}
}

void UJunPlayerActionStateComponent::SetLocomotionState(EJunPlayerLocomotionState NewState)
{
	if (CurrentLocomotionState == NewState)
	{
		return;
	}

	const EJunPlayerLocomotionState OldState = CurrentLocomotionState;
	PreviousLocomotionState = OldState;
	CurrentLocomotionState = NewState;
	LogLocomotionTransition(OldState, NewState);
}

void UJunPlayerActionStateComponent::SetMoveInputState(EJunPlayerMoveInputState NewState)
{
	if (CurrentMoveInputState == NewState)
	{
		return;
	}

	const EJunPlayerMoveInputState OldState = CurrentMoveInputState;
	PreviousMoveInputState = OldState;
	CurrentMoveInputState = NewState;
	LogMoveInputTransition(OldState, NewState);
}

void UJunPlayerActionStateComponent::BuildDefaultTransitionPolicy()
{
	TransitionPolicyRules.Reset();

	constexpr bool AnyReason = true;
	constexpr bool ExactReason = false;

	AddAnyFromTransitionRule(EJunPlayerActionState::None, EJunActionTransitionReason::System, ExactReason);
	AddAnyFromTransitionRule(EJunPlayerActionState::Death, EJunActionTransitionReason::ForcedByDeath, ExactReason);

	// Forced reactions are still gated by ReceiveHit/Execution checks in the owner. The central table only tracks that
	// those states are allowed to interrupt most gameplay actions.
	AddAnyFromTransitionRule(EJunPlayerActionState::Execution, EJunActionTransitionReason::ForcedByExecution, ExactReason);
	AddAnyFromTransitionRule(EJunPlayerActionState::HitReact, EJunActionTransitionReason::ForcedByDamage, ExactReason);
	AddAnyFromTransitionRule(EJunPlayerActionState::ParrySuccess, EJunActionTransitionReason::ForcedByDamage, ExactReason);
	AddAnyFromTransitionRule(EJunPlayerActionState::GuardBlock, EJunActionTransitionReason::ForcedByDamage, ExactReason);
	AddAnyFromTransitionRule(EJunPlayerActionState::GuardBreak, EJunActionTransitionReason::ForcedByGuardBreak, ExactReason);

	// Idle/neutral entry points.
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::JumpCounterStomp);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::Defense);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::DrinkingPotion);
	AddTransitionRule(EJunPlayerActionState::None, EJunPlayerActionState::Execution);

	// Attack cancel families. Fine timing is still owned by the existing CanCancel... functions.
	AddTransitionRule(EJunPlayerActionState::BasicAttack, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::BasicAttack, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::BasicAttack, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::BasicAttack, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::BasicAttack, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::BasicAttack, EJunPlayerActionState::Defense);

	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::HeavyAttack, EJunPlayerActionState::Defense);

	AddTransitionRule(EJunPlayerActionState::Jigen, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::Jigen, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::Jigen, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::Jigen, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::Jigen, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::Jigen, EJunPlayerActionState::Defense);

	// Air attack recovery/cancel family.
	AddTransitionRule(EJunPlayerActionState::JumpAttack, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::JumpAttack, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::JumpAttack, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::JumpAttack, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::JumpAttack, EJunPlayerActionState::Defense);

	// Jump counter follow-up can branch into a normal jump attack during its notify window.
	AddTransitionRule(EJunPlayerActionState::JumpCounterStomp, EJunPlayerActionState::JumpAttack);

	// Evade family.
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::DodgeAttack);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::Dodge, EJunPlayerActionState::Defense);

	AddTransitionRule(EJunPlayerActionState::DodgeAttack, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::DodgeAttack, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::DodgeAttack, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::DodgeAttack, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::DodgeAttack, EJunPlayerActionState::Defense);

	// Defense and hit-reaction recovery/cancel families.
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::Defense, EJunPlayerActionState::DrinkingPotion);

	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::ParrySuccess, EJunPlayerActionState::Defense);

	AddTransitionRule(EJunPlayerActionState::GuardBlock, EJunPlayerActionState::Defense);
	AddTransitionRule(EJunPlayerActionState::GuardBlock, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::GuardBlock, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::GuardBlock, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::GuardBlock, EJunPlayerActionState::Dodge);

	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::BasicAttack);
	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::HeavyAttack);
	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::Jigen);
	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::Dodge);
	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::Jump);
	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::JumpAttack);
	AddTransitionRule(EJunPlayerActionState::HitReact, EJunPlayerActionState::Defense);

	AddTransitionRule(EJunPlayerActionState::GuardBreak, EJunPlayerActionState::HitReact);
	AddTransitionRule(EJunPlayerActionState::GuardBreak, EJunPlayerActionState::Defense);

	// Potion can only finish normally or be interrupted by forced states.
	AddTransitionRule(EJunPlayerActionState::DrinkingPotion, EJunPlayerActionState::None, EJunActionTransitionReason::System, ExactReason);

	// Execution/death return paths.
	AddTransitionRule(EJunPlayerActionState::Execution, EJunPlayerActionState::None, EJunActionTransitionReason::System, ExactReason);
	AddTransitionRule(EJunPlayerActionState::Death, EJunPlayerActionState::None, EJunActionTransitionReason::System, ExactReason);

	(void)AnyReason;
}

void UJunPlayerActionStateComponent::AddTransitionRule(
	EJunPlayerActionState FromState,
	EJunPlayerActionState ToState,
	EJunActionTransitionReason Reason,
	bool bAnyReason)
{
	FJunPlayerActionTransitionRule& Rule = TransitionPolicyRules.AddDefaulted_GetRef();
	Rule.FromState = FromState;
	Rule.ToState = ToState;
	Rule.Reason = Reason;
	Rule.bAnyReason = bAnyReason;
}

void UJunPlayerActionStateComponent::AddAnyFromTransitionRule(
	EJunPlayerActionState ToState,
	EJunActionTransitionReason Reason,
	bool bAnyReason)
{
	FJunPlayerActionTransitionRule& Rule = TransitionPolicyRules.AddDefaulted_GetRef();
	Rule.bAnyFromState = true;
	Rule.ToState = ToState;
	Rule.Reason = Reason;
	Rule.bAnyReason = bAnyReason;
}

bool UJunPlayerActionStateComponent::DoesRuleMatch(
	const FJunPlayerActionTransitionRule& Rule,
	EJunPlayerActionState FromState,
	EJunPlayerActionState ToState,
	EJunActionTransitionReason Reason) const
{
	return (Rule.bAnyFromState || Rule.FromState == FromState) &&
		(Rule.bAnyToState || Rule.ToState == ToState) &&
		(Rule.bAnyReason || Rule.Reason == Reason);
}

EJunActionTransitionReason UJunPlayerActionStateComponent::ResolveTransitionReason(
	EJunPlayerActionState FromState,
	EJunPlayerActionState ToState,
	EJunActionTransitionReason RequestedReason) const
{
	if (RequestedReason != EJunActionTransitionReason::NormalInput ||
		FromState == ToState ||
		FromState == EJunPlayerActionState::None ||
		ToState == EJunPlayerActionState::None)
	{
		return RequestedReason;
	}

	if (IsAttackActionState(FromState) && IsGameplayActionState(ToState))
	{
		return EJunActionTransitionReason::CancelFromAttack;
	}

	if (FromState == EJunPlayerActionState::Dodge ||
		FromState == EJunPlayerActionState::DodgeAttack)
	{
		return EJunActionTransitionReason::CancelFromDodge;
	}

	if (FromState == EJunPlayerActionState::Defense)
	{
		return EJunActionTransitionReason::CancelFromDefense;
	}

	if (IsHitReactionActionState(FromState))
	{
		return EJunActionTransitionReason::CancelFromHitReact;
	}

	return RequestedReason;
}

bool UJunPlayerActionStateComponent::IsAttackActionState(EJunPlayerActionState State) const
{
	return State == EJunPlayerActionState::BasicAttack ||
		State == EJunPlayerActionState::HeavyAttack ||
		State == EJunPlayerActionState::Jigen ||
		State == EJunPlayerActionState::Jump ||
		State == EJunPlayerActionState::JumpAttack;
}

bool UJunPlayerActionStateComponent::IsHitReactionActionState(EJunPlayerActionState State) const
{
	return State == EJunPlayerActionState::ParrySuccess ||
		State == EJunPlayerActionState::GuardBlock ||
		State == EJunPlayerActionState::GuardBreak ||
		State == EJunPlayerActionState::HitReact;
}

bool UJunPlayerActionStateComponent::IsGameplayActionState(EJunPlayerActionState State) const
{
	return State == EJunPlayerActionState::BasicAttack ||
		State == EJunPlayerActionState::HeavyAttack ||
		State == EJunPlayerActionState::Jigen ||
		State == EJunPlayerActionState::Dodge ||
		State == EJunPlayerActionState::DodgeAttack ||
		State == EJunPlayerActionState::Jump ||
		State == EJunPlayerActionState::JumpAttack ||
		State == EJunPlayerActionState::Defense;
}

const TCHAR* UJunPlayerActionStateComponent::LexToString(EJunPlayerActionState State) const
{
	switch (State)
	{
	case EJunPlayerActionState::None: return TEXT("None");
	case EJunPlayerActionState::BasicAttack: return TEXT("BasicAttack");
	case EJunPlayerActionState::HeavyAttack: return TEXT("HeavyAttack");
	case EJunPlayerActionState::Jigen: return TEXT("Jigen");
	case EJunPlayerActionState::Dodge: return TEXT("Dodge");
	case EJunPlayerActionState::DodgeAttack: return TEXT("DodgeAttack");
	case EJunPlayerActionState::Jump: return TEXT("Jump");
	case EJunPlayerActionState::JumpAttack: return TEXT("JumpAttack");
	case EJunPlayerActionState::JumpCounterStomp: return TEXT("JumpCounterStomp");
	case EJunPlayerActionState::Defense: return TEXT("Defense");
	case EJunPlayerActionState::ParrySuccess: return TEXT("ParrySuccess");
	case EJunPlayerActionState::GuardBlock: return TEXT("GuardBlock");
	case EJunPlayerActionState::GuardBreak: return TEXT("GuardBreak");
	case EJunPlayerActionState::HitReact: return TEXT("HitReact");
	case EJunPlayerActionState::Execution: return TEXT("Execution");
	case EJunPlayerActionState::DrinkingPotion: return TEXT("DrinkingPotion");
	case EJunPlayerActionState::Death: return TEXT("Death");
	default: return TEXT("Unknown");
	}
}

const TCHAR* UJunPlayerActionStateComponent::LexToString(EJunPlayerLocomotionState State) const
{
	switch (State)
	{
	case EJunPlayerLocomotionState::Grounded: return TEXT("Grounded");
	case EJunPlayerLocomotionState::Airborne: return TEXT("Airborne");
	case EJunPlayerLocomotionState::Landing: return TEXT("Landing");
	case EJunPlayerLocomotionState::Disabled: return TEXT("Disabled");
	default: return TEXT("Unknown");
	}
}

const TCHAR* UJunPlayerActionStateComponent::LexToString(EJunPlayerMoveInputState State) const
{
	switch (State)
	{
	case EJunPlayerMoveInputState::None: return TEXT("None");
	case EJunPlayerMoveInputState::Requested: return TEXT("Requested");
	case EJunPlayerMoveInputState::Active: return TEXT("Active");
	case EJunPlayerMoveInputState::Blocked: return TEXT("Blocked");
	case EJunPlayerMoveInputState::Disabled: return TEXT("Disabled");
	default: return TEXT("Unknown");
	}
}

const TCHAR* UJunPlayerActionStateComponent::LexToString(EJunActionTransitionReason Reason) const
{
	switch (Reason)
	{
	case EJunActionTransitionReason::None: return TEXT("None");
	case EJunActionTransitionReason::NormalInput: return TEXT("NormalInput");
	case EJunActionTransitionReason::Combo: return TEXT("Combo");
	case EJunActionTransitionReason::CancelFromAttack: return TEXT("CancelFromAttack");
	case EJunActionTransitionReason::CancelFromDodge: return TEXT("CancelFromDodge");
	case EJunActionTransitionReason::CancelFromHitReact: return TEXT("CancelFromHitReact");
	case EJunActionTransitionReason::CancelFromDefense: return TEXT("CancelFromDefense");
	case EJunActionTransitionReason::ForcedByDamage: return TEXT("ForcedByDamage");
	case EJunActionTransitionReason::ForcedByGuardBreak: return TEXT("ForcedByGuardBreak");
	case EJunActionTransitionReason::ForcedByExecution: return TEXT("ForcedByExecution");
	case EJunActionTransitionReason::ForcedByDeath: return TEXT("ForcedByDeath");
	case EJunActionTransitionReason::System: return TEXT("System");
	default: return TEXT("Unknown");
	}
}

void UJunPlayerActionStateComponent::LogTransition(
	EJunPlayerActionState From,
	EJunPlayerActionState To,
	EJunActionTransitionReason Reason) const
{
	if (!bLogActionStateTransitions)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerActionState | %s -> %s | Reason=%s"),
		LexToString(From),
		LexToString(To),
		LexToString(Reason));
}

void UJunPlayerActionStateComponent::LogLocomotionTransition(
	EJunPlayerLocomotionState From,
	EJunPlayerLocomotionState To) const
{
	if (!bLogLocomotionStateTransitions)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerLocomotionState | %s -> %s"),
		LexToString(From),
		LexToString(To));
}

void UJunPlayerActionStateComponent::LogMoveInputTransition(
	EJunPlayerMoveInputState From,
	EJunPlayerMoveInputState To) const
{
	if (!bLogMoveInputStateTransitions)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerMoveInputState | %s -> %s"),
		LexToString(From),
		LexToString(To));
}

void UJunPlayerActionStateComponent::LogDeniedTransition(
	EJunPlayerActionState From,
	EJunPlayerActionState To,
	EJunActionTransitionReason Reason) const
{
	if (!bLogDeniedActionStateTransitions)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PlayerActionState denied | %s -> %s | Reason=%s"),
		LexToString(From),
		LexToString(To),
		LexToString(Reason));
}
