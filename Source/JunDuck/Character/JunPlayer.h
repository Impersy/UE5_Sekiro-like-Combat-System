#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifyState_AttackComboAdvance.h"
#include "Character/JunCharacter.h"
#include "JunPlayer.generated.h"

UENUM(BlueprintType)
enum class EJunCameraMode : uint8
{
	Free,
	LockOn,
	Execution,
	Death
};

UENUM(BlueprintType)
enum class EJunDefenseState : uint8
{
	None = 0,
	Starting = 1,
	Guarding = 3,
	Ending = 4
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
enum class EJunHeavyAttackState : uint8
{
	None,
	Tap,
	ChargeStart,
	ChargeLoop,
	ChargeEnd
};

UENUM(BlueprintType)
enum class EJunJumpAttackState : uint8
{
	None,
	Start,
	Loop,
	End
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
enum class EJunMoveState : uint8
{
	Walk,
	Run,
	Sprint,
	Guard
};

UENUM(BlueprintType)
enum class EJunPlayerHitResolveResult : uint8
{
	Ignored,
	PerfectParrySuccess,
	NormalParrySuccess,
	GuardBlock,
	HitReact
};

UENUM(BlueprintType)
enum class EJunPlayerHitState : uint8
{
	None,
	ParrySuccess,
	GuardBlock,
	GuardBreak,
	HitReact
};

UENUM(BlueprintType)
enum class EJunAirHeavyHitStage : uint8
{
	None,
	Launch,
	Down,
	Land
};

UENUM(BlueprintType)
enum class EJunPlayerDeathState : uint8
{
	Alive,
	FakeDeath,
	FakeDeathReviving,
	RealDeath,
	Respawning
};

UCLASS()
class JUNDUCK_API AJunPlayer : public AJunCharacter
{
	GENERATED_BODY()

public:
	AJunPlayer();

public: // Engine / Character Overrides
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker) override;
	virtual bool Is_Invincible() const override;
	virtual void Jump() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void HandleGameplayEventNotify(FGameplayTag EventTag) override;
	virtual void BeginAttackTraceWindow(
		EHitReactType HitReactType = EHitReactType::LightHit,
		const FJunAttackDamageData& DamageData = FJunAttackDamageData(),
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData = FJunAttackDefenseKnockbackData(),
		const FJunAttackDefenseRuleData& DefenseRuleData = FJunAttackDefenseRuleData(),
		EJunWeaponNiagaraComponent NiagaraComponent = EJunWeaponNiagaraComponent::Trail,
		const FJunAttackTraceOverrideData& TraceOverrideData = FJunAttackTraceOverrideData()) override;
	virtual void EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent = EJunWeaponNiagaraComponent::Trail) override;
	virtual void BeginWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType) override;
	virtual void EndWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType) override;

public: // External Gameplay API
	void BasicAttack();
	void OnHeavyAttackStarted();
	void OnHeavyAttackReleased();
	void StartDodge();
	void OnDodgeInputReleased();
	void OnDefenseStarted();
	void OnDefenseReleased();
	void ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData = FJunAttackDefenseKnockbackData(),
	const FJunAttackDefenseRuleData& DefenseRuleData = FJunAttackDefenseRuleData(),
	bool bCanBuildAttackerPostureOnParry = true);
	void AddCameraLookInput(const FVector2D& Input);
	void SnapCameraToLookAt(const FVector& WorldTarget, float Pitch = -24.f);
	void ToggleLockOn();
	bool TryCancelJumpAttackEndIntoMove();
	bool TryCancelJumpAttackEndIntoDodge();
	bool TryCancelJumpAttackEndIntoBasicAttack();
	bool TryCancelJumpAttackEndIntoHeavyAttack();
	bool TryCancelJumpAttackEndIntoJigen();
	bool TryStartDodgeAttack();
	bool TryCancelDodgeAttackIntoMove();
	bool TryCancelDodgeAttackIntoDodge();
	bool TryCancelDodgeAttackIntoBasicAttack();
	bool TryCancelDodgeAttackIntoHeavyAttack();
	bool TryCancelDodgeAttackIntoJigen();
	bool TryCancelHeavyAttackIntoBasicAttack();
	bool TryCancelDodgeIntoBasicAttack();
	bool TryCancelDodgeIntoHeavyAttack();
	bool TryCancelDodgeIntoJigen();
	bool TryCancelDodgeIntoJump();
	bool TryCancelDodgeIntoDefense();
	bool TryOpenMikiriCounterWindow(bool bRequireForwardInput = true);
	bool TryStartJumpCounterStompFollowUp();
	bool TryCancelJumpCounterStompFollowUpIntoJumpAttack();
	void BeginDodgeAttackWindow();
	void EndDodgeAttackWindow();
	void BeginJumpCounterStompJumpAttackWindow();
	void EndJumpCounterStompJumpAttackWindow();
	void BeginDodgeChainWindow();
	void EndDodgeChainWindow();
	void NotifyGuardRestartAnchorReached(class UAnimMontage* Montage, float AnchorPosition);
	bool TryStartExecution();
	bool TryChooseFakeDeathDie();
	bool TryChooseFakeDeathRevive();
	bool TryStartDrinkPotion();
	bool TryStartJigen();
	void SetHpForTutorial(int32 NewHp);
	void SetPostureForTutorial(float NewPosture);
	void ApplyJumpCounterStompBounce(float UpVelocity, float BackwardVelocity);
	void ApplyJumpCounterStompBounce(float UpVelocity, float BackwardVelocity, float BackwardMoveDuration);
	void ApplyJumpCounterStompHit(
		EHitReactType HitReactType,
		const FJunAttackDamageData& DamageData,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData);
	UFUNCTION(BlueprintCallable, Category = "Equipment|Hat")
	void EquipHat(TSubclassOf<class AHatActor> NewHatClass);
	UFUNCTION(BlueprintCallable, Category = "Equipment|Hat")
	void UnequipHat();

public: // Query / State API
	bool GetPlayerIsFalling();
	bool IsLockOn();
	class AJunCharacter* GetLockOnTarget() const { return LockOnTarget.Get(); }
	FVector GetLockOnMarkerWorldPoint();
	bool IsGuardOn();
	bool IsGuardPoseActive();
	bool IsBasicAttacking() const;
	bool IsHeavyAttacking() const;
	bool IsJigenAttacking() const;
	bool IsJumpAttacking() const;
	bool IsDodgeAttacking() const;
	bool IsDrinkingPotion() const { return bIsDrinkingPotion; }
	bool IsWalking() const;
	bool IsSprinting() const;
	bool IsInParrySuccess() const;
	bool IsGuardBlockReacting() const;
	bool IsGuardBlockReleasing() const { return bGuardBlockReleasePending; }
	bool IsJumpStartAnimTriggered() const;
	EJunMoveState GetMoveState() const;
	bool CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction Action) const;
	bool CanBufferDefenseTransitionCancel() const;
	bool CanCancelDefenseTransitionIntoMove() const;
	bool CanBufferParrySuccessCancel(EJunBufferedParrySuccessCancelAction Action) const;
	bool ShouldUseGuardBase() const;
	EJunDefenseState GetDefenseState() const;
	int32 GetGuardStartRestartSerial() const { return GuardStartRestartSerial; }
	bool IsParryWindowOpen() const;
	float GetDesiredMoveForward() const;
	float GetDesiredMoveRight() const;
	float GetGuardMoveBlendRemainTime() const;
	bool ShouldDeferGuardMoveInput() const;
	float GetDesiredMaxWalkSpeed() const;
	bool ShouldUseRunningAnim() const;
	bool IsExecuting() const;
	bool IsInDeathSequence() const;
	bool IsWaitingForFakeDeathChoice() const;
	bool ShouldSuppressLandingAnim() const { return LandingAnimSuppressRemainTime > 0.f; }
	bool ShouldSuppressAirborneAnim() const { return LandingAnimSuppressRemainTime > 0.f; }
	bool IsMikiriCounterThreatAvailable() const;
	bool DidLastParrySuccessUseLeftSide() const { return bLastParrySuccessUsedLeftSide; }
	EHitReactType GetLastIncomingHitReactType() const { return LastIncomingHitReactType; }
	float GetCurrentPosture() const { return CurrentPosture; }
	float GetMaxPosture() const { return MaxPosture; }
	int32 GetCurrentDrinkPotionCharges() const { return CurrentDrinkPotionCharges; }
	int32 GetMaxDrinkPotionCharges() const { return MaxDrinkPotionCharges; }

	void BeginTutorialControlLock();
	void EndTutorialControlLock();
	void ToggleWalkingState();
	void SetWalkRequested(bool bNewWalkRequested);
	void RestoreDrinkPotionWeaponVisibility();
	void SetDesiredMoveAxes(float NewForward, float NewRight);
	void OnMoveInputReleased();
	void BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction Action);
	void BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction Action);
	void BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction Action);
	void OnAttackComboAdvanceStateBegin(EJunAttackComboType ComboType);
	void OnAttackComboAdvanceStateEnd(EJunAttackComboType ComboType);
	void OnBasicAttackComboAdvanceStateBegin();
	void OnBasicAttackComboAdvanceStateEnd();
	bool TryCancelBasicAttackIntoMove();
	void BeginAttackFacingWindow(float FacingInterpSpeed);
	void EndAttackFacingWindow();
	void BeginHitReactFacingWindow(float FacingInterpSpeed);
	void EndHitReactFacingWindow();

