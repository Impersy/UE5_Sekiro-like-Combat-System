#pragma once

#include "CoreMinimal.h"
#include "Character/JunMonster.h"
#include "WukongBoss.generated.h"

UENUM(BlueprintType)
enum class EWukongCombatState : uint8
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
enum class EWukongActionType : uint8
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
enum class EWukongPlannedActionType : uint8
{
	None,
	Attack,
	NonAttack
};

UENUM(BlueprintType)
enum class EWukongReactiveActionType : uint8
{
	None,
	EvadeBackward
};

UENUM(BlueprintType)
enum class EWukongInitiativeState : uint8
{
	Neutral,
	PlayerPressure,
	BossPattern,
	BossParryCounter
};

UENUM(BlueprintType)
enum class EWukongPlannedNonAttackType : uint8
{
	None,
	Strafe,
	Dash,
	Dodge,
	Hold
};

UENUM(BlueprintType)
enum class EWukongMovementDirection : uint8
{
	None,
	Forward,
	Backward,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EWukongComboSet : uint8
{
	None,
	ComboA,
	ComboB
};

UENUM(BlueprintType)
enum class EWukongNormalAttackType : uint8
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
	SpinSlash
};

UENUM(BlueprintType)
enum class EWukongCodeMoveStopReason : uint8
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

USTRUCT(BlueprintType)
struct FWukongNormalAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	TObjectPtr<class UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float MaxRange = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float CandidateMaxRange = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	bool bFaceTargetDuringAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float FacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float FacingInterpSpeed = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	bool bUseAnimNotifyTiming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float MoveStartTimeAtPlayRateOne = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float MoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float MoveStandOffDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float GroundMotionOverrideDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float AirTrackInterpSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	bool bTrackTargetDuringCodeMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float CodeMoveStopDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float CodeMoveMaxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	bool bTryTurnAfterAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack")
	float PostAttackTurnStartAngle = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|NormalAttack", meta = (ClampMin = "1", ClampMax = "10"))
	int32 SelectionWeight = 1;
};

USTRUCT(BlueprintType)
struct FWukongComboAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack")
	TObjectPtr<class UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack", meta = (ClampMin = "2", ClampMax = "10"))
	int32 MaxComboLength = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ContinueChance = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack")
	bool bUseAnimNotifyTiming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack")
	TArray<float> FacingDurations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack")
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack")
	float CandidateRange = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ComboAttack", meta = (ClampMin = "1", ClampMax = "10"))
	int32 SelectionWeight = 1;
};

USTRUCT(BlueprintType)
struct FWukongReactiveActionExecutionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	EWukongReactiveActionType Type = EWukongReactiveActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	EWukongPlannedNonAttackType NonAttackType = EWukongPlannedNonAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	EWukongMovementDirection MovementDirection = EWukongMovementDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	float Duration = 0.f;
};

USTRUCT(BlueprintType)
struct FWukongReactiveActionTuningData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	float TriggerRange = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseTriggerChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChancePerHitCount = 0.066f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChanceBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxTriggerChance = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	EWukongPlannedNonAttackType PrimaryNonAttackType = EWukongPlannedNonAttackType::Dash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PrimaryNonAttackTypeChance = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	EWukongPlannedNonAttackType SecondaryNonAttackType = EWukongPlannedNonAttackType::Dodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|ReactiveAction")
	EWukongMovementDirection MovementDirection = EWukongMovementDirection::Backward;
};

USTRUCT(BlueprintType)
struct FWukongActionRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|Action")
	EWukongActionType ActionType = EWukongActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|Action")
	EWukongComboSet ComboSet = EWukongComboSet::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|Action")
	int32 ComboLength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|Action")
	EWukongNormalAttackType NormalAttackType = EWukongNormalAttackType::None;
};

UENUM()
enum class EWukongPlannedAttackType : uint8
{
	None,
	ComboAttack,
	NormalAttack
};

