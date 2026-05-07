

#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "JunMonster.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EMonsterState : uint8
{
	CutsceneWait,
	Idle,
	Patrol,
	Chase,
	Return,
	BattleStart,
	Combat,
	Dead
};

UENUM(BlueprintType)
enum class EMonsterMoveState : uint8
{
	Walk,
	Run,
	Guard
};

USTRUCT(BlueprintType)
struct FMonsterAIData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float PatrolRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float PatrolWaitTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float PatrolAcceptRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float DetectRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float LoseTargetRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float ChaseMoveAcceptanceRadius = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float BattleStartRange = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterAIData")
	float CombatBreakRange = 3500.f;
};

USTRUCT(BlueprintType)
struct FMonsterAttackSelection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<class UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	bool bFaceTargetDuringAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float FacingDuration = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float FacingInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	bool bTryTurnAfterAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float PostAttackTurnStartAngle = 55.f;
};

UCLASS()
class JUNDUCK_API AJunMonster : public AJunCharacter
{
	GENERATED_BODY()
	friend class AJunPlayer;

public:
	// Engine / external entry points
	AJunMonster();
	virtual void Tick(float DeltaTime) override;

	// Called from AI/path following or weapon/damage systems.
	void HandlePatrolMoveCompleted(bool bSuccess);
	void ReceiveHit(EHitReactType HitType, float DamageAmount, AActor* DamageCauser);
	void ReceiveHit(EHitReactType HitType, float DamageAmount, AActor* DamageCauser, const FVector& SwingDirection);
	void ReceiveHit(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData);
	virtual void OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker) override;
	virtual void NotifyAttackParriedBy(class AJunPlayer* Parrier);
	bool IsExecutionReady() const;
	bool CanBeExecutedBy(const class AJunPlayer* Player) const;
	bool TryBeginExecutionBy(class AJunPlayer* Player);
	void TriggerPendingExecutionMontage();
	void CancelPendingExecution();
	void RestoreExecutionCapsuleCollisionIgnore();
	void RestoreExecutionCapsuleRadius();
	bool ShouldIgnoreExecutorCapsuleCollisionDuringExecution() const { return bIgnoreExecutorCapsuleCollisionDuringExecution; }
	bool HasExecutionResultApplied() const;
	void HandleEquipWeaponNotify();

	// Animation or external gameplay code queries these states.
	bool HasCombatTarget();
	bool IsGuardOn() const;
	bool IsInCombat();
	bool IsAttacking();
	bool IsRunning() const;
	bool ShouldUseRunLocomotion() const;
	EMonsterState GetCurrentState() const;
	EMonsterMoveState GetMoveState() const;
	int32 GetCurrentLifeCount() const;
	int32 GetMaxLifeCount() const;
	bool IsFinalExecution() const;
	float GetCurrentPosture() const;
	float GetMaxPosture() const;
	FVector2D GetCombatMoveInput() const;
	float GetDesiredMoveForward() const;
	float GetDesiredMoveRight() const;
	float GetDesiredMaxWalkSpeed() const;
	void SetDesiredMoveAxes(float NewForward, float NewRight);

	virtual void BeginAttackTraceWindow(
		EHitReactType HitReactType = EHitReactType::LightHit,
		const FJunAttackDamageData& DamageData = FJunAttackDamageData(),
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData = FJunAttackDefenseKnockbackData()) override;
	virtual void EndAttackTraceWindow() override;
	virtual void BeginKickAttackTraceWindow(
		EHitReactType HitReactType = EHitReactType::LightHit,
		const FJunAttackDamageData& DamageData = FJunAttackDamageData(),
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData = FJunAttackDefenseKnockbackData()) override;
	virtual void EndKickAttackTraceWindow() override;
	void BeginAttackFacingWindow(float InterpSpeed);
	void EndAttackFacingWindow();

protected:
	// Spawn-time setup: team, weapon, home position and initial top-level state.
	virtual void BeginPlay() override;

