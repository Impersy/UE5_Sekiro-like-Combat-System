#pragma once

#include "CoreMinimal.h"
#include "JunPlayerActionTypes.generated.h"

UENUM(BlueprintType)
enum class EJunPlayerActionState : uint8
{
	None,
	BasicAttack,
	HeavyAttack,
	Jigen,
	Dodge,
	DodgeAttack,
	Jump,
	JumpAttack,
	JumpCounterStomp,
	Defense,
	ParrySuccess,
	GuardBlock,
	GuardBreak,
	HitReact,
	Execution,
	DrinkingPotion,
	Death
};

UENUM(BlueprintType)
enum class EJunPlayerLocomotionState : uint8
{
	Grounded,
	Airborne,
	Landing,
	Disabled
};

UENUM(BlueprintType)
enum class EJunPlayerMoveInputState : uint8
{
	None,
	Requested,
	Active,
	Blocked,
	Disabled
};

UENUM(BlueprintType)
enum class EJunActionTransitionReason : uint8
{
	None,
	NormalInput,
	Combo,
	CancelFromAttack,
	CancelFromDodge,
	CancelFromHitReact,
	CancelFromDefense,
	ForcedByDamage,
	ForcedByGuardBreak,
	ForcedByExecution,
	ForcedByDeath,
	System
};

UENUM(BlueprintType)
enum class EJunBufferedRecoveryAction : uint8
{
	None,
	Dodge,
	Jump,
	Defense
};

UENUM(BlueprintType)
enum class EJunBufferedDefenseCancelAction : uint8
{
	None,
	BasicAttack,
	Jump,
	Dodge,
	Move,
	HeavyAttack
};

UENUM(BlueprintType)
enum class EJunBufferedParrySuccessCancelAction : uint8
{
	None,
	BasicAttack,
	HeavyAttack,
	Jigen,
	Defense,
	Jump,
	Dodge,
	Move
};

UENUM(BlueprintType)
enum class EJunPendingActionRequestSource : uint8
{
	None,
	BasicAttackRecovery,
	HeavyAttackRecovery,
	JigenRecovery,
	DefenseTransition,
	ParrySuccess
};

USTRUCT(BlueprintType)
struct FJunPendingActionRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunPendingActionRequestSource Source = EJunPendingActionRequestSource::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunPlayerActionState ActionState = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunActionTransitionReason Reason = EJunActionTransitionReason::NormalInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunBufferedRecoveryAction RecoveryAction = EJunBufferedRecoveryAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunBufferedDefenseCancelAction DefenseCancelAction = EJunBufferedDefenseCancelAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunBufferedParrySuccessCancelAction ParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;

	bool IsSet() const
	{
		return Source != EJunPendingActionRequestSource::None;
	}

	bool IsMoveRequest() const
	{
		return ActionState == EJunPlayerActionState::None && IsSet();
	}

	void Reset()
	{
		Source = EJunPendingActionRequestSource::None;
		ActionState = EJunPlayerActionState::None;
		Reason = EJunActionTransitionReason::NormalInput;
		RecoveryAction = EJunBufferedRecoveryAction::None;
		DefenseCancelAction = EJunBufferedDefenseCancelAction::None;
		ParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	}
};

USTRUCT(BlueprintType)
struct FJunPlayerCancelRule
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunPlayerActionState FromAction = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunPlayerActionState ToAction = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunActionTransitionReason Reason = EJunActionTransitionReason::NormalInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float OpenTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BlendOutTime = 0.f;

	bool IsValid() const
	{
		return FromAction != EJunPlayerActionState::None;
	}
};

USTRUCT(BlueprintType)
struct FJunPlayerActionStartRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunPlayerActionState ActionState = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunActionTransitionReason Reason = EJunActionTransitionReason::NormalInput;

	bool IsSet() const
	{
		return ActionState != EJunPlayerActionState::None;
	}
};

USTRUCT(BlueprintType)
struct FJunPlayerForcedActionRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunPlayerActionState ActionState = EJunPlayerActionState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EJunActionTransitionReason Reason = EJunActionTransitionReason::None;

	bool IsSet() const
	{
		return ActionState != EJunPlayerActionState::None &&
			Reason != EJunActionTransitionReason::None;
	}
};