UCLASS()
class JUNDUCK_API AWukongBoss : public AJunMonster
{
	GENERATED_BODY()

public:
	AWukongBoss();
	void BeginCodeDrivenAttackMoveWindow(EWukongNormalAttackType AttackType);
	void EndCodeDrivenAttackMoveWindow();
	void BeginAttackSuperArmorWindow();
	void EndAttackSuperArmorWindow();
	void HandleComboDecisionPoint();
	bool TryStartNormalAttackLinkFromNotify(
		EWukongNormalAttackType NextAttackType,
		float TriggerChance,
		float BlendOutTime,
		float BlendInTime,
		bool bRequireRange,
		bool bMoveToRangeWhenOutOfRange,
		bool bDelayMoveToRangeWhenOutOfRange,
		bool bUseTestFilter);
	bool TryForceNormalAttackLinkWhenTriggerChanceFails(float BlendOutTime, float BlendInTime);
	bool TryExecuteDelayedNormalAttackRangeLinkFromNotify(float BlendOutTimeOverride = -1.f);
	EWukongNormalAttackType GetCurrentNormalAttackType() const { return CurrentAttackNormalAttackType; }
	bool WasNormalAttackRecentlyUsed(EWukongNormalAttackType AttackType, int32 Depth = 1) const;
	void HandleHitTurnDecisionPoint();
	virtual void NotifyAttackParriedBy(class AJunPlayer* Parrier, float PostureScale = 1.f) override;

protected:
	virtual void HandleGameplayEventNotify(FGameplayTag EventTag) override;
	virtual void EnterCombatState() override;
	virtual void EnterPlayerDeathWait() override;
	virtual void ResumeAfterPlayerFakeDeath() override;
	virtual void ResetAfterPlayerRealDeath() override;
	virtual void UpdateCombat(float DeltaTime) override;
	virtual FMonsterAttackSelection ChooseNextAttackSelection() const override;
	virtual void OnAttackTick(float DeltaTime) override;
	virtual FVector GetLockOnTargetPoint() const override;
	virtual float GetHitReactDuration(EHitReactType HitType) const override;
	virtual float GetHitReactControlLockDuration(EHitReactType HitType) const override;
	virtual FString GetMonsterDebugExtraText() const override;
	virtual void OnHitReactStarted(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection) override;
	virtual bool ShouldStartHitReact(EHitReactType IncomingHitReact) const override;
	virtual void OnHitReactEnded(EHitReactType EndedHitReact) override;
	virtual bool TryHandleIncomingHitBeforeDamage(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		const FJunAttackDefenseRuleData& DefenseRuleData) override;

	void SetWukongCombatState(EWukongCombatState NewState);
	void EnterApproachState();
	void EnterIdleState();
	void EnterHitState();
	void EnterAttackState();
	void EnterParrySuccessState();
	void EnterRecoveryState();
	void EnterRepositionState();
	void EnterNonAttackActionState();
	void EnterEvadeState();
	void EnterTurnState();

	void UpdateApproachState(float DeltaTime);
	void UpdateIdleState(float DeltaTime);
	void UpdateHitState(float DeltaTime);
	void UpdateAttackState(float DeltaTime);
	void UpdateParrySuccessState(float DeltaTime);
	void UpdateRecoveryState(float DeltaTime);
	void UpdateRepositionState(float DeltaTime);
	void UpdateNonAttackActionState(float DeltaTime);
	void UpdateEvadeState(float DeltaTime);
	void UpdateTurnState(float DeltaTime);

