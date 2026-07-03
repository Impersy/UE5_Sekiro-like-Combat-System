#pragma once

#include "CoreMinimal.h"
#include "Character/Monster/JunMonster.h"
#include "JunBossTypes.generated.h"

UENUM(BlueprintType)
enum class EBossCombatState : uint8
{
	Approach,
	Idle,
	Hit,
	Attack,
	ParrySuccess,
	Recovery,
	Reposition,
	NonAttackAction,
	Evade,
	Turn
};

UENUM(BlueprintType)
enum class EBossCombatStateChangeReason : uint8
{
	Unspecified,
	CombatEntered,
	PlayerDeathWait,
	PlayerDeathReset,
	PlanReady,
	PlanInvalid,
	TargetRangeChanged,
	AttackStarted,
	AttackEnded,
	HitReactStarted,
	HitReactEnded,
	ParrySucceeded,
	ParryEnded,
	RecoveryEnded,
	MobilityStarted,
	MobilityEnded,
	TurnStarted,
	TurnEnded,
	ExecutionReadyStarted,
	ExecutionReadyEnded,
	ReactiveAction
};

enum class EBossCombatStateTransitionMode : uint8
{
	EnterState,
	SynchronizeOnly
};

USTRUCT(BlueprintType)
struct FBossCombatStateTransitionRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossCombatState FromState = EBossCombatState::Approach;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossCombatState ToState = EBossCombatState::Approach;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossCombatStateChangeReason Reason = EBossCombatStateChangeReason::Unspecified;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float WorldTimeSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bEnteredState = true;
};

struct FBossNormalAttackCandidatePolicy
{
	bool bIgnoreRange = false;
	bool bIgnoreRepeat = false;
};

UENUM(BlueprintType)
enum class EBossActionType : uint8
{
	None,
	ComboAttack,
	NormalAttack,
	ParryCounter,
	Dash,
	Dodge,
	Turn
};

UENUM(BlueprintType)
enum class EBossPlannedActionType : uint8
{
	None,
	Attack,
	NonAttack
};

UENUM(BlueprintType)
enum class EBossReactiveActionType : uint8
{
	None,
	EvadeBackward
};

UENUM(BlueprintType)
enum class EBossInitiativeState : uint8
{
	Neutral,
	PlayerPressure,
	BossPattern,
	BossParryCounter
};

UENUM(BlueprintType)
enum class EBossPlannedNonAttackType : uint8
{
	None,
	Strafe,
	Dash,
	Dodge,
	Hold
};

UENUM(BlueprintType)
enum class EBossMovementDirection : uint8
{
	None,
	Forward,
	Backward,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EBossComboSet : uint8
{
	None,
	ComboA,
	ComboB
};

UENUM(BlueprintType)
enum class EBossNormalAttackType : uint8
{
	None,
	JumpAttack,
	ChargeAttack,
	DodgeAttack,
	NinjaA,
	NinjaB,
	Execution,
	Bow4Combo,
	BowBackDashAttack,
	BowHeavyAttack,
	FastComeSlash,
	SlowComeSlash,
	FakeDownSlash,
	LionSlash,
	SpinSlash,
	LightningSword
};

UENUM(BlueprintType)
enum class EBossNormalAttackLinkResult : uint8
{
	Validated,
	Started,
	Delayed,
	MovingToRange,
	TriggerFailed,
	NoCandidate,
	NoPendingRequest,
	BlockedByExecution,
	InvalidTarget,
	InvalidState,
	AttackNotAllowed,
	MissingAttackData,
	MissingAnimation,
	OutOfRange,
	MontageFailed
};

JUNDUCK_API const TCHAR* LexToString(EBossNormalAttackLinkResult Result);

USTRUCT(BlueprintType)
struct FBossNormalAttackLinkCandidate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	EBossNormalAttackType AttackType = EBossNormalAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	float BlendOutTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	float BlendInTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bRequireRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bMoveToRangeWhenOutOfRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bUseTestFilter = true;
};

struct FBossNormalAttackLinkRequest
{
	EBossNormalAttackType DefaultAttackType = EBossNormalAttackType::None;
	TArray<EBossNormalAttackType> LegacyAdditionalAttackTypes;
	TArray<FBossNormalAttackLinkCandidate> AdditionalCandidates;
	float TriggerChance = 0.5f;
	float DefaultBlendOutTime = 0.12f;
	float DefaultBlendInTime = 0.12f;
	bool bDefaultRequireRange = false;
	bool bDefaultMoveToRangeWhenOutOfRange = false;
	bool bDelayMoveToRangeWhenOutOfRange = false;
	bool bForceAttackLinkWhenTriggerChanceFails = false;
	bool bDefaultUseTestFilter = true;
	bool bExcludeCurrentAttack = true;
	bool bAvoidRecentlyUsedAttack = true;
	bool bDebugLog = false;
};

