#pragma once

#include "CoreMinimal.h"
#include "Character/Monster/JunMonster.h"
#include "Character/Monster/Boss/JunBossTypes.h"
#include "JunBoss.generated.h"

enum class EJunCombatDefenseVFX : uint8;

UCLASS()
class JUNDUCK_API AJunBoss : public AJunMonster
{
	GENERATED_BODY()

public:
	#pragma region Notify And External Entry Points

	AJunBoss();
	void BeginCodeDrivenAttackMoveWindow(EBossNormalAttackType AttackType);
	void EndCodeDrivenAttackMoveWindow();
	void BeginLightningSwordAirMoveWindow(
		EBossLightningSwordAirMoveMode MoveMode,
		float TargetHeightOffset,
		float RiseSpeed,
		float DiveSpeed,
		float DiveGravityScale,
		bool bLockHorizontalVelocity);
	void EndLightningSwordAirMoveWindow(EBossLightningSwordAirMoveMode MoveMode);
	void BeginAttackSuperArmorWindow();
	void EndAttackSuperArmorWindow();
	void BeginAirborneHitReactLockWindow();
	void EndAirborneHitReactLockWindow();
	void HandleComboDecisionPoint();
	EBossNormalAttackLinkResult ProcessNormalAttackLinkRequest(const FBossNormalAttackLinkRequest& Request);
	EBossNormalAttackLinkResult TryExecuteDelayedNormalAttackRangeLinkFromNotify(float BlendOutTimeOverride = -1.f);
	bool AdvanceVariantNormalAttackFromNotify();
	EBossNormalAttackType GetCurrentNormalAttackType() const { return CurrentAttackRuntime.NormalAttackType; }
	bool ShouldReduceFootPlacementAlpha() const;
	bool WasNormalAttackRecentlyUsed(EBossNormalAttackType AttackType, int32 Depth = 1) const;
	bool IsNormalAttackAllowedByCurrentPhase(EBossNormalAttackType NormalAttackType) const;
	void HandleHitTurnDecisionPoint();
	void FinishParrySuccessStateFromNotify();
	virtual void NotifyAttackParriedBy(
		class AJunPlayer* Parrier,
		float PostureScale = 1.f,
		const FJunAttackDefenseRuleData& DefenseRuleData = FJunAttackDefenseRuleData()) override;
	virtual bool NotifyMikiriCounteredBy(class AJunPlayer* CounterPlayer) override;

	#pragma endregion

protected:
	#pragma region Monster Overrides

	virtual void HandleGameplayEventNotify(FGameplayTag EventTag) override;
	virtual void EnterCombatState() override;
	virtual bool ShouldShowBossCombatHUD() const override { return true; }
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
	virtual void OnIncomingHitResolvedWithoutHitReact(EHitReactType HitType) override;
	virtual bool CanBeInterruptedBy(EHitReactType IncomingHitReact) const override;
	virtual bool ShouldStartHitReact(EHitReactType IncomingHitReact) const override;
	virtual ECharacterHitReactDirection AdjustHitReactDirection(
		EHitReactType HitType,
		ECharacterHitReactDirection HitDirection,
		const FJunAttackDefenseRuleData& DefenseRuleData) const override;
	virtual void OnHitReactEnded(EHitReactType EndedHitReact) override;
	virtual void OnExecutionReadyStarted() override;
	virtual void OnExecutionReadyEnded(bool bMissedExecution) override;
	virtual void FinishExecutionRecovery() override;
	virtual EJunDefenseOutcome TryHandleIncomingHitBeforeDamage(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		const FJunAttackDefenseRuleData& DefenseRuleData) override;

	#pragma endregion

	#pragma region State Machine Core