	bool CanStartWukongTurn() const;
	bool TryStartOrWaitForWukongTurn(float YawThreshold);
	bool TryStartHitReactTurnTransition(bool bStopHitReactMontage);
	bool ShouldTurnAfterHitReact(ECharacterHitReactDirection HitDirection) const;
	bool HasAnyAttackCandidateForCurrentDistance() const;
	bool HasValidPlannedAttack() const;
	bool HasValidPlannedMovement() const;
	bool IsMobilityNonAttackType(EWukongPlannedNonAttackType NonAttackType) const;
	bool IsComboSetEnabledByTestFilter(EWukongComboSet ComboSet) const;
	bool IsNormalAttackEnabledByTestFilter(EWukongNormalAttackType NormalAttackType) const;
	float GetMaxEnabledComboCandidateRange() const;
	float GetMaxEnabledAttackCandidateRange() const;
	bool IsFartherThanAllAttackCandidates() const;
	bool HasAnyNonBowAttackCandidateForCurrentDistance() const;
	EWukongMovementDirection ChooseStrafeDirectionAvoidingImmediateReverse() const;
	bool IsOppositeStrafeDirection(EWukongMovementDirection A, EWukongMovementDirection B) const;
	void CollectNormalAttackCandidates(float TargetDistance, bool bIgnoreRepeat, TArray<EWukongNormalAttackType>& OutCandidates) const;
	int32 GetComboAttackSelectionWeight() const;
	bool HasAnyComboMontage() const;
	const FWukongComboAttackData* GetComboAttackData(EWukongComboSet ComboSet) const;
	bool IsComboAttackUsable(const FWukongComboAttackData& ComboAttackData) const;
	int32 GetComboAttackMaxLength(const FWukongComboAttackData& ComboAttackData) const;
	FName GetComboSectionName(int32 ComboLength) const;
	void ConfigurePlannedComboMontageSections();
	bool ShouldContinueComboAttack(const FWukongComboAttackData& ComboAttackData, int32 CurrentComboLength) const;
	const FWukongReactiveActionTuningData* GetReactiveActionTuningData(EWukongReactiveActionType ReactiveActionType) const;
	bool IsWithinSelectionRange(const FMonsterAttackSelection& Selection) const;
	const FWukongNormalAttackData* GetNormalAttackData(EWukongNormalAttackType AttackType) const;
	UAnimMontage* GetPlannedComboMontage() const;
	const FWukongNormalAttackData* GetPlannedNormalAttackData() const;
	float GetPlannedComboFacingDuration() const;
	UAnimMontage* GetMobilityNonAttackMontage(EWukongPlannedNonAttackType NonAttackType, EWukongMovementDirection MovementDirection) const;
	UAnimMontage* GetPlannedNonAttackMontage() const;
	UAnimMontage* GetReactiveActionMontage() const;
	EWukongCombatState GetReactiveActionTargetState(EWukongReactiveActionType ReactiveActionType) const;
	float GetTargetYawAbsDelta() const;
	float GetTargetDistance2D() const;
	float GetPlannedAttackMinRange() const;
	float GetPlannedAttackMaxRange() const;
	float GetAttackCandidateMaxRange(EWukongPlannedAttackType AttackType) const;
	bool IsTargetTooFarForPlannedAttack() const;
	bool IsTargetTooCloseForPlannedAttack() const;
	bool CanTriggerCloseRangeBackDash() const;
	void StartCloseRangeBackDashCooldown();
	void UpdateFacingTowardsTargetWithoutTurn(float DeltaTime, float InterpSpeed);
	void BeginNoAttackCandidateApproach();
	void ClearNoAttackCandidateApproach();
	void ResetReactiveActionState();
	void ResetActiveReactiveActionState();
	void ResetReactiveEvadePressure();
	void ResetPendingAttackMotionState();
	virtual void ResetCurrentAttackRuntimeState() override;
	void ClearDelayedNormalAttackRangeLink();
	bool IsBowNormalAttackType(EWukongNormalAttackType AttackType) const;
	void BeginBowAttackPresentation();
	void EndBowAttackPresentation();
	void ApplyTimedMontageSuperArmor(float MontageDuration, float DurationScale = -1.f);
	void UpdateTimedMontageSuperArmor(float DeltaTime);
	void ClearTimedMontageSuperArmor();
	bool IsPlayerPressureActive() const;
	void StartPlayerPressure();
	void ClearPlayerPressure();
	void UpdatePlayerPressure(float DeltaTime);
	void UpdatePlannedCombatPlanAge(float DeltaTime);
	bool IsPlannedAttackExpired() const;
	bool ShouldForceCloseAfterBowAttack() const;
	bool ShouldSuppressBowAttackCandidate() const;
	void ResetPlannedCombatPlan();
	void EnsureCombatPlan();
	bool TryEnterReactiveStateFromIdle();
	bool TryEnterPlannedStateFromIdle();
	bool TryEnterPlannedAttackStateFromIdle();
	bool TryEnterPlannedNonAttackStateFromIdle();
	bool HasQueuedReactiveAction() const;
	bool CanStartMobilityMontageSafely() const;
	bool CanParryIncomingHit(EHitReactType HitType, AActor* DamageCauser, const FVector& SwingDirection) const;
	bool IsParryableHitType(EHitReactType HitType) const;
	float GetPerfectParryChanceForHitType(EHitReactType HitType) const;
	float GetNormalParryChanceForHitType(EHitReactType HitType) const;
	UAnimMontage* GetParrySuccessMontage(const FVector& SwingDirection);
	UAnimMontage* GetParryCounterMontage() const;
	UAnimMontage* GetParryCounterFollowUpMontage() const;
	class UNiagaraComponent* FindNiagaraComponentByName(FName ComponentName) const;
	void PlayParryParticle();
	void StartParrySuccessAgainstIncomingHit(
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData);
	bool TryStartParryCounterDecision();
	bool TryStartParryCounterAttack();
	bool TryStartParryBackJump();
	bool TryStartParryCounterFollowUp();

