#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JunDefine.h"
#include "JunPlayerController.generated.h"


struct FInputActionValue;

/**
 * 
 */
UCLASS()
class JUNDUCK_API AJunPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJunPlayerController(const FObjectInitializer& objectInitializer);
	void PlayPlayerPostureBreakGlow();
	void PlayBossPostureBreakGlow();
	void StartPlayerPostureBreakHidePresentation();
	void HideBossPostureImmediately();
	void ShowBossClearUI();
	void ShowFakeDeathUI();
	void HideFakeDeathUI();
	void ShowRealDeathUI();
	void StartDeathFullBlackFadeIn();
	void HideDeathUI();
	bool IsDeathFullBlackOpaque() const;
	void SetLockOnMarkerSuppressed(bool bSuppressed);
	void ResetPlayerPostureVisibilityState();
	void PlayDangerMarkerOnPlayer();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void InitializeCombatWidgets();
	void UpdateCombatWidgets();
	void UpdateDangerMarkerWidget();
	class AJunMonster* FindActiveCombatBoss() const;
	void UpdateLockOnMarkerWidget(class AJunCharacter* CurrentLockOnTarget);

	void Input_Move(const FInputActionValue& InputValue);
	void Input_MoveReleased(const FInputActionValue& InputValue);
	void Input_Turn(const FInputActionValue& InputValue);
	void Input_Jump(const FInputActionValue& InputValue);
	void Input_Dodge(const FInputActionValue& InputValue);
	void Input_DodgeReleased(const FInputActionValue& InputValue);
	void Input_BasicAttack(const FInputActionValue& InputValue);
	void Input_HeavyAttackStarted(const FInputActionValue& InputValue);
	void Input_HeavyAttackReleased(const FInputActionValue& InputValue);
	void Input_LockOn(const FInputActionValue& InputValue);
	void Input_Defense(const FInputActionValue& InputValue);
	void Input_DefenseReleased(const FInputActionValue& InputValue);
	void Input_RunStarted(const FInputActionValue& InputValue);
	void Input_RunReleased(const FInputActionValue& InputValue);
	void Input_WalkToggle(const FInputActionValue& InputValue);

	bool bDodgeInputHeld = false;
	float LastDodgeInputTime = -FLT_MAX;

protected:
	TObjectPtr<class AJunPlayer> JunPlayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UJunCombatHUDWidget> CombatHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UJunLockOnMarkerWidget> LockOnMarkerWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UJunDangerMarkerWidget> DangerMarkerWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UJunCombatHUDWidget> CombatHUDWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UJunLockOnMarkerWidget> LockOnMarkerWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UJunDangerMarkerWidget> DangerMarkerWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|LockOn")
	float LockOnMarkerShowDelay = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|LockOn")
	TObjectPtr<class AJunCharacter> PreviousLockOnMarkerTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|LockOn")
	float LockOnMarkerShowDelayRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|LockOn")
	bool bLockOnMarkerSuppressed = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Danger")
	FVector DangerMarkerPlayerWorldOffset = FVector(0.f, 0.f, 120.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|MikiriCounter", meta = (ClampMin = "0.0"))
	float MikiriCounterDodgeDefenseInputGraceTime = 0.2f;
};