	void UpdateBossCombatTimers(float DeltaTime);
	bool EnsureBossCombatTargetValid();
	void UpdateCurrentBossCombatState(float DeltaTime);
	void TransitionBossCombatState(
		EBossCombatState NewState,
		EBossCombatStateChangeReason Reason = EBossCombatStateChangeReason::Unspecified,
		EBossCombatStateTransitionMode TransitionMode = EBossCombatStateTransitionMode::EnterState);
	bool IsBossCombatStateTransitionAllowed(
		EBossCombatState FromState,
		EBossCombatState ToState,
		EBossCombatStateChangeReason Reason,
		EBossCombatStateTransitionMode TransitionMode) const;
	void ExitCurrentBossCombatState(EBossCombatState NextState, EBossCombatStateChangeReason Reason);
	bool IsBossCombatStateTransitionSuspicious(
		EBossCombatState FromState,
		EBossCombatState ToState,
		EBossCombatStateChangeReason Reason,
		bool bEnteredState,
		FString& OutWarning) const;
	void RecordBossCombatStateTransition(
		EBossCombatState FromState,
		EBossCombatState ToState,
		EBossCombatStateChangeReason Reason,
		bool bEnteredState);

	#pragma endregion

	#pragma region Combat State Handlers

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
	void ResetAttackEntryRuntimeState();
	bool TryPrepareNormalAttackEntry(
		const FMonsterAttackSelection& AttackSelection,
		const FBossNormalAttackData* CurrentNormalAttackData);
	void PrepareComboAttackEntry(const FMonsterAttackSelection& AttackSelection);
	void QueueCodeMoveForAttackEntryIfNeeded(
		const FBossNormalAttackData* CurrentNormalAttackData,
		float CurrentNormalAttackPlayRate);
	void ApplyAttackEntrySuperArmorIfNeeded(
		const FMonsterAttackSelection& AttackSelection,
		const FBossNormalAttackData* CurrentNormalAttackData);
	void ConfigureAttackEntryAfterMontageStart();
	bool ProcessBossAttackExecution(const FBossAttackExecutionRequest& Request);
	bool ProcessBossSpecialReactionExecution(const FBossSpecialReactionExecutionRequest& Request);

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

	#pragma endregion

	#pragma region Attack Selection And Shared Data