protected: // BasicAttack
	void OnBasicAttackComboWindowBegin();

	UFUNCTION(BlueprintCallable, Category = "BasicAttack")
	void ResetBasicAttackCombo();

	void StartBasicAttack();
	void TryAdvanceBasicAttackCombo();
	void TryExecuteBufferedBasicAttackRecoveryAction();
	bool CanCancelBasicAttackIntoMove() const;
	void FinishBasicAttack();
	void CancelBasicAttackForRecoveryTransition(float BlendOutTime = 0.05f);
	void CancelBasicAttackIntoDefense();
	bool CanCancelBasicAttackIntoHeavyAttack() const;
	bool TryCancelBasicAttackIntoHeavyAttack();
	bool CanCancelBasicAttackIntoJigen() const;
	bool TryCancelBasicAttackIntoJigen();

	UFUNCTION()
	void OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // HeavyAttack
	void OnHeavyAttackComboWindowBegin();
	void OnHeavyAttackComboAdvanceStateBegin();
	void OnHeavyAttackComboAdvanceStateEnd();
	void TryAdvanceHeavyAttackCombo();
	void UpdateHeavyAttackInput(float DeltaTime);
	bool CanStartHeavyAttack() const;
	void BeginHeavyAttackHoldInput();
	void ResetHeavyAttackChargeInput();
	void StartHeavyAttackTap();
	void StartHeavyAttackCharge();
	void StartHeavyAttackChargeEnd();
	void ExecuteHeavyAttackChargeEndDash();
	void FinishHeavyAttack();
	float GetCurrentHeavyAttackTimelineRate() const;
	bool CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction Action) const;
	void BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction Action);
	void TryExecuteBufferedHeavyAttackRecoveryAction();
	void CancelHeavyAttackForRecoveryTransition(float BlendOutTime = 0.15f);
	bool CanCancelHeavyAttackIntoMove() const;
	bool TryCancelHeavyAttackIntoMove();
	bool CanCancelHeavyAttackIntoBasicAttack() const;
	bool CanCancelHeavyAttackIntoHeavyAttack() const;
	bool TryCancelHeavyAttackIntoHeavyAttack();
	bool CanCancelHeavyAttackIntoJigen() const;
	bool TryCancelHeavyAttackIntoJigen();

	UFUNCTION()
	void OnHeavyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Jigen
	void OnJigenComboWindowBegin();
	void OnJigenComboAdvanceStateBegin();
	void OnJigenComboAdvanceStateEnd();
	void StartJigen();
	void TryAdvanceJigenCombo();
	void FinishJigen();
	void CancelJigen(float BlendOutTime = 0.12f);
	bool CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction Action) const;
	void BufferJigenRecoveryAction(EJunBufferedRecoveryAction Action);
	void TryExecuteBufferedJigenRecoveryAction();
	void CancelJigenForRecoveryTransition(float BlendOutTime = 0.15f);
	bool CanCancelJigenIntoMove() const;
	bool TryCancelJigenIntoMove();
	bool CanCancelJigenIntoBasicAttack() const;
	bool TryCancelJigenIntoBasicAttack();
	bool CanCancelJigenIntoHeavyAttack() const;

	UFUNCTION()
	void OnJigenMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // JumpAttack
	void UpdateJumpAttackState(float DeltaTime);
	bool CanStartJumpAttack() const;
	bool HasEnoughAirTimeForJumpAttack() const;
	float GetDistanceFromGround() const;
	bool IsJumpCounterEvadeSuccessful() const;
	void UpdateJumpCounterStompOpportunity();
	void UpdateJumpCounterStompFollowUp(float DeltaTime);
	bool CanStartJumpCounterStompFollowUp() const;
	FVector GetJumpCounterStompTargetLocation(const class AJunMonster* TargetMonster) const;
	void SetJumpCounterStompTargetCollisionIgnored(class AJunMonster* TargetMonster, bool bIgnore);
	void RestoreJumpCounterStompTargetCollisionIgnored();
	void FinishJumpCounterStompFollowUp(bool bApplyBounce);
	void StartJumpAttack();
	void RequestJumpAttackEnd();
	void EnterJumpAttackEnd();
	void FinishJumpAttack();
	float GetCurrentJumpAttackTimelineRate() const;
	bool CanCancelJumpAttackEndIntoMove() const;
	bool CanCancelJumpAttackEndIntoDodge() const;
	bool CanCancelJumpAttackEndIntoDefense() const;
	bool TryCancelJumpAttackEndIntoDefense();
	bool CanCancelJumpAttackEndIntoBasicAttack() const;
	bool CanCancelJumpAttackEndIntoHeavyAttack() const;
	bool CanCancelJumpAttackEndIntoJigen() const;
	void CancelJumpAttackForRecoveryTransition(float BlendOutTime = 0.1f);

	UFUNCTION()
	void OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnJumpCounterStompFollowUpMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Dodge
	UFUNCTION()
	void OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void UpdateDodgeState(float DeltaTime);
	void StartDodgeInternal(bool bIgnoreDodgeBlockAndReleaseGate);
	void FinishDodgeState();
	void FinishDodgeAttack();
	bool TryStartDodgeChain();
	bool CanCancelDodgeIntoRecoveryAction() const;
	void CancelDodgeForRecoveryTransition(float BlendOutTime);
	bool TryCancelDashDodgeIntoMove();
	float GetCurrentDodgeAttackTimelineRate() const;
	bool CanCancelDodgeAttackIntoMove() const;
	bool CanCancelDodgeAttackIntoDodge() const;
	bool CanCancelDodgeAttackIntoBasicAttack() const;
	bool CanCancelDodgeAttackIntoHeavyAttack() const;
	bool CanCancelDodgeAttackIntoJigen() const;
	bool CanCancelDodgeAttackIntoDefense() const;
	bool TryCancelDodgeAttackIntoDefense();
	void CancelDodgeAttackForRecoveryTransition(float BlendOutTime = 0.1f);
	void AlignActorToDesiredMoveDirectionForDodge();
	class UAnimMontage* GetDodgeMontageToPlay() const;
	bool IsDashDodgeMontage(const UAnimMontage* Montage) const;
	bool IsForwardDashMontage(const UAnimMontage* Montage) const;
	bool IsBackDashMontage(const UAnimMontage* Montage) const;
	bool IsSideDashMontage(const UAnimMontage* Montage) const;
	float GetDodgePlayRateForMontage(const UAnimMontage* Montage) const;
	float GetDodgeRecoveryCancelOpenTimeForMontage(const UAnimMontage* Montage) const;
	float GetDodgeMoveCancelOpenTimeForMontage(const UAnimMontage* Montage) const;
	float GetCurrentDodgeTimelineRate() const;

	UFUNCTION()
	void OnDodgeAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Execution
	bool CanStartExecution(const class AJunMonster* Monster) const;
	void StartExecution(class AJunMonster* Monster);
	void TriggerExecutionTargetMontage();
	void FinishExecution(bool bInterrupted = false);
	void SetExecutionCapsuleCollisionIgnore(class AJunMonster* Monster, bool bIgnore);
	void ReduceExecutionCapsuleRadius(class AJunMonster* Monster);
	void RestoreExecutionCapsuleRadius(class AJunMonster* Monster);
	class UAnimMontage* GetExecutionMontageForTarget(const class AJunMonster* Monster) const;

	UFUNCTION()
	void OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Death
	void EnterFakeDeath();
	void EnterRealDeath();
	void ConfirmRealDeathFromFakeDeath();
	bool TryStartAirDeathSequence();
	void StartAirDeathHitFallSequence();
	UAnimMontage* GetAirDeathLandMontage() const;
	void TriggerPendingDeathPresentation();
	void StartFakeDeathRevive();
	void FinishFakeDeathRevive();
	void UpdateDeathState(float DeltaTime);
	void StartFakeDeathSound();
	void StopFakeDeathSound(bool bReleaseBGMDuck = true);
	void StartRealDeathSound();
	void StopRealDeathSound();
	void ApplyDeathControlLock();
	void ClearDeathControlLock();
	void NotifyBossPlayerFakeDeathStarted();
	void NotifyBossPlayerRevivedFromFakeDeath();
	void NotifyBossPlayerRealDeathStarted();
	void ResetBossesAfterPlayerRealDeath();
	void RespawnAtPlayerStart();
	void ClearHitReactRuntimeStateForRevive();
	void ScheduleFakeDeathReviveAutoLockOn();
	void UpdateFakeDeathReviveAutoLockOn(float DeltaTime);
	void ClearFakeDeathReviveAutoLockOn();
	void StartLockOnAfterFakeDeathRevive(class AJunCharacter* NewTarget);

	UFUNCTION()
	void OnFakeDeathGetUpMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Drink
	bool CanStartDrinkPotion() const;
	void RefillDrinkPotionCharges();
	void ApplyDrinkPotionHeal();
	void UpdateDrinkPotionHeal(float DeltaTime);
	void CancelDrinkPotionHeal();
	void HideWeaponForDrinkPotion();
	void FinishDrinkPotion(bool bInterrupted);

	UFUNCTION()
	void OnDrinkPotionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Hit
	EJunPlayerHitResolveResult ResolveIncomingHitResult(EHitReactType IncomingHitType, const AActor* DamageCauser) const;
	bool CanBeInterruptedBy(EHitReactType IncomingHitType) const;
	bool IsDamageCauserInDefenseAngle(const AActor* DamageCauser) const;
	ECharacterHitReactDirection DetermineHitReactDirection(const AActor* DamageCauser, const FVector& SwingDirection) const;
	ECharacterHitReactDirection DetermineHitReactDirection(
		const AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseRuleData& DefenseRuleData) const;
	ECharacterKnockbackDirection DetermineKnockbackDirectionFromDamageCauser(const AActor* DamageCauser) const;
	UAnimMontage* GetHitReactMontage(EHitReactType HitType, ECharacterHitReactDirection HitDirection) const;
	EJunAirHitReactType ResolveAirHitReactType() const;
	UAnimMontage* GetAirHitReactMontage(EJunAirHitReactType AirHitReactType) const;
	UAnimMontage* GetParrySuccessMontage(const FVector& SwingDirection);
	UAnimMontage* GetAirParrySuccessMontage();
	UAnimMontage* GetMikiriParryMontage(const AActor* DamageCauser) const;
	UAnimMontage* GetMikiriParryReadyMontage(const AActor* ReferenceActor) const;
	AActor* GetMikiriCounterReferenceActor() const;
	bool AddPosture(float Amount);
	void ReducePosture(float Amount);
	bool IsPostureFull() const;
	void UpdatePostureRecovery(float DeltaTime);
	bool CanRecoverPosture() const;
	void StartParrySuccess();
	bool TryHandleMikiriCounter(AActor* DamageCauser);
	void StartMikiriCounterSuccess(AActor* DamageCauser);
	void UpdateMikiriCounterWindow(float DeltaTime);
	void FinishMikiriCounterReady();
	void CancelParrySuccessForCancelTransition(float BlendOutTime);
	void ExecuteBufferedParrySuccessCancelAction();
	void TryExecuteBufferedParrySuccessCancelAction();
	void StartGuardBlock();
	void StartGuardBreak();
	void ApplyAirGuardBreakTuning();
	void StartAirGuardBreakLandMontage();
	void StartHitReact(EHitReactType HitType, ECharacterHitReactDirection HitDirection);
	void PlayHitReactMontageWithBlend(class UAnimMontage* HitReactMontage, bool bRestartingHitReact);
	void RequestPlayerHitReactHitStop();
	void UpdateAirHeavyHitReact(float DeltaTime);
	void StartAirHeavyHitDownStage();
	void StartAirHeavyHitLandStage();
	void ResetAirHeavyHitReactState();
	void ApplyAirLightHitFallTuning();
	void ApplyAirHeavyHitFallTuning();
	void RestoreAirHeavyHitFallTuning();
	void ApplyAirHitKnockback(EJunAirHitReactType AirHitReactType);
	void ApplyCommonKnockback(
		ECharacterKnockbackDirection KnockbackDirection,
		float Strength,
		float BrakingDecelerationOverride,
		float GroundFrictionOverride,
		float BrakingFrictionFactorOverride,
		float OverrideDuration
	);
	void ClearCombatInputBuffers();
	void InterruptActionsForHitReaction();
	void UpdatePlayerHitState(float DeltaTime);
	void UpdateGuardBreakVulnerability(float DeltaTime);
	bool CanUseHitReactFacingWindow() const;
	void ReleaseHitReactControlLock();
	bool TryCancelHitReactIntoMove();
	bool TryCancelHitReactIntoJump();
	void FinishPlayerHitState();

