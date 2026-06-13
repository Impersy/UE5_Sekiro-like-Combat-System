#pragma once

#include "CoreMinimal.h"
#include "Character/JunMonster.h"
#include "Components/WidgetComponent.h"
#include "JunTutorialNPC.generated.h"

class USphereComponent;
class UWidgetComponent;
class AJunTutorialNPCPlacementPoint;

UENUM(BlueprintType)
enum class EJunTutorialNPCMode : uint8
{
	DialogueOnly,
	PassiveDummy,
	AttackTraining,
	ExecutionTraining
};

UENUM(BlueprintType)
enum class EJunTutorialTaskType : uint8
{
	LockOn,
	BasicAttackComboFinalHit,
	JumpAttackHit,
	DashAttackHit,
	HeavyAttackComboHit,
	HeavyChargeHit,
	JigenSecondHit,
	DrinkPotion,
	GuardPostureRecovery,
	ParryThreeTimes,
	AirParryOnce,
	MikiriCounter,
	JumpCounterStomp,
	Execution
};

USTRUCT(BlueprintType)
struct FJunTutorialNPCDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = "true"))
	FText DialogueText;
};

UCLASS()
class JUNDUCK_API AJunTutorialNPC : public AJunMonster
{
	GENERATED_BODY()

public:
	AJunTutorialNPC();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Dialogue")
	bool CanStartDialogue(const AJunPlayer* Player) const;

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Dialogue")
	bool TryStartDialogue(AJunPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Dialogue")
	void FinishDialogue();

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	bool IsPlayerInDialogueRange() const { return bPlayerInDialogueRange; }

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	bool IsDialogueActive() const { return bDialogueActive; }

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	bool HasDialogue() const { return Dialogues.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	int32 GetCurrentDialogueIndex() const { return CurrentDialogueIndex; }

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	bool HasCurrentDialogueLine() const;

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	FJunTutorialNPCDialogueLine GetCurrentDialogueLine() const;

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Dialogue")
	void ResetDialogue();

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Dialogue")
	bool AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Dialogue")
	bool StartTutorialEndDialogue(AJunPlayer* Player);

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Dialogue")
	bool IsTutorialEndDialogueActive() const { return bTutorialEndDialogueActive; }

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Combat")
	void SetTutorialCombatEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Tutorial")
	void SetTutorialMode(EJunTutorialNPCMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Tutorial")
	void BeginTutorialCutsceneWait(AJunPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Tutorial")
	void StartTutorialEquip();

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Task")
	void StartTutorialTasks(AJunPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Task")
	void CompleteCurrentTutorialTask();

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Task")
	EJunTutorialTaskType GetCurrentTutorialTask() const;

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Task")
	int32 GetCurrentTutorialTaskIndex() const { return CurrentTutorialTaskIndex; }

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Task")
	bool IsTutorialTaskCompleted() const { return bTutorialTasksCompleted; }

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Task")
	bool IsTutorialInProgress() const { return bTutorialStarted && !bTutorialTasksCompleted; }

	void NotifyTutorialLockOn(AJunPlayer* Player);
	void NotifyTutorialAttackHit(AJunPlayer* Player, EJunTutorialAttackType AttackType);
	void NotifyTutorialDrinkPotion(AJunPlayer* Player);
	void NotifyTutorialParrySuccess(AJunPlayer* Player, bool bAirParry);
	void NotifyTutorialMikiriSuccess(AJunPlayer* Player);
	void NotifyTutorialJumpCounterStompSuccess(AJunPlayer* Player);
	void NotifyTutorialExecutionStarted(AJunPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Placement")
	bool MoveToTutorialPlacementPoint(AJunTutorialNPCPlacementPoint* PlacementPoint);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Placement")
	void ReturnToInitialPlacement();

	UFUNCTION(BlueprintPure, Category = "TutorialNPC|Placement")
	FTransform GetInitialPlacementTransform() const { return InitialPlacementTransform; }

	bool IsDebugTutorialHomeReturnEnabled() const { return bDebugTutorialHomeReturn; }

	virtual bool CanBeLockOnTarget() const override;
	virtual bool ShouldShowBossCombatHUD() const override;
	virtual bool IsInCombat() override;
	virtual bool IsFinalExecution() const override;
	virtual bool TryBeginExecutionBy(class AJunPlayer* Player) override;
	virtual void TriggerPendingExecutionMontage(bool bApplyResultImmediately = true) override;
	virtual void ApplyPendingExecutionResult() override;
	virtual void OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker) override;
	virtual void NotifyAttackParriedBy(
		class AJunPlayer* Parrier,
		float PostureScale = 1.f,
		const FJunAttackDefenseRuleData& DefenseRuleData = FJunAttackDefenseRuleData()) override;
	virtual bool NotifyMikiriCounteredBy(class AJunPlayer* CounterPlayer) override;

protected:
	virtual void BeginPlay() override;
	virtual bool CanUpdateBehavior() const override;
	virtual void EnterCombatState() override;
	virtual void UpdateCombat(float DeltaTime) override;
	virtual bool TryHandleIncomingHitBeforeDamage(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		const FJunAttackDefenseRuleData& DefenseRuleData) override;
	virtual void AddPosture(float Amount) override;

	UFUNCTION()
	void OnDialogueRangeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnDialogueRangeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void OnTutorialExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Dialogue")
	void OnDialogueStarted(AJunPlayer* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Dialogue")
	void OnDialogueFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Dialogue")
	void OnDialogueLineChanged(const FJunTutorialNPCDialogueLine& DialogueLine, int32 DialogueIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Task")
	void OnTutorialTaskChanged(EJunTutorialTaskType NewTask, int32 TaskIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Task")
	void OnTutorialCompleted();

	void PlayIdleWaitMontage();
	void SetInteractionPromptVisible(bool bVisible);
	void ApplyInteractionPromptSettings();
	void UpdateInteractionPromptBillboard();
	void UpdateTutorialHomeReturn();
	void UpdateTutorialTask(float DeltaSeconds);
	void DrawTutorialTaskDebug() const;
	void ApplyTutorialTaskMode();
	bool UpdateTutorialTrainingAttackRange(float DeltaSeconds);
	void StopTutorialTrainingAttackRangeAdjustment();
	bool ShouldSuspendTutorialHomeReturnForAttackTraining() const;
	void BeginTutorialTaskTransition(int32 NextTaskIndex);
	void BeginTutorialTaskTransitionHomeReturn();
	void FinishTutorialTaskTransition();
	void UpdateTutorialTaskTransition(float DeltaSeconds);
	void TryStartTutorialTrainingAttack(float DeltaSeconds);
	void StartTutorialTrainingAttack(UAnimMontage* Montage, float PlayRate);
	UAnimMontage* GetTutorialAttackMontageForCurrentTask() const;
	bool IsCurrentTutorialTask(EJunTutorialTaskType TaskType) const;
	void ResetTutorialTaskRuntime();
	void RestoreTutorialNPCHealth();
	void RestoreTutorialNPCHealthImmediately();
	void UpdateTutorialNPCHealthRestore(float DeltaSeconds);
	void StartTutorialNPCPostureDrainToZero();
	void UpdateTutorialNPCPostureDrain(float DeltaSeconds);
	void ClampTutorialPostureForCurrentMode();
	void UpdateTutorialHomeReturnMoveAxes();
	void UpdateTutorialHomeReturnFacing(float DeltaSeconds);
	void FinishTutorialHomeReturn();
	void RefreshTutorialHomeReturnGoalLocation();
	bool RequestTutorialHomeReturnMove();
	float GetTutorialHomeReturnEffectiveAcceptRadius() const;
	float GetTutorialTaskTransitionWaitDuration() const;
	bool CanUpdateTutorialHomeReturn() const;
	bool IsExecutionTrainingEnabled() const { return TutorialMode == EJunTutorialNPCMode::ExecutionTraining; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	TObjectPtr<USphereComponent> DialogueRangeSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	TObjectPtr<UWidgetComponent> InteractionPromptWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	EWidgetSpace InteractionPromptSpace = EWidgetSpace::Screen;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	FVector2D InteractionPromptDrawSize = FVector2D(360.f, 80.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	bool bBillboardWorldInteractionPrompt = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	TObjectPtr<UAnimMontage> IdleWaitMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TutorialNPC|Dialogue", meta = (ClampMin = "0.1"))
	float DialogueRangeRadius = 220.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	bool bHideWeaponWhileWaitingForDialogue = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	TArray<FJunTutorialNPCDialogueLine> Dialogues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	TArray<FJunTutorialNPCDialogueLine> TutorialEndDialogues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Combat")
	bool bTutorialCombatEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	EJunTutorialNPCMode TutorialMode = EJunTutorialNPCMode::DialogueOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	TArray<EJunTutorialTaskType> TutorialTasks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Drink", meta = (ClampMin = "1"))
	int32 TutorialDrinkTaskTargetHp = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Drink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TutorialDrinkTaskTargetHpRatio = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Posture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TutorialGuardPostureRecoveryStartRatio = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Posture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TutorialGuardPostureRecoveryCompleteRatio = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Posture")
	bool bTutorialGuardPostureRecoveryRequiresGuard = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Health", meta = (ClampMin = "0.0"))
	float TutorialHealthRestoreDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Posture", meta = (ClampMin = "0.0"))
	float TutorialPostureDrainToZeroDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Execution", meta = (ClampMin = "1.0"))
	float TutorialExecutionMaxPosture = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack")
	TObjectPtr<UAnimMontage> TutorialParryTrainingAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack")
	TObjectPtr<UAnimMontage> TutorialAirParryTrainingAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack")
	TObjectPtr<UAnimMontage> TutorialMikiriTrainingAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack")
	TObjectPtr<UAnimMontage> TutorialJumpCounterTrainingAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack")
	TObjectPtr<UAnimMontage> TutorialMikiriCounteredMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack", meta = (ClampMin = "0.0"))
	float TutorialMikiriCounteredBlendOutTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack", meta = (ClampMin = "0.0"))
	float TutorialMikiriCounteredBlendInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack", meta = (ClampMin = "0.001"))
	float TutorialMikiriCounteredPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack", meta = (ClampMin = "0.0"))
	float TutorialMikiriCounterPostureGain = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack", meta = (ClampMin = "0.0"))
	float TutorialTrainingAttackCooldown = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack", meta = (ClampMin = "0.001"))
	float TutorialTrainingAttackPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack|Range")
	bool bUseTutorialTrainingAttackRange = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack|Range", meta = (ClampMin = "0.0"))
	float TutorialTrainingAttackMinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack|Range", meta = (ClampMin = "0.0"))
	float TutorialTrainingAttackMaxRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Attack|Range", meta = (ClampMin = "0.0"))
	float TutorialTrainingAttackRangeTolerance = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialHomeReturnStartDistance = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialHomeReturnAcceptRadius = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialDummyHomeReturnMoveStartDelay = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialAttackHomeReturnMoveStartDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task", meta = (ClampMin = "0"))
	float TutorialTaskTransitionWaitDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task|Drink", meta = (ClampMin = "0"))
	float TutorialDrinkTaskTransitionWaitDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Debug")
	bool bDebugTutorialHomeReturn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Debug")
	bool bDebugTutorialTaskProgress = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	bool bTutorialStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	int32 CurrentTutorialTaskIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	bool bTutorialTasksCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	bool bTutorialTaskTransitionPending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	bool bTutorialTaskTransitionWaitingAtHome = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	bool bTutorialTaskTransitionWaitedForHitReact = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	EJunTutorialNPCMode TutorialTaskTransitionSourceMode = EJunTutorialNPCMode::DialogueOnly;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	int32 PendingTutorialTaskIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	float TutorialTaskTransitionWaitRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	int32 TutorialHeavyHitMask = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	int32 TutorialParrySuccessCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	float TutorialTrainingAttackCooldownRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Task")
	bool bTutorialTrainingAttackRangeAdjusting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Health")
	float TutorialHealthRestoreRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Health")
	float TutorialHealthRestorePendingAmount = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Health")
	float TutorialHealthRestoreAccumulator = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Posture")
	float TutorialPostureDrainRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Posture")
	float TutorialPostureDrainStartValue = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	FVector TutorialHomeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	FVector TutorialHomeReturnGoalLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	bool bTutorialReturningHome = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	bool bTutorialHomeMoveRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	float TutorialHomeReturnMoveDelayRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	TObjectPtr<AJunPlayer> NearbyPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	bool bPlayerInDialogueRange = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	bool bDialogueActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	bool bTutorialEndDialogueActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Dialogue")
	int32 CurrentDialogueIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Placement")
	FTransform InitialPlacementTransform = FTransform::Identity;
};