USTRUCT(BlueprintType)
struct FBossDelayedNormalAttackLinkRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackLink")
	bool bPending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackLink")
	EBossNormalAttackType AttackType = EBossNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackLink")
	float BlendOutTime = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackLink")
	float BlendInTime = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackLink")
	bool bUseTestFilter = true;

	bool IsValid() const
	{
		return bPending && AttackType != EBossNormalAttackType::None;
	}

	void Reset()
	{
		*this = FBossDelayedNormalAttackLinkRequest{};
	}
};

UENUM(BlueprintType)
enum class EBossCodeMoveStopReason : uint8
{
	None,
	Started,
	TooClose,
	MaxDistance,
	DurationEnd,
	LostTarget,
	InvalidMovement,
	Interrupted
};

UENUM(BlueprintType)
enum class EBossLightningSwordAirMoveMode : uint8
{
	LiftAndHover,
	Hover,
	Dive
};

USTRUCT(BlueprintType)
struct FBossNormalAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	TObjectPtr<class UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float MaxRange = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float CandidateMaxRange = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	bool bFaceTargetDuringAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float FacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float FacingInterpSpeed = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	bool bUseAnimNotifyTiming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float MoveStartTimeAtPlayRateOne = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float MoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float MoveStandOffDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float GroundMotionOverrideDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float AirTrackInterpSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	bool bTrackTargetDuringCodeMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float CodeMoveStopDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float CodeMoveMaxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	bool bTryTurnAfterAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack")
	float PostAttackTurnStartAngle = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|NormalAttack", meta = (ClampMin = "1", ClampMax = "10"))
	int32 SelectionWeight = 1;
};

USTRUCT(BlueprintType)
struct FBossComboAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack")
	TObjectPtr<class UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack", meta = (ClampMin = "2", ClampMax = "10"))
	int32 MaxComboLength = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ContinueChance = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack")
	bool bUseAnimNotifyTiming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack")
	TArray<float> FacingDurations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack")
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack")
	float CandidateRange = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ComboAttack", meta = (ClampMin = "1", ClampMax = "10"))
	int32 SelectionWeight = 1;
};

USTRUCT(BlueprintType)
struct FBossReactiveActionExecutionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	EBossReactiveActionType Type = EBossReactiveActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	EBossPlannedNonAttackType NonAttackType = EBossPlannedNonAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	EBossMovementDirection MovementDirection = EBossMovementDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	float Duration = 0.f;
};

UENUM(BlueprintType)
enum class EBossReactiveActionDecisionResult : uint8
{
	None,
	ResetPressure,
	AccumulatePressure,
	QueueAction
};

USTRUCT(BlueprintType)
struct FBossReactiveActionDecision
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	EBossReactiveActionDecisionResult Result = EBossReactiveActionDecisionResult::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	EBossReactiveActionType ActionType = EBossReactiveActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	int32 CloseHitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	float TriggerChance = 0.f;
};

USTRUCT(BlueprintType)
struct FBossReactiveActionState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	EBossReactiveActionType QueuedAction = EBossReactiveActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	FBossReactiveActionExecutionData ActiveAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	int32 BackwardEvadeCloseHitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	float BackwardEvadeTriggerChance = 0.f;

	void ResetQueuedAction()
	{
		QueuedAction = EBossReactiveActionType::None;
	}

	void ResetActiveAction()
	{
		ActiveAction = FBossReactiveActionExecutionData{};
	}

	void ResetPressure()
	{
		BackwardEvadeCloseHitCount = 0;
		BackwardEvadeTriggerChance = 0.f;
	}
};

USTRUCT(BlueprintType)
struct FBossReactiveActionTuningData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	float TriggerRange = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseTriggerChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChancePerHitCount = 0.066f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChanceBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxTriggerChance = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	EBossPlannedNonAttackType PrimaryNonAttackType = EBossPlannedNonAttackType::Dash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PrimaryNonAttackTypeChance = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	EBossPlannedNonAttackType SecondaryNonAttackType = EBossPlannedNonAttackType::Dodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|ReactiveAction")
	EBossMovementDirection MovementDirection = EBossMovementDirection::Backward;
};

USTRUCT(BlueprintType)
struct FBossActionRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Action")
	EBossActionType ActionType = EBossActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Action")
	EBossComboSet ComboSet = EBossComboSet::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Action")
	int32 ComboLength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Action")
	EBossNormalAttackType NormalAttackType = EBossNormalAttackType::None;
};

USTRUCT(BlueprintType)
struct FBossAttackRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackRuntime")
	EBossActionType ActionType = EBossActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackRuntime")
	EBossComboSet ComboSet = EBossComboSet::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackRuntime")
	int32 ComboLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackRuntime")
	EBossNormalAttackType NormalAttackType = EBossNormalAttackType::None;

	void Reset()
	{
		*this = FBossAttackRuntimeState{};
	}
};

struct FBossAttackExecutionRequest
{
	FMonsterAttackSelection Selection;
	FBossAttackRuntimeState RuntimeState;
	EBossInitiativeState InitiativeState = EBossInitiativeState::BossPattern;
	float BlendInTime = -1.f;
	bool bInterruptedByHitReact = false;
};

