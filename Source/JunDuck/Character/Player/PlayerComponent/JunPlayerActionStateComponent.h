#pragma once

#include "CoreMinimal.h"
#include "Character/Player/JunPlayerActionTypes.h"
#include "Components/ActorComponent.h"
#include "JunPlayerActionStateComponent.generated.h"

USTRUCT(BlueprintType)
struct FJunPlayerActionTransitionRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State")
	bool bAnyFromState = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State", meta = (EditCondition = "!bAnyFromState"))
	EJunPlayerActionState FromState = EJunPlayerActionState::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State")
	bool bAnyToState = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State", meta = (EditCondition = "!bAnyToState"))
	EJunPlayerActionState ToState = EJunPlayerActionState::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State")
	bool bAnyReason = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State", meta = (EditCondition = "!bAnyReason"))
	EJunActionTransitionReason Reason = EJunActionTransitionReason::None;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JUNDUCK_API UJunPlayerActionStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunPlayerActionStateComponent();

	virtual void BeginPlay() override;

	bool RequestAction(
		EJunPlayerActionState RequestedState,
		EJunActionTransitionReason Reason = EJunActionTransitionReason::NormalInput);
	bool CanTransitionTo(
		EJunPlayerActionState RequestedState,
		EJunActionTransitionReason Reason = EJunActionTransitionReason::NormalInput) const;
	bool CanTransition(
		EJunPlayerActionState FromState,
		EJunPlayerActionState ToState,
		EJunActionTransitionReason Reason) const;
	void SetActionState(
		EJunPlayerActionState NewState,
		EJunActionTransitionReason Reason = EJunActionTransitionReason::System);
	void ClearActionState(EJunActionTransitionReason Reason = EJunActionTransitionReason::System);
	void ClearActionStateIf(
		EJunPlayerActionState ExpectedState,
		EJunActionTransitionReason Reason = EJunActionTransitionReason::System);
	void SetLocomotionState(EJunPlayerLocomotionState NewState);
	void SetMoveInputState(EJunPlayerMoveInputState NewState);

	EJunPlayerActionState GetCurrentActionState() const { return CurrentActionState; }
	EJunPlayerActionState GetPreviousActionState() const { return PreviousActionState; }
	EJunPlayerLocomotionState GetCurrentLocomotionState() const { return CurrentLocomotionState; }
	EJunPlayerLocomotionState GetPreviousLocomotionState() const { return PreviousLocomotionState; }
	EJunPlayerMoveInputState GetCurrentMoveInputState() const { return CurrentMoveInputState; }
	EJunPlayerMoveInputState GetPreviousMoveInputState() const { return PreviousMoveInputState; }
	EJunActionTransitionReason GetLastTransitionReason() const { return LastTransitionReason; }
	bool IsInActionState(EJunPlayerActionState State) const { return CurrentActionState == State; }
	bool IsInLocomotionState(EJunPlayerLocomotionState State) const { return CurrentLocomotionState == State; }
	bool IsInMoveInputState(EJunPlayerMoveInputState State) const { return CurrentMoveInputState == State; }

private:
	void BuildDefaultTransitionPolicy();
	void AddTransitionRule(
		EJunPlayerActionState FromState,
		EJunPlayerActionState ToState,
		EJunActionTransitionReason Reason = EJunActionTransitionReason::None,
		bool bAnyReason = true);
	void AddAnyFromTransitionRule(
		EJunPlayerActionState ToState,
		EJunActionTransitionReason Reason = EJunActionTransitionReason::None,
		bool bAnyReason = true);
	bool DoesRuleMatch(
		const FJunPlayerActionTransitionRule& Rule,
		EJunPlayerActionState FromState,
		EJunPlayerActionState ToState,
		EJunActionTransitionReason Reason) const;
	EJunActionTransitionReason ResolveTransitionReason(
		EJunPlayerActionState FromState,
		EJunPlayerActionState ToState,
		EJunActionTransitionReason RequestedReason) const;
	bool IsAttackActionState(EJunPlayerActionState State) const;
	bool IsHitReactionActionState(EJunPlayerActionState State) const;
	bool IsGameplayActionState(EJunPlayerActionState State) const;
	const TCHAR* LexToString(EJunPlayerActionState State) const;
	const TCHAR* LexToString(EJunPlayerLocomotionState State) const;
	const TCHAR* LexToString(EJunPlayerMoveInputState State) const;
	const TCHAR* LexToString(EJunActionTransitionReason Reason) const;
	void LogTransition(EJunPlayerActionState From, EJunPlayerActionState To, EJunActionTransitionReason Reason) const;
	void LogLocomotionTransition(EJunPlayerLocomotionState From, EJunPlayerLocomotionState To) const;
	void LogMoveInputTransition(EJunPlayerMoveInputState From, EJunPlayerMoveInputState To) const;
	void LogDeniedTransition(EJunPlayerActionState From, EJunPlayerActionState To, EJunActionTransitionReason Reason) const;

	UPROPERTY()
	TObjectPtr<class AJunPlayer> OwnerPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action State", meta = (AllowPrivateAccess = "true"))
	EJunPlayerActionState CurrentActionState = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action State", meta = (AllowPrivateAccess = "true"))
	EJunPlayerActionState PreviousActionState = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion State", meta = (AllowPrivateAccess = "true"))
	EJunPlayerLocomotionState CurrentLocomotionState = EJunPlayerLocomotionState::Grounded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion State", meta = (AllowPrivateAccess = "true"))
	EJunPlayerLocomotionState PreviousLocomotionState = EJunPlayerLocomotionState::Grounded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Move Input State", meta = (AllowPrivateAccess = "true"))
	EJunPlayerMoveInputState CurrentMoveInputState = EJunPlayerMoveInputState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Move Input State", meta = (AllowPrivateAccess = "true"))
	EJunPlayerMoveInputState PreviousMoveInputState = EJunPlayerMoveInputState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action State", meta = (AllowPrivateAccess = "true"))
	EJunActionTransitionReason LastTransitionReason = EJunActionTransitionReason::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State|Policy", meta = (AllowPrivateAccess = "true"))
	TArray<FJunPlayerActionTransitionRule> TransitionPolicyRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogActionStateTransitions = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogLocomotionStateTransitions = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogMoveInputStateTransitions = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action State|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogDeniedActionStateTransitions = true;
};