	bool CanStartBossTurn() const;
	bool TryStartOrWaitForBossTurn(float YawThreshold);
	bool TryStartHitReactTurnTransition(bool bStopHitReactMontage);
	bool ShouldTurnAfterHitReact(ECharacterHitReactDirection HitDirection) const;
	bool HasAnyAttackCandidateForCurrentDistance() const;
	bool HasValidPlannedAttack() const;
	bool HasValidPlannedMovement() const;
	bool IsMobilityNonAttackType(EBossPlannedNonAttackType NonAttackType) const;
	bool IsComboSetEnabledByTestFilter(EBossComboSet ComboSet) const;
	bool IsNormalAttackEnabledByTestFilter(EBossNormalAttackType NormalAttackType) const;
	bool IsPhaseTwoActive() const;
	bool IsComboSetAllowedInCurrentPhase(EBossComboSet ComboSet) const;
	bool IsNormalAttackAllowedInCurrentPhase(EBossNormalAttackType NormalAttackType) const;
	float GetMaxEnabledComboCandidateRange() const;
	float GetMaxEnabledAttackCandidateRange() const;
	bool IsFartherThanAllAttackCandidates() const;
	bool HasAnyNonBowAttackCandidateForCurrentDistance() const;
	EBossMovementDirection ChooseStrafeDirectionAvoidingImmediateReverse() const;
	bool IsOppositeStrafeDirection(EBossMovementDirection A, EBossMovementDirection B) const;
	void CollectNormalAttackCandidates(
		float TargetDistance,
		const FBossNormalAttackCandidatePolicy& Policy,
		TArray<FBossWeightedNormalAttackCandidate>& OutCandidates) const;
	bool CanUseNormalAttackCandidate(
		EBossNormalAttackType NormalAttackType,
		float TargetDistance,
		const FBossNormalAttackCandidatePolicy& Policy,
		const FBossNormalAttackData*& OutNormalAttackData) const;
	EBossNormalAttackType SelectNormalAttackCandidate(
		const TArray<FBossWeightedNormalAttackCandidate>& Candidates) const;
	int32 GetComboAttackSelectionWeight() const;
	bool HasAnyComboMontage() const;
	const FBossComboAttackData* GetComboAttackData(EBossComboSet ComboSet) const;
	bool IsComboAttackUsable(const FBossComboAttackData& ComboAttackData) const;
	int32 GetComboAttackMaxLength(const FBossComboAttackData& ComboAttackData) const;
	FName GetComboSectionName(int32 ComboLength) const;
	void ConfigurePlannedComboMontageSections();
	bool ShouldContinueComboAttack(const FBossComboAttackData& ComboAttackData, int32 CurrentComboLength) const;
	const FBossReactiveActionTuningData* GetReactiveActionTuningData(EBossReactiveActionType ReactiveActionType) const;
	bool IsWithinSelectionRange(const FMonsterAttackSelection& Selection) const;
	const FBossNormalAttackData* GetNormalAttackData(EBossNormalAttackType AttackType) const;
	UAnimMontage* GetPlannedComboMontage() const;
	const FBossNormalAttackData* GetPlannedNormalAttackData() const;
	float GetPlannedComboFacingDuration() const;
	UAnimMontage* GetMobilityNonAttackMontage(EBossPlannedNonAttackType NonAttackType, EBossMovementDirection MovementDirection) const;
	UAnimMontage* GetPlannedNonAttackMontage() const;
	UAnimMontage* GetReactiveActionMontage() const;
	EBossCombatState GetReactiveActionTargetState(EBossReactiveActionType ReactiveActionType) const;
	float GetTargetYawAbsDelta() const;
	float GetTargetDistance2D() const;
	float GetPlannedAttackMinRange() const;
	float GetPlannedAttackMaxRange() const;
	float GetAttackCandidateMaxRange(EBossPlannedAttackType AttackType) const;
	bool IsTargetTooFarForPlannedAttack() const;
	bool IsTargetTooCloseForPlannedAttack() const;
	bool CanTriggerCloseRangeBackDash() const;
	void StartCloseRangeBackDashCooldown();
	enum class EBossFacingDecisionType : uint8
	{
		None,
		Rotate,
		StartTurn
	};

	struct FBossFacingDecision
	{
		EBossFacingDecisionType Type = EBossFacingDecisionType::None;
		FRotator TargetRotation = FRotator::ZeroRotator;
	};

	FBossFacingDecision EvaluateFacingTowardsTargetWithoutTurn() const;
	void ApplyFacingDecision(const FBossFacingDecision& Decision, float DeltaTime, float InterpSpeed);
	void UpdateFacingTowardsTargetWithoutTurn(float DeltaTime, float InterpSpeed);
	void StopMovementForBossTurn();
	void BeginNoAttackCandidateApproach();
	void ClearNoAttackCandidateApproach();
	void ResetReactiveActionState();
	void ResetActiveReactiveActionState();
	void ResetReactiveEvadePressure();
	void ResetPendingAttackMotionState();
	virtual void ResetCurrentAttackRuntimeState() override;

	#pragma endregion

	#pragma region Attack Link