protected:
	// Top-level state machine.
	// Enter*() configures the state once, Update*() advances it every Tick.
	void SetMonsterState(EMonsterState NewState);
	void EnterCutsceneWaitState();
	void EnterIdleState();
	void EnterPatrolState();
	void EnterChaseState();
	void EnterBattleStartState();
	void EnterReturnState();
	virtual void EnterCombatState();
	void EnterDeadState();

	void Update_State(float DeltaTime);
	void UpdateCutsceneWait(float DeltaTime);
	void UpdateIdle(float DeltaTime);
	void UpdatePatrol(float DeltaTime);
	void UpdateChase(float DeltaTime);
	void UpdateBattleStart(float DeltaTime);
	void UpdateReturn(float DeltaTime);
	virtual void UpdateCombat(float DeltaTime);
	void UpdateBattleStartMovementBlend(float DeltaTime);
	void UpdateCombatFacing(float DeltaTime);
	void TryStartCombatTurn();
	bool TryStartTurnTowardsTargetThenState(EMonsterState NextState);
	bool CanStartGenericTurnTowardsTarget() const;
	bool CanStartCombatTurn() const;
	bool IsCombatTurnPlaying() const;
	float GetCombatTargetYawDelta() const;
	UAnimMontage* ChooseCombatTurnMontage(float YawDelta) const;
	void CancelCombatTurn(float BlendOutTime = 0.05f);
	void UpdateAttack(float DeltaTime);
	void FinishAttack();
	virtual FMonsterAttackSelection ChooseNextAttackSelection() const;
	virtual bool TryStartPostAttackTurn();
	virtual void OnAttackTick(float DeltaTime);
	UAnimMontage* GetCurrentAttackMontage() const;
	const FMonsterAttackSelection& GetCurrentAttackSelection() const;
	bool TryStartCombatTurnWithThreshold(float MinimumTurnAngle);
	virtual FString GetMonsterDebugExtraText() const;

protected:
	// AI / target helpers used by the state machine.
	void TryFindTarget();
	bool CanDetectTarget(AActor* Target) const;
	bool CanKeepTarget(AActor* Target) const;
	bool CanRemainInCombat(AActor* Target) const;
	bool CanAttackTarget() const;
	void ChooseNextPatrolLocation();
	void MoveToLocation(const FVector& Dest, float AcceptanceRadius = -1.f);
	void MoveToTarget(AActor* Target);
	void StartChase(AActor* NewTarget);
	void StopAIMovement();
	void TryAttack();
	void ClearCurrentTarget();
	bool TryHandleDeadCurrentTarget(EMonsterState NextStateIfDead);
	void StopAllAttackTraces();
	void ResetCurrentAttackRuntimeState();
	bool TryResolveReachableLocationToward(const FVector& DesiredLocation, FVector& OutReachableLocation) const;
	float GetEffectiveReturnReachedDistance() const;
	bool HasReachedReturnTarget() const;
	void ResetCombatTurnState();

protected:
	// Weapon lifecycle helper.
	void SpawnAndAttachWeapon();
	void AttachWeaponToSocket(FName SocketName);
	void AttachWeaponToHandSocket();
	void AttachWeaponToSheathedSocket();