	UFUNCTION()
	void OnParrySuccessMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void FinishParrySuccessState();
	void RefreshPlannedCombatPlan();
	void RefreshPlannedAttackType();
	void RefreshPlannedNonAttackType();
	void RefreshNoAttackFallbackMovementType();
	bool TryQueueReactiveAction(EWukongReactiveActionType ReactiveActionType);
	bool TryQueueReactiveBackwardEvade();
	bool TryReactToHitDirection(ECharacterHitReactDirection HitDirection);
	bool TryAccumulateReactiveBackwardEvadePressure(ECharacterHitReactDirection HitDirection);
	bool TryActivateReactiveAction(EWukongReactiveActionType ReactiveActionType);
	bool TryConsumeReactiveAction();
	bool TryPlanReactiveBackwardEvade();
	void QueueCodeDrivenAttackMove(EWukongNormalAttackType AttackType, float Delay);
	void StartCodeDrivenAttackMove(EWukongNormalAttackType AttackType);
	void UpdateCodeDrivenAttackMove(float DeltaTime);
	void StopCodeDrivenAttackMove(bool bClearVelocity = true, EWukongCodeMoveStopReason StopReason = EWukongCodeMoveStopReason::Interrupted);
	void CacheCodeDrivenAttackMoveDebug(EWukongCodeMoveStopReason StopReason);
	void ApplyAttackGroundMotionOverride(float Duration);
	void RestoreAttackGroundMotionOverride();
	void RecordAction(EWukongActionType ActionType, EWukongComboSet ComboSet = EWukongComboSet::None, int32 ComboLength = 0, EWukongNormalAttackType NormalAttackType = EWukongNormalAttackType::None);
	bool WasRecentlyUsed(EWukongActionType ActionType, int32 Depth) const;
	bool WasRecentlyUsed(EWukongComboSet ComboSet, int32 ComboLength, int32 Depth) const;
	bool WasRecentlyUsed(EWukongNormalAttackType AttackType, int32 Depth) const;
	bool WasRecentlyUsedBowNormalAttack(int32 Depth) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|ComboAttack")
	FWukongComboAttackData ComboAttackA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|ComboAttack")
	FWukongComboAttackData ComboAttackB;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData JumpAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData ChargeAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData DodgeAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData NinjaAAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData NinjaBAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData ExecutionAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData FastComeSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData SlowComeSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData FakeDownSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData LionSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack")
	FWukongNormalAttackData SpinSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack|Bow")
	FWukongNormalAttackData Bow4ComboAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack|Bow")
	FWukongNormalAttackData BowBackDashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|NormalAttack|Bow")
	FWukongNormalAttackData BowHeavyAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dash")
	TObjectPtr<class UAnimMontage> DashForwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dash")
	TObjectPtr<class UAnimMontage> DashBackwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dash")
	TObjectPtr<class UAnimMontage> DashLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dash")
	TObjectPtr<class UAnimMontage> DashRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dodge")
	TObjectPtr<class UAnimMontage> DodgeForwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dodge")
	TObjectPtr<class UAnimMontage> DodgeBackwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dodge")
	TObjectPtr<class UAnimMontage> DodgeLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Dodge")
	TObjectPtr<class UAnimMontage> DodgeRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessLUpMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessLDownMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessRUpMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessRDownMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessFrontUpLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessFrontUpRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterDownLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterDownRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterFollowUpLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterFollowUpRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> ParryBackJumpMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightHitPerfectParryChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightHitNormalParryChance = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrongHitPerfectParryChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrongHitNormalParryChance = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float NormalParryPostureGain = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterBlendOutTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterBlendInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParryCounterDecisionCounterChance = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.1"))
	float ParryCounterPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParryCounterAttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParryCounterFacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParryCounterFacingInterpSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterFollowUpBlendOutTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterFollowUpBlendInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.1"))
	float ParryCounterFollowUpPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.1"))
	float ParryBackJumpPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParryBackJumpBlendOutTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParryBackJumpBlendInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParryCounterFollowUpAttackRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParryCounterFollowUpFacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParryCounterFollowUpFacingInterpSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParrySuccessFacingDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParrySuccessFacingInterpSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Parry", meta = (ClampMin = "0.0"))
	float ParrySuccessFallbackDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry|VFX")
	FName ParryParticleComponentName = TEXT("Parry_Particle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry|VFX")
	bool bAutoDeactivateParryParticle = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Parry|VFX", meta = (ClampMin = "0"))
	float ParryParticleAutoDeactivateDelay = 0.1f;

	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> CachedParryParticleComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	TObjectPtr<class UAnimMontage> CurrentParrySuccessMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	float ParrySuccessFallbackRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bHasSelectedInitialParrySuccessSide = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bNextParrySuccessUsesLeftSide = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bNextParryFrontUpUsesLeftMontage = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bCurrentParryCounterUsesLeftMontage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bCurrentParryCounterUsesDownMontage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bParryCounterStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bCurrentParryCounterPerfectParried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Parry")
	bool bParryCounterFollowUpStarted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Recovery")
	float RecoveryDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Plan", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AttackPlanWeight = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Plan", meta = (ClampMin = "0.0"))
	float PlannedAttackMaxWaitTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Approach")
	float NoAttackCandidateApproachDelay = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Approach")
	bool bForceApproachAfterBowAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Approach", meta = (ClampMin = "0.0"))
	float PostBowCloseRangeForceMaxTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float StrafeMinDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float StrafeMoveInputStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float BackStrafeMoveInputStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float StrafeMoveSpeedScale = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float StrafeAnimInputStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float StrafeMinDistanceToTarget = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloseRangeBackDashChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float CloseRangeBackDashCooldown = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float YawMismatchRepositionChance = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float YawMismatchIdleHoldDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float AttackBackOffDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float AttackBackOffMoveInputStrength = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Debug")
	bool bDebugCodeDrivenAttackMove = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bUseAttackPatternTestFilter = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableComboA = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableComboB = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableJumpAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableChargeAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableDodgeAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableNinjaA = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableNinjaB = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableExecution = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableFastComeSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableSlowComeSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableFakeDownSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableLionSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableSpinSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableBow4Combo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableBowBackDashAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|PatternTest")
	bool bTestEnableBowHeavyAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float EvadeFallbackDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float NoAttackHoldDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Turn")
	float TurnStartAngle = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Turn")
	float MoveTurnStartAngle = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float ApproachFacingInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Movement")
	float EvadeFacingInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|ReactiveAction")
	float ReactiveEvadeBlendInTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|ReactiveAction")
	FWukongReactiveActionTuningData ReactiveBackwardEvade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float WukongLightHitDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float WukongLargeHitDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float WukongLargeHitLongDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float HitReactTurnBlendOutTime = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	ECharacterHitReactDirection LastHitReactDirectionForTurn = ECharacterHitReactDirection::Front_F;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	bool bHitReactTurnTransitionInProgress = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float WukongHeavyHitControlLockDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float WukongPostHeavyHitSuperArmorDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Armor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MontageSuperArmorDurationScale = 0.8f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|HitReact")
	float PostHeavyHitSuperArmorRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Armor")
	float TimedMontageSuperArmorRemainTime = 0.f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Turn")
	float IdleEntryYawTolerance = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wukong|Combat", meta = (ClampMin = "0.0"))
	float PlayerPressureHoldDuration = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongCombatState CurrentCombatSubState = EWukongCombatState::Approach;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongInitiativeState InitiativeState = EWukongInitiativeState::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float PlayerPressureRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float CombatSubStateElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float CloseRangeBackDashCooldownRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float CurrentRepositionDirection = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bUseIdleHoldForYawMismatch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongPlannedActionType PlannedActionType = EWukongPlannedActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongReactiveActionType CurrentReactiveAction = EWukongReactiveActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	FWukongReactiveActionExecutionData ActiveReactiveActionData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|ReactiveAction")
	int32 ReactiveBackwardEvadeCloseHitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|ReactiveAction")
	float ReactiveBackwardEvadeAccumulatedTriggerChance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongPlannedAttackType PlannedAttackType = EWukongPlannedAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongNormalAttackType PlannedNormalAttackType = EWukongNormalAttackType::None;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongPlannedNonAttackType PlannedNonAttackType = EWukongPlannedNonAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongPlannedNonAttackType LastCompletedNonAttackType = EWukongPlannedNonAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongMovementDirection LastCompletedStrafeDirection = EWukongMovementDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bShouldStartNoAttackFallbackWithStrafe = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongMovementDirection PlannedMovementDirection = EWukongMovementDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float PlannedMovementDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongComboSet PlannedComboSet = EWukongComboSet::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	int32 PlannedComboLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float PlannedAttackWaitTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	TArray<FWukongActionRecord> RecentActionHistory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wukong|Combat")
	int32 MaxRecentActionHistory = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongActionType CurrentAttackActionType = EWukongActionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongComboSet CurrentAttackComboSet = EWukongComboSet::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	int32 CurrentAttackComboLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongNormalAttackType CurrentAttackNormalAttackType = EWukongNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongNormalAttackType LastStartedNormalAttackType = EWukongNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bHasDelayedNormalAttackRangeLink = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	EWukongNormalAttackType DelayedNormalAttackRangeLinkType = EWukongNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float DelayedNormalAttackRangeLinkBlendOutTime = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float DelayedNormalAttackRangeLinkBlendInTime = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bDelayedNormalAttackRangeLinkUseTestFilter = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bNoAttackCandidateApproachInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bUseNonAttackFallbackUntilAttackCandidateAppears = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	float NoAttackCandidateApproachRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Combat")
	bool bPostBowCloseRangeApproachInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Bow")
	float PostBowCloseRangeForceElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|Bow")
	bool bBowAttackPresentationActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	bool bPendingCodeDrivenAttackMove = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	bool bCodeDrivenAttackMoveActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	EWukongNormalAttackType CodeDrivenAttackMoveType = EWukongNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float CodeDrivenAttackMoveDelayRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	FVector CodeDrivenAttackMoveStartLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	EWukongCodeMoveStopReason LastCodeDrivenAttackMoveStopReason = EWukongCodeMoveStopReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float LastCodeDrivenAttackMoveTargetDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float LastCodeDrivenAttackMoveTravelDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float LastCodeDrivenAttackMoveStopDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float LastCodeDrivenAttackMoveMaxDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	bool bAttackGroundMotionOverrideActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float AttackGroundMotionOverrideRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float DefaultGroundFriction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float DefaultBrakingDecelerationWalking = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wukong|AttackMotion")
	float DefaultBrakingFrictionFactor = 0.f;
};