	void QueueDelayedNormalAttackRangeLink(
		EBossNormalAttackType AttackType,
		float BlendOutTime,
		float BlendInTime,
		bool bUseTestFilter);
	bool ConsumeDelayedNormalAttackRangeLink(FBossDelayedNormalAttackLinkRequest& OutRequest);
	void ClearDelayedNormalAttackRangeLink();
	EBossNormalAttackLinkResult TryStartNormalAttackLinkFromNotify(
		EBossNormalAttackType NextAttackType,
		float TriggerChance,
		float BlendOutTime,
		float BlendInTime,
		bool bRequireRange,
		bool bMoveToRangeWhenOutOfRange,
		bool bDelayMoveToRangeWhenOutOfRange,
		bool bUseTestFilter);
	EBossNormalAttackLinkResult TryForceNormalAttackLinkWhenTriggerChanceFails(float BlendOutTime, float BlendInTime);
	EBossNormalAttackLinkResult ValidateNormalAttackLinkRequest(
		EBossNormalAttackType NextAttackType,
		bool bUseTestFilter,
		const FBossNormalAttackData*& OutNextAttackData,
		UAnimMontage*& OutNextMontage,
		UAnimInstance*& OutAnimInstance,
		bool& bOutLinkingFromParrySuccess) const;
	EBossNormalAttackLinkResult HandleOutOfRangeNormalAttackLink(
		EBossNormalAttackType NextAttackType,
		const FBossNormalAttackData& NextAttackData,
		UAnimInstance& AnimInstance,
		float TargetDistance,
		float BlendOutTime,
		float BlendInTime,
		bool bMoveToRangeWhenOutOfRange,
		bool bDelayMoveToRangeWhenOutOfRange,
		bool bUseTestFilter,
		bool bLinkingFromParrySuccess);
	void CleanupCurrentAttackForNormalAttackLink(
		EBossNormalAttackType NextAttackType,
		UAnimInstance& AnimInstance,
		float BlendOutTime,
		bool bLinkingFromParrySuccess,
		bool bResetAttackRuntime);
	EBossNormalAttackLinkResult StartImmediateNormalAttackLink(
		EBossNormalAttackType NextAttackType,
		const FBossNormalAttackData& NextAttackData,
		UAnimMontage& NextMontage,
		UAnimInstance& AnimInstance,
		float BlendInTime);

	#pragma endregion

	#pragma region Variant Attack And Bow

	bool TryStartVariantNormalAttack(EBossNormalAttackType AttackType, const FBossNormalAttackData& AttackData);
	bool PlayVariantNormalAttackDashStep();
	bool PlayVariantNormalAttackFinalMontage();
	bool IsVariantNormalAttackType(EBossNormalAttackType AttackType) const;
	float GetVariantNormalAttackChance(EBossNormalAttackType AttackType) const;
	bool IsWithinVariantNormalAttackDistance(EBossNormalAttackType AttackType, const FBossNormalAttackData& AttackData) const;
	UAnimMontage* GetVariantNormalAttackDashMontage(EBossNormalAttackType AttackType) const;
	UAnimMontage* GetVariantNormalAttackFinalMontage(EBossNormalAttackType AttackType, const FBossNormalAttackData& AttackData) const;
	int32 GetVariantNormalAttackRequiredDashCount(EBossNormalAttackType AttackType) const;
	void ClearVariantNormalAttackState();
	bool IsBowNormalAttackType(EBossNormalAttackType AttackType) const;
	void BeginBowAttackPresentation();
	void EndBowAttackPresentation();

	#pragma endregion

