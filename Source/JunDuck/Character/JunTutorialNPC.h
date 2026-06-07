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

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Combat")
	void SetTutorialCombatEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Tutorial")
	void SetTutorialMode(EJunTutorialNPCMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Tutorial")
	void BeginTutorialCutsceneWait(AJunPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "TutorialNPC|Tutorial")
	void StartTutorialEquip();

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

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Dialogue")
	void OnDialogueStarted(AJunPlayer* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Dialogue")
	void OnDialogueFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "TutorialNPC|Dialogue")
	void OnDialogueLineChanged(const FJunTutorialNPCDialogueLine& DialogueLine, int32 DialogueIndex);

	void PlayIdleWaitMontage();
	void SetInteractionPromptVisible(bool bVisible);
	void ApplyInteractionPromptSettings();
	void UpdateInteractionPromptBillboard();
	void UpdateTutorialHomeReturn();
	void UpdateTutorialHomeReturnMoveAxes();
	void UpdateTutorialHomeReturnFacing(float DeltaSeconds);
	void FinishTutorialHomeReturn();
	void RefreshTutorialHomeReturnGoalLocation();
	bool RequestTutorialHomeReturnMove();
	float GetTutorialHomeReturnEffectiveAcceptRadius() const;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Combat")
	bool bTutorialCombatEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	EJunTutorialNPCMode TutorialMode = EJunTutorialNPCMode::DialogueOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialHomeReturnStartDistance = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialHomeReturnAcceptRadius = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial", meta = (ClampMin = "0"))
	float TutorialHomeReturnMoveStartDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Debug")
	bool bDebugTutorialHomeReturn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Tutorial")
	bool bTutorialStarted = false;

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
	int32 CurrentDialogueIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPC|Placement")
	FTransform InitialPlacementTransform = FTransform::Identity;
};