protected:
	// Hit react lifecycle.
	// Direction is decided from swing direction, not attacker location.
	void StartHitReact(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection);
	void UpdateHitReact(float DeltaTime);
	void EndHitReact();
	void ReleaseHitReactControlLock();
	virtual void OnHitReactStarted(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection);
	virtual void OnHitReactEnded(EHitReactType EndedHitReact);
	bool IsInHitReact() const;
	bool CanBeInterruptedBy(EHitReactType IncomingHitReact) const;
	virtual bool ShouldStartHitReact(EHitReactType IncomingHitReact) const;
	virtual bool TryHandleIncomingHitBeforeDamage(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData);
	virtual float GetHitReactDuration(EHitReactType HitType) const;
	virtual float GetHitReactControlLockDuration(EHitReactType HitType) const;
	ECharacterHitReactDirection DetermineHitReactDirection(const AActor* DamageCauser, const FVector& SwingDirection) const;
	UAnimMontage* GetHitReactMontage(EHitReactType HitType, ECharacterHitReactDirection HitDirection) const;
	bool CanUpdateBehavior() const;
	ECharacterKnockbackDirection DetermineKnockbackDirectionFromDamageCauser(const AActor* DamageCauser) const;
	void ApplyHitReactKnockback(AActor* DamageCauser, const FJunDefenseKnockbackData& KnockbackData, UAnimMontage* HitReactMontage);
	void UpdateHitReactKnockbackBraking(float DeltaTime);
	void RestoreDefaultMonsterMovementBrakingSettings();

protected:
	void AddPosture(float Amount);
	void UpdatePostureRecovery(float DeltaTime);
	bool CanRecoverPosture() const;
	float GetPostureRecoveryVitalityScale() const;
	void StartExecutionReady();
	void EndExecutionReady();
	void FinishExecutionRecovery();
	void UpdateExecutionFacing(float DeltaTime);
	void ResetExecutionRuntimeState();
	float GetExecutionMontageDuration(const UAnimMontage* Montage, float FallbackDuration) const;

	UFUNCTION()
	void OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnCutsceneWaitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected:
	// AI tuning / top-level state runtime
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	FMonsterAIData AIData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	EMonsterState CurrentState = EMonsterState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	float StateTime = 0.f; // 어떤 상태에 들어온 후 지난 시간

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float BattleStartDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float BattleStartMoveBlendDuration = 0.4f;

protected:
	// Boss intro / staging state. The montage should loop Idle -> Idle, then branch Idle -> Equip.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutscene")
	bool bStartWithCutsceneWait = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutscene")
	TObjectPtr<class UAnimMontage> CutsceneWaitMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutscene")
	FName CutsceneWaitIdleSectionName = TEXT("Idle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutscene")
	FName CutsceneWaitEquipSectionName = TEXT("Equip");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutscene", meta = (ClampMin = "0"))
	float CutsceneTriggerRange = 1300.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cutscene")
	bool bCutsceneTriggered = false;

protected:
	// Runtime references cached after spawn / detection.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class AWeaponActor> EquippedWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class AWeaponActor> EquippedScabbard = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class AWeaponActor> EquippedKickWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class AWeaponActor> EquippedKickWeaponRight = nullptr;

protected:
	// Patrol / return runtime data.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
	FVector PatrolTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Patrol")
	bool bStartWithPatrol = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Patrol")
	float IdleToPatrolDelay = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Return")
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Return")
	FVector ReturnTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Return")
	float SearchWaitTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Return")
	float ReturnAcceptRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Return")
	float ReturnReachedTolerance = 15.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Return")
	bool bIsSearching = false;

protected:
	// Combat / movement runtime flags and transient values.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDebugFreezeMovement = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsGuardOn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bAttackInterruptedByHitReact = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bCombatTurnInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bHasPendingStateAfterCombatTurn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float AttackTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float AttackFacingRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	bool bIsHasTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsRunning = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bRunLocomotionRequested = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float CombatFacingInterpSpeed = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float AttackFacingInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float CombatTurnStartAngle = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float CombatTurn180Threshold = 135.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float CombatTurnMaxGroundSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn", meta = (ClampMin = "0.01"))
	float CombatTurnPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	bool bDebugCombatTurnYaw = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	float LastCombatTurnStartYaw = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	float LastCombatTurnPostPlayYaw = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	float LastCombatTurnEndYaw = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector2D CombatMoveInput = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DesiredMoveForward = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DesiredMoveRight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float BattleStartInitialMoveForward = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float BattleStartInitialMoveRight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float BattleStartMoveBlendRemainTime = 0.f;