	#pragma region Combat Planning And Runtime

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
	bool TryConsumeReactiveBossActionFromIdle();
	bool EvaluateNextBossActionFromIdle();
	bool TryExecutePlannedBossActionFromIdle();
	bool TryExecutePlannedAttackFromIdle();
	bool TryExecutePlannedMovementFromIdle();
	bool HasQueuedReactiveAction() const;
	bool CanStartMobilityMontageSafely() const;
	void RefreshPlannedCombatPlan();
	bool IsCurrentCombatPlanValid() const;
	bool TryPlanForcedApproachAfterBowAttack(bool bShouldCloseAfterBow);
	bool TryPlanApproachUntilAttackCandidate();
	bool TryPlanNoAttackFallback();
	bool TryPlanAttackCombatPlan(bool bShouldCloseAfterBow);
	void PlanNonAttackCombatPlan();
	void RefreshPlannedAttackType();
	void ResetPlannedAttackChoice();
	bool BuildNormalAttackCandidatesForPlan(float TargetDistance, TArray<FBossWeightedNormalAttackCandidate>& OutNormalAttackCandidates) const;
	void CollectPlannedAttackTypeCandidates(
		const TArray<FBossWeightedNormalAttackCandidate>& NormalAttackCandidates,
		TArray<EBossPlannedAttackType>& OutAttackCandidates) const;
	bool ApplyPlannedAttackTypeSelection(
		const TArray<EBossPlannedAttackType>& AttackCandidates,
		const TArray<FBossWeightedNormalAttackCandidate>& NormalAttackCandidates);
	void RefreshPlannedNonAttackType();
	void RefreshNoAttackFallbackMovementType();
	bool TryQueueReactiveAction(EBossReactiveActionType ReactiveActionType);
	bool TryReactToHitDirection(ECharacterHitReactDirection HitDirection);
	FBossReactiveActionDecision EvaluateReactiveActionDecision(ECharacterHitReactDirection HitDirection) const;
	bool ApplyReactiveActionDecision(const FBossReactiveActionDecision& Decision);
	bool EvaluateReactiveActionExecution(
		EBossReactiveActionType ReactiveActionType,
		FBossReactiveActionExecutionData& OutExecutionData) const;
	void ApplyReactiveActionExecution(const FBossReactiveActionExecutionData& ExecutionData);
	bool TryConsumeReactiveAction();
	bool EvaluateReactiveBackwardEvade(FBossReactiveActionExecutionData& OutExecutionData) const;

	#pragma endregion

	#pragma region Parry

	FBossParryDecision EvaluateParryDecision(
		EHitReactType HitType,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseRuleData& DefenseRuleData) const;
	bool ApplyParryDecision(
		const FBossParryDecision& Decision,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData);
	bool IsDamageCauserInParryFrontCone(const AActor* DamageCauser) const;
	bool IsParryableHitType(EHitReactType HitType) const;
	float GetPerfectParryChanceForHitType(EHitReactType HitType) const;
	float GetNormalParryChanceForHitType(EHitReactType HitType) const;
	void ResetParryExchangeCycle();
	void ResetParryExchangePerfectChance(EHitReactType HitType);
	void AccumulateParryExchangeNormalChance(EHitReactType HitType);
	void AccumulateParryExchangePerfectChance(EHitReactType HitType);
	void UpdateParryExchangeDecay(float DeltaTime);
	bool ShouldEscapeParryExchangeBeforeCounter() const;
	UAnimMontage* GetParryExchangeEscapeMontage() const;
	UAnimMontage* GetParrySuccessMontage(const FVector& SwingDirection);
	UAnimMontage* GetParryCounterMontage() const;
	UAnimMontage* GetParryCounterFollowUpMontage() const;
	bool TryStartPerfectParryRebound(AJunPlayer* Parrier, const FJunAttackDefenseRuleData& DefenseRuleData);
	UAnimMontage* GetPerfectParryReboundMontage(const AJunPlayer* Parrier) const;
	USceneComponent* FindSceneComponentByName(FName ComponentName) const;
	void PlayParryParticle(EJunCombatDefenseVFX EffectType);
	void StartParrySuccessAgainstIncomingHit(
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		EJunCombatDefenseVFX EffectType);
	bool TryStartParryCounterDecision();
	bool TryStartParryCounterAttack();
	bool TryStartParryBackJump();
	bool TryStartParryCounterFollowUp();

	UFUNCTION()
	void OnParrySuccessMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void FinishParrySuccessState();

	#pragma endregion

	#pragma region Code Move And Action History