protected: // Defense
	void UpdateBasicAttackJumpAndMoveCancelState(float DeltaTime);
	void UpdateBasicAttackRecoveryBuffer(float DeltaTime);
	void UpdateDefenseInput(float DeltaTime);
	void CompleteGuardBlockReleaseIntoGuardEnd();
	void ExecuteBufferedDefenseTransitionCancelAction();
	void TryExecuteBufferedDefenseTransitionCancelAction();
	void CancelDefenseForCancelTransition(float BlendOutTime = -1.f);
	void FinishDefenseForCancel();
	void StartDefenseSequence();
	void StartAirParrySequence();
	void BeginDefenseFromBufferedInput();
	void ResolveCurrentDeflectAttempt();
	void StartGuardRestartBlend(class UAnimMontage* Montage, float TargetPosition, float BlendDuration);
	void UpdateGuardRestartBlend(float DeltaTime);
	void FinishGuardRestartBlend(bool bSnapToTarget);
	void ResetGuardRestartAnchor();
	void ApplyDefenseStartBlockTags();
	void ApplyAirDefenseStartBlockTags();
	void ApplyGuardOnBlockTags();
	void ClearDefenseBlockTags();
	void EnterGuardLoop();
	void BeginGuardEnd();
	void BeginAirGuardEnd();
	void FinishDefense();
	void OpenParryWindow(bool bCountAsParryAttempt = true);
	void ResetParrySpamPenalty();
	void DrawDefenseTimingDebug() const;
	float GetCurrentDefenseTimelineRate() const;

	UFUNCTION()
	void OnGuardStartMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnGuardEndMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnAirGuardStartMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnAirGuardEndMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnLockOnTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected: // Jump / Movement Helpers
	void ApplyRunningStateToAnimInstance(bool bNewIsRunning);
	void SetupMeshAndCollision();
	void SetupCameraComponents();
	void SetupMovementDefaults();
	void UpdateCameraAnchor(float DeltaTime);
	void CacheDefaultMovementBrakingSettings();
	void RestoreDefaultMovementBrakingSettings();
	void TryInitializeCameraRotationFromController();
	void ResetCameraToSpawnView(const FRotator& SpawnRotation);
	void SnapCameraToCurrentFreeView();
	void StartCameraHardSnap(int32 FrameCount = 2);
	void UpdateCameraHardSnap();
	void ValidateLockOnTarget();
	void UpdateMovementSpeed(float DeltaTime);
	void UpdateFreeRunRotationRate();
	void UpdateCharacterRotationForCurrentCameraMode(float DeltaTime);
	void UpdateJumpStartAnimTrigger(float DeltaTime);
	void TryStartLockOnTurn();
	void CancelLockOnTurn(float BlendOutTime = -1.f);
	bool CanStartLockOnTurn() const;
	bool IsLockOnTurnPlaying() const;
	float GetLockOnTargetYawDelta() const;
	UAnimMontage* ChooseLockOnTurnMontage(float YawDelta) const;
	float GetCurrentRunMoveSpeed() const;
	float GetLockOnMoveSpeedMultiplier() const;
	FRotator GetJumpLaunchBasisRotation() const;
	FVector BuildJumpLaunchVelocity(const FVector2D& MoveInput) const;
	FVector GetJumpLaunchDirection(const FVector2D& MoveInput) const;
	void UpdateMovementBraking(float DeltaTime);