protected:
	// Execution / posture system shared by normal monsters and bosses.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "1"))
	int32 MaxLifeCount = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	int32 CurrentLifeCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "1"))
	float MaxPosture = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	float CurrentPosture = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	float PostureRecoveryDelayRemainTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bDisablePostureGain = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "0"))
	float PostureGainPerDamage = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "0"))
	float ParriedPostureGain = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution|PostureRecovery", meta = (ClampMin = "0"))
	float PostureRecoveryDelay = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution|PostureRecovery", meta = (ClampMin = "0"))
	float PostureRecoveryRate = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "0.1"))
	float ExecutionReadyDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "0"))
	float ExecutionInteractRange = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "0", ClampMax = "180"))
	float ExecutionFrontAngle = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bIgnoreExecutorCapsuleCollisionDuringExecution = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> ReadyExecutedMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> ExecutedMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> ExecutedDeathMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> ExecutedFinishMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bExecutionReady = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bBeingExecuted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bExecutionResultApplied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	float ExecutionReadyRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class AJunPlayer> CurrentExecutionInstigator = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	TObjectPtr<class UAnimMontage> CurrentExecutionMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	float SavedExecutionCapsuleRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bExecutionCapsuleRadiusReduced = false;

	FTimerHandle ExecutionRecoveryTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<class UAnimMontage> CurrentCombatTurnMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	EMonsterState PendingStateAfterCombatTurn = EMonsterState::Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float WalkMoveSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float RunMoveSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float GuardMoveSpeed = 180.f;

protected:
	// Test attack setup for simple "in range -> attack montage" behavior.
	UFUNCTION()
	void OnCombatTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<class UAnimMontage> AttackMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<class UAnimMontage> CurrentAttackMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	FMonsterAttackSelection CurrentAttackSelection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bAttackFacingWindowActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	float AttackFacingWindowInterpSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
	TObjectPtr<class UAnimMontage> TurnLeft90Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
	TObjectPtr<class UAnimMontage> TurnRight90Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
	TObjectPtr<class UAnimMontage> TurnLeft180Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
	TObjectPtr<class UAnimMontage> TurnRight180Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float DefaultAttackDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float DefaultAttackRange = 250.f;

protected:
	// Designer-assigned weapon setup.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeaponActor> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeaponActor> DefaultScabbardClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeaponActor> DefaultKickWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeaponActor> DefaultKickWeaponRightClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket_R");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName SheathedWeaponSocketName = TEXT("WeaponSocket_Scabbard");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName ScabbardSocketName = TEXT("ScabSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName KickWeaponSocketName = TEXT("foot_rSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName KickWeaponRightSocketName = TEXT("foot_lSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 KickWeaponTraceSampleCount = 2;

protected:
	// Hit react runtime state, timing, and animation assets.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	EHitReactType CurrentHitReact = EHitReactType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	ECharacterHitReactDirection CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HitReactTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float CurrentHitReactDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float CurrentHitReactControlLockRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HitReactKnockbackBrakingOverrideRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	bool bHitReactKnockbackBrakingOverrideActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HitReactKnockbackBrakingDecelerationOverride = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HitReactKnockbackGroundFrictionOverride = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HitReactKnockbackBrakingFrictionFactorOverride = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float DefaultMonsterBrakingDecelerationWalking = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float DefaultMonsterGroundFriction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	float DefaultMonsterBrakingFrictionFactor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact")
	float LightHitDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact")
	float HeavyHitDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact")
	float LargeHitDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact")
	float LargeHitLongDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> HeavyHitFront_AMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> HeavyHitFront_BMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> HeavyHitFront_CMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LargeHitBackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LargeHitFrontMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LargeHitFrontLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LargeHitFrontRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LargeHitLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LargeHitRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LightHitBackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LightHitFront_FMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LightHitFront_FLMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LightHitFront_FRMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LightHitLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UAnimMontage> LightHitRightMontage = nullptr;
};