	void QueueCodeDrivenAttackMove(EBossNormalAttackType AttackType, float Delay);
	void StartCodeDrivenAttackMove(EBossNormalAttackType AttackType);
	void UpdateCodeDrivenAttackMove(float DeltaTime);
	void StopCodeDrivenAttackMove(bool bClearVelocity = true, EBossCodeMoveStopReason StopReason = EBossCodeMoveStopReason::Interrupted);
	void CacheCodeDrivenAttackMoveDebug(EBossCodeMoveStopReason StopReason);
	void UpdateLightningSwordAirMove(float DeltaTime);
	void StopLightningSwordAirMove(bool bClearVelocity = true);
	void ApplyAttackGroundMotionOverride(float Duration);
	void RestoreAttackGroundMotionOverride();
	void RecordAction(EBossActionType ActionType, EBossComboSet ComboSet = EBossComboSet::None, int32 ComboLength = 0, EBossNormalAttackType NormalAttackType = EBossNormalAttackType::None);
	bool WasRecentlyUsed(EBossActionType ActionType, int32 Depth) const;
	bool WasRecentlyUsed(EBossComboSet ComboSet, int32 ComboLength, int32 Depth) const;
	bool WasRecentlyUsed(EBossNormalAttackType AttackType, int32 Depth) const;
	bool WasRecentlyUsedBowNormalAttack(int32 Depth) const;

	#pragma endregion

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|ComboAttack")
	FBossComboAttackData ComboAttackA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|ComboAttack")
	FBossComboAttackData ComboAttackB;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData JumpAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData ChargeAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData DodgeAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData NinjaAAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData NinjaBAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData ExecutionAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData FastComeSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData SlowComeSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData FakeDownSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData LionSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData SpinSlashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack")
	FBossNormalAttackData LightningSwordAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Bow")
	FBossNormalAttackData Bow4ComboAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Bow")
	FBossNormalAttackData BowBackDashAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Bow")
	FBossNormalAttackData BowHeavyAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dash")
	TObjectPtr<class UAnimMontage> DashForwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dash")
	TObjectPtr<class UAnimMontage> DashBackwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dash")
	TObjectPtr<class UAnimMontage> DashLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dash")
	TObjectPtr<class UAnimMontage> DashRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dodge")
	TObjectPtr<class UAnimMontage> DodgeForwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dodge")
	TObjectPtr<class UAnimMontage> DodgeBackwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dodge")
	TObjectPtr<class UAnimMontage> DodgeLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dodge")
	TObjectPtr<class UAnimMontage> DodgeRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	TObjectPtr<class UAnimMontage> ChargeVariantDashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	TObjectPtr<class UAnimMontage> ChargeVariantFollowUpDashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	TObjectPtr<class UAnimMontage> NinjaBVariantDashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	TObjectPtr<class UAnimMontage> NinjaBVariantFollowUpDashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	TObjectPtr<class UAnimMontage> ChargeVariantAttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	TObjectPtr<class UAnimMontage> NinjaBVariantAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChargeVariantChance = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float ChargeVariantMinDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float ChargeVariantMaxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NinjaBVariantChance = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float NinjaBVariantMinDistance = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float NinjaBVariantMaxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float VariantDashBlendOutTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float VariantDashBlendInTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.1"))
	float VariantDashPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	bool bVariantDashFaceTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float VariantDashFacingDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant", meta = (ClampMin = "0.0"))
	float VariantDashFacingInterpSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessLUpMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessLDownMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessRUpMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessRDownMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessFrontUpLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParrySuccessFrontUpRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterDownLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterDownRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterFollowUpLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryCounterFollowUpRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry")
	TObjectPtr<class UAnimMontage> ParryBackJumpMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Rebound")
	TObjectPtr<class UAnimMontage> PerfectParryReboundLMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Rebound")
	TObjectPtr<class UAnimMontage> PerfectParryReboundRMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightHitPerfectParryChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightHitNormalParryChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrongHitPerfectParryChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrongHitNormalParryChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float ParryFrontConeAngle = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float NormalParryPostureGain = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalParryChanceGainPerIncomingHit = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PerfectParryChanceGainPerNormalParry = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrongNormalParryChanceGainPerIncomingHit = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrongPerfectParryChanceGainPerNormalParry = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0"))
	int32 ParryExchangeEscapeMinNormalParryCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParryExchangeEscapeChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0"))
	float ParryExchangeDecayDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry|Exchange", meta = (ClampMin = "0.0"))
	float ParryExchangeChanceDecayPerSecond = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterBlendOutTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterBlendInTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParryCounterDecisionCounterChance = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.1"))
	float ParryCounterPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParryCounterAttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParryCounterFacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParryCounterFacingInterpSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterFollowUpBlendOutTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParryCounterFollowUpBlendInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.1"))
	float ParryCounterFollowUpPlayRate = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.1"))
	float ParryBackJumpPlayRate = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParryBackJumpBlendOutTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParryBackJumpBlendInTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParryCounterFollowUpAttackRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParryCounterFollowUpFacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParryCounterFollowUpFacingInterpSpeed = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParrySuccessFacingDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	float ParrySuccessFacingInterpSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0.0"))
	float ParrySuccessFallbackDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Parry|VFX")
	FName ParryEffectLocationComponentName = TEXT("Parry_Location");