private:
	UAnimInstance* GetPlayerAnimInstance();

	void SpawnAndAttachWeapon();
	void SpawnAndAttachScabbard();
	void SpawnAndAttachDefaultHat();
	void CacheDefenseEffectComponents();
	class UNiagaraComponent* FindNiagaraComponentByName(FName ComponentName) const;
	USceneComponent* FindSceneComponentByName(FName ComponentName) const;
	void PlayDefenseEffect(
		TObjectPtr<class UNiagaraComponent>& CachedComponent,
		FName ComponentName,
		TObjectPtr<class USceneComponent>& CachedLocationComponent,
		FName LocationComponentName);
	void PlayDefenseCameraShake(float Scale);

protected: // Target / Facing
	class AJunCharacter* FindBestLockOnTarget();
	class AJunCharacter* FindBestAttackTarget();
	void UpdateAttackFacing(float DeltaTime);
	void UpdateHitReactFacing(float DeltaTime);

protected: // Camera
	void UpdateJunCamera(float DeltaTime);
	void UpdateFreeCamera(float DeltaTime);
	void UpdateCameraFOV(float DeltaTime);
	float GetPitchAdjustedSpringArmLength() const;
	FVector GetCameraForwardOnPlane() const;

	void StartLockOn(AJunCharacter* NewTarget);
	void EndLockOn();
	bool IsLockOnTargetValid() const;

	void UpdateLockOnCamera(float DeltaTime);
	bool ShouldUseJumpCounterLockOnCameraPitch() const;
	void StartExecutionCamera(class AJunMonster* Monster);
	void EndExecutionCamera();
	void AdvanceExecutionCameraStage();
	void AdvanceExecutionFinishCameraPullInStage();
	void UpdateExecutionCamera(float DeltaTime);
	void StartDeathCamera();
	void EndDeathCamera();
	void UpdateDeathCamera(float DeltaTime);
	FVector GetCurrentExecutionCameraSocketOffset() const;
	float GetCurrentExecutionCameraFOV() const;
	float GetCurrentExecutionCameraArmLength() const;
	float GetCurrentExecutionCameraApplyArmLength() const;
	float GetCurrentExecutionCameraRotationInterpSpeed() const;
	float GetCurrentExecutionCameraArmLengthInterpSpeed() const;
	float GetCurrentExecutionCameraArmLengthRestoreInterpSpeed() const;
	float GetCurrentExecutionCameraPitchOffset() const;
	void UpdateLockOnCharacterRotation(float DeltaTime);
	FVector GetRawLockOnBonePoint() const;
	FVector GetLockOnDebugSpherePoint();
	FVector GetRawLockOnCapsulePoint() const;
	FVector GetFilteredLockOnTargetPoint();

protected: // Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USceneComponent> CameraAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<class UAudioComponent> FakeDeathAudioComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<class UAudioComponent> RealDeathAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bTestInvincible = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Defense")
	FName ParryParticleComponentName = TEXT("Parry_Particle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Defense")
	FName ParryEffectLocationComponentName = TEXT("Parry_Location");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Defense")
	FName DeflectParticleComponentName = TEXT("Deflect_Particle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Defense")
	FName DeflectEffectLocationComponentName = TEXT("Deflect_Location");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Defense")
	FName GuardBreakParticleComponentName = TEXT("Brake_Particle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Defense")
	FName GuardBreakEffectLocationComponentName = TEXT("Brake_Location");

	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> CachedParryParticleComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> CachedDeflectParticleComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> CachedGuardBreakParticleComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class USceneComponent> CachedParryEffectLocationComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class USceneComponent> CachedDeflectEffectLocationComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class USceneComponent> CachedGuardBreakEffectLocationComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Defense")
	TSubclassOf<class UCameraShakeBase> DefenseCameraShakeClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Defense", meta = (ClampMin = "0.0"))
	float PerfectParryCameraShakeScale = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Defense", meta = (ClampMin = "0.0"))
	float NormalParryCameraShakeScale = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Defense", meta = (ClampMin = "0.0"))
	float GuardHitCameraShakeScale = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Defense", meta = (ClampMin = "0.0"))
	float MikiriParryCameraShakeScale = 6.f;

protected: // External References
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class AJunCharacter> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	TObjectPtr<class AJunCharacter> LockOnTarget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class UAnimInstance> AnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	class AWeaponActor* EquippedWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	class AWeaponActor* EquippedScabbard = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Hat")
	TObjectPtr<class AHatActor> EquippedHat = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<class UAnimMontage> CurrentDodgeMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DodgeAttack")
	TObjectPtr<class UAnimMontage> CurrentDodgeAttackMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
	TObjectPtr<class UAnimMontage> CurrentLockOnTurnMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	TObjectPtr<class UAnimMontage> CurrentHeavyAttackMontage;

protected: // Runtime Camera State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	EJunCameraMode CameraMode = EJunCameraMode::Free;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector2D PendingCameraLookInput = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float TargetCameraYaw = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float TargetCameraPitch = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bCameraRotationInitialized = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	int32 CameraHardSnapRemainFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bSavedCameraLagEnabledForHardSnap = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	bool bLockOnActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	FVector CachedLockOnTargetPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	float CachedLockOnRangeAlpha = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	float SmoothedLockOnDistance2D = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	float SmoothedLockOnPitchOffset = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	bool bLockOnClosePitchModeActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	bool bLockOnFarPitchModeActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	float SmoothedLockOnOcclusionLateralOffset = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	FVector CachedLockOnAimDirection2D = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	bool bLockOnCloseAimStabilizationActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	bool bLockOnTurnInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	float FakeDeathReviveAutoLockOnRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	TWeakObjectPtr<class AJunCharacter> PendingFakeDeathReviveLockOnTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Execution")
	EJunCameraMode CameraModeBeforeExecution = EJunCameraMode::Free;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Execution")
	TObjectPtr<class AJunMonster> ExecutionCameraTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Execution")
	bool bExecutionCameraSecondStage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Execution")
	bool bExecutionCameraFinishPullInStage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Execution")
	bool bCurrentExecutionIsFinish = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Execution")
	bool bExecutionCameraRestoreUsesFinishTuning = false;