UENUM(BlueprintType)
enum class EBossSpecialReactionType : uint8
{
	None,
	PerfectParryRebound,
	MikiriCountered
};

struct FBossSpecialReactionExecutionRequest
{
	EBossSpecialReactionType Type = EBossSpecialReactionType::None;
	FMonsterAttackSelection Selection;
	EBossInitiativeState InitiativeState = EBossInitiativeState::Neutral;
	float BlendInTime = 0.f;
};

USTRUCT(BlueprintType)
struct FBossCodeMoveState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	bool bPending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	bool bActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	EBossNormalAttackType AttackType = EBossNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	float DelayRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	EBossCodeMoveStopReason LastStopReason = EBossCodeMoveStopReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	float LastTargetDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	float LastTravelDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	float LastStopDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CodeMove")
	float LastMaxDistance = 0.f;

	void ResetMotion()
	{
		bPending = false;
		bActive = false;
		AttackType = EBossNormalAttackType::None;
		DelayRemainTime = 0.f;
		StartLocation = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct FBossVariantAttackState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|VariantAttack")
	bool bActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|VariantAttack")
	EBossNormalAttackType TargetAttackType = EBossNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|VariantAttack")
	int32 CompletedDashCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|VariantAttack")
	int32 RequiredDashCount = 0;

	void Activate(EBossNormalAttackType InTargetAttackType, int32 InRequiredDashCount)
	{
		bActive = true;
		TargetAttackType = InTargetAttackType;
		CompletedDashCount = 0;
		RequiredDashCount = FMath::Max(0, InRequiredDashCount);
	}

	bool HasRemainingDash() const
	{
		return CompletedDashCount < RequiredDashCount;
	}

	void AdvanceDash()
	{
		++CompletedDashCount;
	}

	void Reset()
	{
		*this = FBossVariantAttackState{};
	}
};

struct FBossWeightedNormalAttackCandidate
{
	EBossNormalAttackType AttackType = EBossNormalAttackType::None;
	int32 Weight = 1;
};

UENUM()
enum class EBossPlannedAttackType : uint8
{
	None,
	ComboAttack,
	NormalAttack
};

USTRUCT(BlueprintType)
struct FBossCombatPlan
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	EBossPlannedActionType ActionType = EBossPlannedActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	EBossPlannedAttackType AttackType = EBossPlannedAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	EBossNormalAttackType NormalAttackType = EBossNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	EBossComboSet ComboSet = EBossComboSet::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	int32 ComboLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	EBossPlannedNonAttackType NonAttackType = EBossPlannedNonAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	EBossMovementDirection MovementDirection = EBossMovementDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	float MovementDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|CombatPlan")
	float AttackWaitTime = 0.f;

	void ResetAttackChoice()
	{
		AttackType = EBossPlannedAttackType::None;
		NormalAttackType = EBossNormalAttackType::None;
		ComboSet = EBossComboSet::None;
		ComboLength = 0;
	}

	void Reset()
	{
		*this = FBossCombatPlan{};
	}
};

USTRUCT(BlueprintType)
struct FBossParryExchangeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float NormalParryChance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float PerfectParryChance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float StrongNormalParryChance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float StrongPerfectParryChance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	int32 NormalParryCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float IdleTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	bool bPendingChanceGain = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	EHitReactType PendingHitType = EHitReactType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	bool bCounterStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	bool bCounterPerfectParried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	bool bCounterFollowUpStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	TObjectPtr<class UAnimMontage> SuccessMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float SuccessFallbackRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryExchange")
	float SuccessExitActionLockRemainTime = 0.f;

	void ClearPendingChanceGain()
	{
		bPendingChanceGain = false;
		PendingHitType = EHitReactType::None;
	}

	void ResetExchangeProgress()
	{
		NormalParryChance = 0.f;
		PerfectParryChance = 0.f;
		StrongNormalParryChance = 0.f;
		StrongPerfectParryChance = 0.f;
		NormalParryCount = 0;
		IdleTime = 0.f;
		ClearPendingChanceGain();
	}

	void ResetCounterProgress()
	{
		bCounterStarted = false;
		bCounterPerfectParried = false;
		bCounterFollowUpStarted = false;
	}
};

UENUM(BlueprintType)
enum class EBossParryDecisionResult : uint8
{
	NotParryable,
	InvalidHitType,
	InvalidTarget,
	InvalidState,
	OutsideFrontCone,
	InvalidHitDirection,
	ChanceFailed,
	NormalParry,
	PerfectParry
};

USTRUCT(BlueprintType)
struct FBossParryDecision
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryDecision")
	EBossParryDecisionResult Result = EBossParryDecisionResult::NotParryable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryDecision")
	EHitReactType HitType = EHitReactType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryDecision")
	float NormalParryChance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|ParryDecision")
	float PerfectParryChance = 0.f;

	bool IsParry() const
	{
		return Result == EBossParryDecisionResult::NormalParry ||
			Result == EBossParryDecisionResult::PerfectParry;
	}
};