	UPROPERTY(Transient)
	TObjectPtr<class USceneComponent> CachedParryEffectLocationComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	FBossParryExchangeState ParryExchangeState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Parry", meta = (ClampMin = "0"))
	float ParrySuccessExitActionLockDuration = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	bool bHasSelectedInitialParrySuccessSide = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	bool bNextParrySuccessUsesLeftSide = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	bool bNextParryFrontUpUsesLeftMontage = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	bool bCurrentParryCounterUsesLeftMontage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Parry")
	bool bCurrentParryCounterUsesDownMontage = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Recovery")
	float RecoveryDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Plan", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AttackPlanWeight = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Plan", meta = (ClampMin = "0.0"))
	float PlannedAttackMaxWaitTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|MikiriCounter")
	TObjectPtr<class UAnimMontage> MikiriCounterMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterBlendOutTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterBlendInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|MikiriCounter", meta = (ClampMin = "0.1"))
	float MikiriCounterPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterPostureGain = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Approach")
	float NoAttackCandidateApproachDelay = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Approach")
	bool bForceApproachAfterBowAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Approach", meta = (ClampMin = "0.0"))
	float PostBowCloseRangeForceMaxTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float StrafeMinDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float StrafeMoveInputStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float BackStrafeMoveInputStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float StrafeMoveSpeedScale = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float StrafeAnimInputStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float StrafeMinDistanceToTarget = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloseRangeBackDashChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float CloseRangeBackDashCooldown = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float YawMismatchRepositionChance = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float YawMismatchIdleHoldDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float AttackBackOffDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float AttackBackOffMoveInputStrength = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Debug")
	bool bDebugCodeDrivenAttackMove = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bUseAttackPatternTestFilter = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableComboA = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestComboARequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableComboB = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestComboBRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableJumpAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestJumpAttackRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableChargeAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestChargeAttackRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableDodgeAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestDodgeAttackRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableNinjaA = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestNinjaARequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableNinjaB = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestNinjaBRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableExecution = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestExecutionRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableFastComeSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestFastComeSlashRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableSlowComeSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestSlowComeSlashRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableFakeDownSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestFakeDownSlashRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableLionSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestLionSlashRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableSpinSlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestSpinSlashRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableLightningSword = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestLightningSwordRequiresPhaseTwo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableBow4Combo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestBow4ComboRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableBowBackDashAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestBowBackDashAttackRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestEnableBowHeavyAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|PatternTest")
	bool bTestBowHeavyAttackRequiresPhaseTwo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float EvadeFallbackDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float NoAttackHoldDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	float TurnStartAngle = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	float MoveTurnStartAngle = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Facing", meta = (ClampMin = "0", ClampMax = "180"))
	float MaxCodeFacingAngle = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Facing", meta = (ClampMin = "0"))
	float FacingStateEntryLockDuration = 0.08f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Facing")
	float CodeFacingLockRemainTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float ApproachFacingInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Movement")
	float EvadeFacingInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	float ReactiveEvadeBlendInTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|ReactiveAction")
	FBossReactiveActionTuningData ReactiveBackwardEvade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	float BossLightHitDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	float BossLargeHitDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	float BossLargeHitLongDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	float HitReactTurnBlendOutTime = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	ECharacterHitReactDirection LastHitReactDirectionForTurn = ECharacterHitReactDirection::Front_F;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	bool bHitReactTurnTransitionInProgress = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|HitReact")
	float BossHeavyHitControlLockDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Armor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MontageSuperArmorDurationScale = 0.8f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Armor")
	float TimedMontageSuperArmorRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Armor")
	bool bAirborneHitReactLockWindowActive = false;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	float IdleEntryYawTolerance = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat", meta = (ClampMin = "0.0"))
	float PlayerPressureHoldDuration = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossCombatState CurrentCombatSubState = EBossCombatState::Approach;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossCombatStateChangeReason LastCombatSubStateChangeReason = EBossCombatStateChangeReason::Unspecified;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	TArray<FBossCombatStateTransitionRecord> CombatStateTransitionHistory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxCombatStateTransitionHistory = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|StatePolicy")
	bool bAuditBossCombatStateTransitions = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|StatePolicy")
	bool bEnforceBossCombatStateTransitionPolicy = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossInitiativeState InitiativeState = EBossInitiativeState::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float PlayerPressureRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float CombatSubStateElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float CloseRangeBackDashCooldownRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float CurrentRepositionDirection = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bUseIdleHoldForYawMismatch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	FBossCombatPlan CurrentCombatPlan;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	FBossReactiveActionState ReactiveActionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossPlannedNonAttackType LastCompletedNonAttackType = EBossPlannedNonAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossMovementDirection LastCompletedStrafeDirection = EBossMovementDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bShouldStartNoAttackFallbackWithStrafe = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	TArray<FBossActionRecord> RecentActionHistory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	int32 MaxRecentActionHistory = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	FBossAttackRuntimeState CurrentAttackRuntime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|SpecialReaction")
	EBossSpecialReactionType ActiveSpecialReaction = EBossSpecialReactionType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	EBossNormalAttackType LastStartedNormalAttackType = EBossNormalAttackType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|NormalAttack|Variant")
	FBossVariantAttackState VariantAttackState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	FBossDelayedNormalAttackLinkRequest DelayedNormalAttackRangeLinkRequest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bNoAttackCandidateApproachInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bUseNonAttackFallbackUntilAttackCandidateAppears = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float NoAttackCandidateApproachRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bPostBowCloseRangeApproachInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Bow")
	float PostBowCloseRangeForceElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Bow")
	bool bBowAttackPresentationActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackMotion")
	FBossCodeMoveState CodeMoveState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	bool bLightningSwordAirMoveActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	EBossLightningSwordAirMoveMode LightningSwordAirMoveMode = EBossLightningSwordAirMoveMode::LiftAndHover;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	float LightningSwordAirMoveTargetZ = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	float LightningSwordAirMoveRiseSpeed = 900.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	float LightningSwordAirMoveDiveSpeed = 2200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	float LightningSwordAirMoveDiveGravityScale = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	bool bLightningSwordAirMoveLockHorizontalVelocity = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	TEnumAsByte<EMovementMode> LightningSwordAirMovePreviousMovementMode = MOVE_Walking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	uint8 LightningSwordAirMovePreviousCustomMovementMode = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|LightningSword")
	float LightningSwordAirMovePreviousGravityScale = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackMotion")
	bool bAttackGroundMotionOverrideActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackMotion")
	float AttackGroundMotionOverrideRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackMotion")
	float DefaultGroundFriction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackMotion")
	float DefaultBrakingDecelerationWalking = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AttackMotion")
	float DefaultBrakingFrictionFactor = 0.f;
};