protected: // Runtime Combat / Defense State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	bool bIsBasicAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bIsHeavyAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bHeavyAttackInputHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bHeavyAttackChargeStartPlayed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bHeavyAttackChargeEndRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackInputHoldTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeStartRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeLoopElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackSectionElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	float CurrentHeavyAttackPlayRate = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bHeavyAttackChargeEndDashExecuted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	EJunHeavyAttackState HeavyAttackState = EJunHeavyAttackState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	EJunBufferedRecoveryAction BufferedHeavyAttackRecoveryAction = EJunBufferedRecoveryAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	int32 HeavyAttackComboIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bCanBufferHeavyAttackComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bBufferedHeavyAttackComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bBufferedHeavyAttackInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bHeavyAttackComboAdvanceStateActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeavyAttack")
	bool bCanRestartHeavyAttackAfterComboAdvance = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	bool bIsJigenAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	int32 JigenComboIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	bool bCanBufferJigenComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	bool bBufferedJigenComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	bool bBufferedJigenInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	bool bJigenComboAdvanceStateActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	float JigenSectionElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jigen")
	EJunBufferedRecoveryAction BufferedJigenRecoveryAction = EJunBufferedRecoveryAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpAttack")
	bool bIsJumpAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpAttack")
	bool bJumpAttackEndRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackSectionElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpAttack")
	float CurrentJumpAttackPlayRate = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpAttack")
	EJunJumpAttackState JumpAttackState = EJunJumpAttackState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterFollowUpDefenseBypassAvailable = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterStompFollowUpAvailable = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterStompFollowUpActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterStompJumpAttackWindowOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterStompInvincibilityActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterStompOpportunityPending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompCorrectionElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	FVector JumpCounterStompCorrectionStartLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	FVector JumpCounterStompCorrectionTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompBounceMoveRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	FVector JumpCounterStompBounceMoveDirection = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompBounceMoveSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	TObjectPtr<class AJunMonster> JumpCounterStompTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	TObjectPtr<class AJunMonster> JumpCounterStompPendingTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpCounter")
	TObjectPtr<class AJunMonster> JumpCounterStompIgnoredCollisionTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	int32 BasicAttackComboIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	bool bCanBufferBasicAttackComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	bool bBufferedBasicAttackComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	bool bBufferedBasicAttackInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	bool bBasicAttackComboAdvanceStateActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	bool bCanRestartBasicAttackAfterComboAdvance = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	EJunBufferedRecoveryAction BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	float PostBasicAttackJumpDodgeBufferRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	float PostBasicAttackDefenseBufferRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeFinishRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeInternalCooldownRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeInvincibleRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	float CurrentDodgePlayRate = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge|Camera")
	bool bDodgeCameraAnchorProjectionActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge|Camera")
	FVector DodgeCameraAnchorStartLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge|Camera")
	FVector DodgeCameraAnchorDirection = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge|Camera")
	float DodgeCameraAnchorProjectedDistance = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	bool bDodgeInputReleasedSinceLastDodge = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bShowDefenseTimingDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bShowDodgeCameraDebug = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	bool bDodgeChainWindowActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DodgeAttack")
	bool bDodgeAttackWindowActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DodgeAttack")
	bool bIsDodgeAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bIsExecuting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class AJunMonster> CurrentExecutionTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> CurrentExecutionMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	float SavedExecutionCapsuleRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bExecutionCapsuleRadiusReduced = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float ForwardDodgePlayRate = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashForwardPlayRate = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashBackPlayRate = 1.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashSidePlayRate = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float LeftDodgePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float RightDodgePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float ForwardDodgeFinishTime = 1.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashForwardFinishTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashBackFinishTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashSideFinishTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float SideDodgeFinishTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float ForwardDodgeInternalCooldownDuration = 1.02f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashForwardInternalCooldownDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashBackInternalCooldownDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashSideInternalCooldownDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float SideDodgeInternalCooldownDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float ForwardDodgeInvincibleDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashForwardInvincibleDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashBackInvincibleDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashSideInvincibleDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float SideDodgeInvincibleDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float DodgeChainBlendOutTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float DodgeRecoveryCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float ForwardDodgeRecoveryCancelOpenTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashForwardRecoveryCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashBackRecoveryCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashSideRecoveryCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashForwardMoveCancelOpenTime = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashSideMoveCancelOpenTime = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashBackMoveCancelOpenTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float DashMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	float LockOnSideDashRotationInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float ForwardDodgeMoveCancelOpenTime = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float SideDodgeMoveCancelOpenTime = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float SideDodgeRecoveryCancelOpenTime = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack", meta = (ClampMin = "0.1"))
	float DodgeAttackPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackMoveCancelOpenTime = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackDodgeCancelOpenTime = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackDefenseCancelOpenTime = 0.93f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackBasicAttackCancelOpenTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackHeavyAttackCancelOpenTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackMoveCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackDodgeCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackDefenseCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackBasicAttackCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	float DodgeAttackHeavyAttackCancelBlendOutTime = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	EJunDefenseState DefenseState = EJunDefenseState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	int32 GuardStartRestartSerial = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	EJunBufferedDefenseCancelAction BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bParryWindowOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float ParryWindowRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MikiriCounter")
	bool bMikiriCounterWindowOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MikiriCounter")
	float MikiriCounterWindowRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float CurrentParryWindowDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float CurrentPerfectParryWindowDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float ParrySpamPenaltyResetRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	int32 ParrySpamPenaltyStack = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float CurrentDefensePlayRate = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bDefenseButtonHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bHoldRequestedForCurrentDeflect = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bDeflectResolveReceived = false;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float DeflectResolveRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float CurrentDeflectHeldDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float DefenseTransitionElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardEndFinishRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardEndBaseReleaseRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float PostDefenseTransitionCancelBufferRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bKeepGuardBaseWhileEnding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardMoveBlendRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardExitMoveReleaseRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bDeferGuardMoveInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Air")
	bool bAirParrySequenceActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<class UAnimMontage> GuardRestartAnchorMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardRestartAnchorPosition = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bGuardRestartAnchorReached = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<class UAnimMontage> GuardRestartBlendMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardRestartBlendStartPosition = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardRestartBlendTargetPosition = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardRestartBlendElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardRestartBlendDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	float GuardRestartBlendRestorePlayRate = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bGuardRestartBlendActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	bool bGuardBlockReleasePending = false;

	FTimerHandle GuardBlockReleaseIntoGuardEndTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	EJunPlayerHitState CurrentHitState = EJunPlayerHitState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float PlayerHitStateRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Posture")
	float CurrentPosture = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Posture")
	float PostureRecoveryDelayRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Posture")
	float GuardBreakVulnerableRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	EJunPlayerDeathState DeathState = EJunPlayerDeathState::Alive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	int32 CurrentReviveCount = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	float RealDeathHoldRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	float RealDeathFullBlackHoldRemainTime = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	bool bRealDeathBlackFadeStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	bool bPendingDeathPresentationIsRealDeath = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	bool bDeathPresentationStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death")
	bool bAirDeathSequenceActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Posture")
	bool bAirGuardBreakLandMontagePending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	int32 CurrentDrinkPotionCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	bool bIsDrinkingPotion = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	bool bDrinkPotionSavedWalkRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	bool bDrinkPotionWeaponHidden = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	bool bDrinkPotionSavedWeaponHidden = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	float DrinkPotionHealRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	float DrinkPotionPendingHealAmount = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink")
	float DrinkPotionHealAccumulator = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float PlayerHitControlLockRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bTutorialControlLocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float ChainParryWindowRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessDefenseHoldElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	EHitReactType CurrentHitReactType = EHitReactType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	EHitReactType LastIncomingHitReactType = EHitReactType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	ECharacterHitReactDirection CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> CurrentHitReactMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	bool bCurrentHitReactUsesAirMontage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	EJunAirHeavyHitStage AirHeavyHitStage = EJunAirHeavyHitStage::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float AirHeavyHitStageRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	bool bAirHeavyHitFallTuningActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float DefaultAirHeavyHitGravityScale = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float DefaultAirHeavyHitFallingLateralFriction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	FVector LastIncomingSwingDirection = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	ECharacterKnockbackDirection LastIncomingKnockbackDirection = ECharacterKnockbackDirection::Backward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	FJunAttackDefenseKnockbackData LastIncomingDefenseKnockbackData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	FJunAttackDefenseRuleData LastIncomingDefenseRuleData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	EJunBufferedParrySuccessCancelAction BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	bool bParrySuccessHeavyAttackInputHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	bool bHasSelectedInitialParrySuccessSide = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	bool bNextParrySuccessUsesLeftSide = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	bool bLastParrySuccessUsedLeftSide = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float KnockbackBrakingOverrideRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float KnockbackBrakingDecelerationOverride = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float KnockbackGroundFrictionOverride = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	float KnockbackBrakingFrictionFactorOverride = 0.f;

protected: // Runtime Movement / Jump State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bWalkRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bSprintRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float TargetMaxWalkSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DesiredMoveForward = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DesiredMoveRight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float PendingMoveForward = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float PendingMoveRight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DefaultWalkingBrakingDeceleration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DefaultGroundFriction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DefaultBrakingFrictionFactor = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bFreeRunSlideActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector FreeRunSlideDirection = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float FreeRunSlideSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpStartAnimTriggerRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump")
	float LandingAnimSuppressRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	bool bAttackFacingWindowActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	float AttackFacingWindowInterpSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	bool bHitReactFacingWindowActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HitReactFacingWindowInterpSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<AActor> HitReactFacingTarget = nullptr;

protected: // Camera Tuning
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraAnchorInterpSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraAnchorDodgeInterpSpeed = 7.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Dodge")
	bool bProjectDodgeCameraAnchorToDodgeDirection = true;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SpringArmLocationInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraSocketOffsetInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DefaultSpringArmLength = 470.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float PitchArmShortenStartPitch = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float PitchArmShortenEndPitch = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float PitchShortenedSpringArmLength = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float PitchArmShortenInterpSpeed = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float FreeCameraFOV = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float LockOnCameraFOV = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float ExecutionCameraFOV = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float ExecutionFinishCameraFOV = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraFOVInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SpringArmLengthInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Free")
	FVector FreeCameraSocketOffset = FVector(0.f, 25.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float FreeCameraYawSpeed = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float FreeCameraPitchSpeed = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float FreeCameraRotationInterpSpeed = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float MinCameraPitch = -60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float MaxCameraPitch = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float SpawnCameraPitch = -15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float MovingLookInputScale = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Free")
	float AttackLookInputScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Dodge")
	FVector DodgeCameraSocketOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|LockOn")
	FVector LockOnCameraSocketOffset = FVector(0.f, 25.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|Crossing", meta = (ClampMin = "0"))
	float LockOnOcclusionAvoidDistance = 260.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|Crossing", meta = (ClampMin = "0"))
	float LockOnOcclusionLineDistance = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|Crossing", meta = (ClampMin = "0"))
	float LockOnOcclusionShoulderOffset = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|Crossing", meta = (ClampMin = "0"))
	float LockOnOcclusionOffsetInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Execution")
	FVector ExecutionCameraSocketOffset = FVector(0.f, 25.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Execution")
	FVector ExecutionFinishCameraSocketOffset = FVector(0.f, 25.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionCameraArmLength = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraArmLength = 350.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionCameraApplyArmLength = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraApplyArmLength = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraPullInArmLength = 130.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionCameraRotationInterpSpeed = 9.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraRotationInterpSpeed = 9.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionCameraArmLengthInterpSpeed = 9.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraArmLengthInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionCameraArmLengthRestoreInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraArmLengthRestoreInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionCameraPitchOffset = -10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Execution")
	float ExecutionFinishCameraPitchOffset = -10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Death")
	FVector DeathCameraSocketOffset = FVector(0.f, 25.f, 10.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Death")
	float DeathCameraArmLength = 420.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Death")
	float DeathCameraRotationInterpSpeed = 4.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Death")
	float DeathCameraPitchOffset = -12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Death")
	float DeathCameraTargetHeight = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
    float LockOnRotationInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnPitchRotationInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnCloseRotationInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnClosePitchRotationInterpSpeed = 3.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnCloseDistance = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnFarDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0", ClampMax = "1"))
	float LockOnCloseTargetBlend = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnRangeAlphaInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnDistanceInterpSpeed = 7.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnClosePitchEnterDistance = 280.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnClosePitchExitDistance = 420.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnFarPitchEnterDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnFarPitchExitDistance = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnPitchOffsetInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnCloseAimStabilizeDistance = 260.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnCloseAimStabilizeReleaseDistance = 340.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnCloseAimMaxYawDegreesPerSecond = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnCharacterRotationInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnAcquireDistance = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnBreakDistance = 150000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float FakeDeathReviveAutoLockOnDelay = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnPitchOffset = -10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnClosePitchOffset = -35.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|ExecutionReady")
	float ExecutionReadyLockOnPitchOffset = -15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|ExecutionReady", meta = (ClampMin = "0"))
	float ExecutionReadyLockOnArmLength = 360.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|ExecutionReady", meta = (ClampMin = "1"))
	float ExecutionReadyLockOnFOV = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float MinLockOnCameraPitch = -35.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float MaxLockOnCameraPitch = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|JumpCounter")
	float JumpCounterLockOnPitchOffset = -55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|JumpCounter")
	float JumpCounterMinLockOnCameraPitch = -65.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn|JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterLockOnPitchMaxDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTargetZRiseInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTargetZFallInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTargetZIgnoreThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTargetXYIgnoreThreshold = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn", meta = (ClampMin = "0"))
	float LockOnTargetXYInterpSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTurnStartAngle = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTurn180Threshold = 135.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTurnMaxGroundSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnTurnCancelBlendOutTime = 0.12f;

protected: // Attack / Defense Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Facing")
	float FreeAttackFacingAcquireDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Facing", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float FreeAttackFacingAcquireAngle = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardStartPlayRate = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardStartVisualRestartMinInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardStartRestartBlendDuration = 0.035f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Air")
	float AirGuardStartPlayRate = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Air")
	float AirGuardStartVisualRestartMinInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Air")
	float AirGuardStartRestartBlendDuration = 0.035f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Air")
	float AirGuardLandCancelBlendOutTime = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardEndPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardEndStateDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardEndBlendInTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardStartToEndBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardMoveBlendDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float DefaultParryWindowDuration = 0.3f; // 기본 패리 판정 시간

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float PerfectParryWindowDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MikiriCounterForwardInputThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterThreatSearchRadius = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterReadyMinDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterWindowDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterDodgeBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float ParrySpamPenaltyResetDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParrySpamPenaltyPerStack = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float MinParryWindowDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float MinPerfectParryWindowDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float DefenseFrontAngle = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float DefaultDeflectResolveTime = 0.156f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardEnterHoldThreshold = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardEndFinishTimeOffset = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardEndBaseReleaseTimeOffset = 0.48f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float PostDefenseTransitionCancelBufferDuration = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float DefenseCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float DefenseMoveCancelOpenTime = 0.096f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float DefenseMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardBlockReleaseBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessDuration = 0.45f; // 몽타주 길이 못 읽은 경우 사용

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ChainParryWindowDuration = 1.f; // 연속 패리 판정 가능 시간

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessBasicAttackCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessHeavyAttackCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessJumpCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessDodgeCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessMoveCancelOpenTime = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessDefenseCancelOpenTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float ParrySuccessDefenseHoldConfirmTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessBasicAttackCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessHeavyAttackCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessJumpCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessDodgeCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessDefenseCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float GuardBlockDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "1"))
	float MaxPosture = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "0"))
	float PerfectParryPostureGain = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "0"))
	float NormalParryPostureGain = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "0"))
	float PerfectAirParryPostureReduction = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "0"))
	float NormalAirParryPostureReduction = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "0"))
	float GuardBlockPostureGain = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture", meta = (ClampMin = "0"))
	float HitReactPostureGain = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture")
	float GuardBreakDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture")
	float GuardBreakControlLockDuration = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture")
	float GuardBreakDamageMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakKnockbackStrength = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakGravityScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakFallingLateralFriction = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air")
	float AirGuardBreakDownwardVelocity = -180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakAirborneMaxDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakLandingBrakingDeceleration = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakLandingGroundFriction = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakLandingBrakingFrictionFactor = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Air", meta = (ClampMin = "0"))
	float AirGuardBreakLandingSlideDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	int32 MaxReviveCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0", ClampMax = "1"))
	float FakeDeathReviveHealthRatio = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0"))
	float RealDeathUIHoldDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0"))
	float RealDeathFullBlackHoldDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	FName DeathLoopSectionName = TEXT("Death");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound")
	TObjectPtr<class USoundBase> FakeDeathSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound")
	TObjectPtr<class USoundBase> RealDeathSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound", meta = (ClampMin = "0"))
	float FakeDeathSoundVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound", meta = (ClampMin = "0"))
	float RealDeathSoundVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound", meta = (ClampMin = "0"))
	float FakeDeathSoundFadeInTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound", meta = (ClampMin = "0"))
	float FakeDeathSoundFadeOutTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound", meta = (ClampMin = "0"))
	float RealDeathSoundFadeInTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Sound", meta = (ClampMin = "0"))
	float RealDeathSoundFadeOutTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Recovery", meta = (ClampMin = "0"))
	float PostureRecoveryDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Recovery", meta = (ClampMin = "0"))
	float PostureRecoveryRate = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Recovery", meta = (ClampMin = "0"))
	float GuardPostureRecoveryMultiplier = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Posture|Recovery", meta = (ClampMin = "0"))
	float RunPostureRecoveryMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float HitReactDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float HitReactMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float HitReactBlendInTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float HitReactRestartBlendInTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LightHitReactDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LightHitControlLockDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirLightHitReactDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirLightHitControlLockDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirLightHitKnockbackStrength = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirLightHitGravityScale = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirLightHitFallingLateralFriction = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirLightHitInitialDownwardVelocity = -300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirHeavyHitReactDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirHeavyHitControlLockDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirHeavyHitKnockbackStrength = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirHeavyHitMinGroundDistance = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirDeathMinGroundDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirHeavyHitSequenceMaxDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirHeavyHitLaunchStageMaxDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirHeavyHitGravityScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float AirHeavyHitFallingLateralFriction = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirHeavyHitInitialDownwardVelocity = -300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float AirHeavyHitDownwardVelocity = -500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float HeavyHitReactDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float HeavyHitAControlLockDuration = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float HeavyHitBControlLockDuration = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float HeavyHitCControlLockDuration = 1.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LargeHitReactDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LargeHitControlLockDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LargeHitLongReactDuration = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LargeHitLongControlLockDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LightingShockDuration = 2.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float LightingShockControlLockDuration = 2.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float GuardBlockKnockbackStrength = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float GuardBlockKnockbackBrakingDeceleration = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float GuardBlockKnockbackGroundFriction = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float GuardBlockKnockbackBrakingFrictionFactor = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float GuardBlockKnockbackOverrideDuration = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessKnockbackStrength = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessKnockbackBrakingDeceleration = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessKnockbackGroundFriction = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessKnockbackBrakingFrictionFactor = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	float ParrySuccessKnockbackOverrideDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float PostBasicAttackJumpDodgeBufferDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float PostBasicAttackDefenseBufferDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeThreshold = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack", meta = (ClampMin = "0.1"))
	float HeavyAttackTapPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack", meta = (ClampMin = "0.1"))
	float HeavyAttackChargePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeLoopMaxDuration = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeFullDashThresholdRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndDashMaxSpeed = 2400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndDashMinSpeedRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndDashGroundMotionDuration = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	FName HeavyAttackChargeStartSectionName = TEXT("Start");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	FName HeavyAttackChargeLoopSectionName = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	FName HeavyAttackChargeEndSectionName = TEXT("End");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	FName HeavyAttackTapFirstSectionName = TEXT("1");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	FName HeavyAttackTapSecondSectionName = TEXT("2");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackTapDodgeCancelOpenTime = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackTapJumpCancelOpenTime = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackTapDefenseCancelOpenTime = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackTapMoveCancelOpenTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndDodgeCancelOpenTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndJumpCancelOpenTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndDefenseCancelOpenTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndMoveCancelOpenTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackTapBasicAttackCancelOpenTime = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackChargeEndBasicAttackCancelOpenTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackDodgeCancelBlendOutTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackJumpCancelBlendOutTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackDefenseCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackMoveCancelBlendOutTime = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackBasicAttackCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackHeavyAttackCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackRestartBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackComboBlendInTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	float HeavyAttackStartBlendInTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen", meta = (ClampMin = "0.1"))
	float JigenPlayRate = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	FName JigenFirstSectionName = TEXT("1");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	FName JigenSecondSectionName = TEXT("2");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenComboBlendInTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenStartBlendInTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenDodgeCancelOpenTime = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenJumpCancelOpenTime = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenDefenseCancelOpenTime = 1.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenMoveCancelOpenTime = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenBasicAttackCancelOpenTime = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenHeavyAttackCancelOpenTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenDodgeCancelBlendOutTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenJumpCancelBlendOutTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenDefenseCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenBasicAttackCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JigenHeavyAttackCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float BasicAttackJigenCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	TArray<float> BasicAttackJigenCancelOpenTimes = { 0.5f, 0.5f, 0.6f, 0.6f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float HeavyAttackTapJigenCancelOpenTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float HeavyAttackChargeEndJigenCancelOpenTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float HeavyAttackJigenCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JumpAttackEndJigenCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float JumpAttackEndJigenCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float DodgeAttackJigenCancelOpenTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float DodgeAttackJigenCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	float DodgeJigenCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack", meta = (ClampMin = "0.1"))
	float JumpAttackPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackMinStartTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackMinStartVerticalVelocity = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackMinGroundDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterEvadeMinGroundDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterStompMaxStartDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterStompOpportunityDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterStompCorrectionDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompTargetForwardOffset = -30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompTargetHeightOffset = 85.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterStompCapsuleMargin = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter")
	bool bJumpCounterStompIgnoreTargetCollisionDuringFollowUp = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0", EditCondition = "bJumpCounterStompIgnoreTargetCollisionDuringFollowUp"))
	float JumpCounterStompMinVisualDistance = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterStompPostureGain = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompBounceUpVelocity = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter")
	float JumpCounterStompBounceBackwardVelocity = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0.1"))
	float JumpCounterStompPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter", meta = (ClampMin = "0"))
	float JumpCounterStompJumpAttackCancelBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack", meta = (ClampMin = "0"))
	float JumpAttackStartForwardImpulseStrength = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack", meta = (ClampMin = "0"))
	float JumpAttackStationaryImpulseMaxHorizontalSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	FName JumpAttackStartSectionName = TEXT("Start");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	FName JumpAttackLoopSectionName = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	FName JumpAttackEndSectionName = TEXT("End");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndMoveCancelOpenTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndDodgeCancelOpenTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndDefenseCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndBasicAttackCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndHeavyAttackCancelOpenTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndDodgeCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndDefenseCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndBasicAttackCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	float JumpAttackEndHeavyAttackCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack", meta = (ClampMin = "0.1"))
	float BasicAttackPlayRate = 1.f;

	// Cancel open times are authored in montage timeline seconds at PlayRate 1.0.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	TArray<float> BasicAttackDodgeCancelOpenTimes = { 0.5f, 0.5f, 0.6f, 0.6f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	TArray<float> BasicAttackJumpCancelOpenTimes = { 0.5f, 0.5f, 0.6f, 0.6f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	TArray<float> BasicAttackDefenseCancelOpenTimes = { 0.1f, 0.1f, 0.1f, 0.1f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	TArray<float> BasicAttackMoveCancelOpenTimes = { 1.1f, 0.8f, 0.8f, 0.9f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	TArray<float> BasicAttackHeavyAttackCancelOpenTimes = { 0.5f, 0.5f, 0.6f, 0.6f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackDodgeCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackJumpCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackDefenseCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackHeavyAttackCancelBlendOutTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackRestartBlendOutTime = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BasicAttack")
	float BasicAttackSectionElapsedTime = 0.f;

	float GuardExitMoveBlendDuration = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	float GuardExitMoveReleaseDelay = 0.04f;

protected: // Movement / Jump Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float WalkMoveSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float FreeRunMoveSpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Rotation", meta = (ClampMin = "0.0"))
	float FreeRunMinTurnRotationRate = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Rotation", meta = (ClampMin = "0.0"))
	float FreeRunMaxTurnRotationRate = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Rotation", meta = (ClampMin = "0.01"))
	float FreeRunTurnAngleExponent = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float LockOnRunMoveSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float LockOnForwardMoveSpeedMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float LockOnSideMoveSpeedMultiplier = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float LockOnBackwardMoveSpeedMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintMoveSpeed = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float GuardMoveSpeed = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float MoveSpeedInterpRate = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float FreeRunStopBrakingDeceleration = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float FreeRunSlideMinStartSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0"))
	int32 MaxDrinkPotionCharges = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DrinkPotionHealRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
	float DrinkPotionHealDuration = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.001"))
	float DrinkPotionPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
	float DrinkPotionBlendInTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
	float DrinkPotionInterruptBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump")
	float LockOnJumpMinimumHorizontalSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump")
	float JumpStartAnimTriggerDuration = 0.1f;

protected: // Animation Assets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BasicAttack")
	TObjectPtr<class UAnimMontage> BasicAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	TObjectPtr<class UAnimMontage> HeavyAttackTapMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HeavyAttack")
	TObjectPtr<class UAnimMontage> HeavyAttackChargeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jigen")
	TObjectPtr<class UAnimMontage> JigenMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink")
	TObjectPtr<class UAnimMontage> DrinkMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpAttack")
	TObjectPtr<class UAnimMontage> JumpAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpCounter")
	TObjectPtr<class UAnimMontage> JumpCounterStompFollowUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|PlayerHit")
	bool bEnablePlayerHitReactHitStop = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|PlayerHit", meta = (ClampMin = "0.0"))
	float PlayerHitReactHitStopDuration = 0.035f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|PlayerHit", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float PlayerHitReactHitStopTimeScale = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|PlayerHit")
	int32 PlayerHitReactHitStopPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<class UAnimMontage> DodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<class UAnimMontage> BackDashMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	TObjectPtr<class UAnimMontage> DashForwardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	TObjectPtr<class UAnimMontage> DashLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash")
	TObjectPtr<class UAnimMontage> DashRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DodgeAttack")
	TObjectPtr<class UAnimMontage> DodgeAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> ExecutionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> ExecutionFinishMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|Sound")
	TObjectPtr<class USoundBase> ExecutionStartSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|Sound", meta = (ClampMin = "0"))
	float ExecutionStartSoundVolume = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion")
	bool bEnableExecutionStartSlowMotion = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0"))
	float ExecutionStartSlowMotionDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0.001", ClampMax = "1"))
	float ExecutionStartSlowMotionTimeScale = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0"))
	float ExecutionStartSlowMotionBlendInTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0"))
	float ExecutionStartSlowMotionBlendOutTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion")
	bool bEnableExecutionFinishStartSlowMotion = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0"))
	float ExecutionFinishStartSlowMotionDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0.001", ClampMax = "1"))
	float ExecutionFinishStartSlowMotionTimeScale = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0"))
	float ExecutionFinishStartSlowMotionBlendInTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion", meta = (ClampMin = "0"))
	float ExecutionFinishStartSlowMotionBlendOutTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution|SlowMotion")
	int32 ExecutionStartSlowMotionPriority = 40;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<class UAnimMontage> FakeDeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<class UAnimMontage> FakeDeathGetUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<class UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<class UAnimMontage> AirFakeDeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<class UAnimMontage> AirDeathMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDodgeRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDodgeBackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDodgeLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDashForwardLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDashForwardRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDashBackLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Dash|LockOn")
	TObjectPtr<class UAnimMontage> LockOnDashBackRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LockOn")
	TObjectPtr<class UAnimMontage> LockOnTurnLeft90Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LockOn")
	TObjectPtr<class UAnimMontage> LockOnTurnRight90Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LockOn")
	TObjectPtr<class UAnimMontage> LockOnTurnLeft180Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LockOn")
	TObjectPtr<class UAnimMontage> LockOnTurnRight180Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<class UAnimMontage> GuardStartMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<class UAnimMontage> GuardEndMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Air")
	TObjectPtr<class UAnimMontage> AirGuardStartMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Air")
	TObjectPtr<class UAnimMontage> AirGuardEndMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> ParrySuccessL_UpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> ParrySuccessL_DownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> ParrySuccessR_UpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit|Air")
	TObjectPtr<class UAnimMontage> AirParrySuccessL_UpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit|Air")
	TObjectPtr<class UAnimMontage> AirParrySuccessR_UpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> ParrySuccessR_DownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> ParrySuccessFrontUpLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter")
	TObjectPtr<class UAnimMontage> MikiriParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter")
	TObjectPtr<class UAnimMontage> MikiriParryRMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter")
	TObjectPtr<class UAnimMontage> MikiriParryReadyMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MikiriCounter")
	TObjectPtr<class UAnimMontage> MikiriParryReadyRMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> CurrentParrySuccessMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MikiriCounter")
	TObjectPtr<class UAnimMontage> CurrentMikiriParryReadyMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MikiriCounter")
	float MikiriCounterReadyRemainTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> GuardBlockMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> GuardBreakMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit|Air")
	TObjectPtr<class UAnimMontage> AirGuardBreakMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> HeavyHitFront_AMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> HeavyHitFront_BMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> HeavyHitFront_CMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LargeHitBackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LargeHitFrontMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LargeHitFrontLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LargeHitFrontRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LargeHitLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LargeHitRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightHitBackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightHitFront_FMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightHitFront_FLMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightHitFront_FRMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightHitLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightHitRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> AirHitFLightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> AirHitFHeavyMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> AirHitFHeavyDownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> AirHitFHeavyLandMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<class UAnimMontage> LightingShockMontage;

protected: // Weapon Assets
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeaponActor> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeaponActor> DefaultScabbardClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocketName = TEXT("Weapon_r");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName ScabbardSocketName = TEXT("ScabSocket");

protected: // Hat Assets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Hat")
	TSubclassOf<class AHatActor> DefaultHatClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Hat")
	FName HatSocketName = TEXT("HatSocket");
};
