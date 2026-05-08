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
	void ShowBossClearUI();
	void ShowFakeDeathUI();
	void HideFakeDeathUI();
	void ShowRealDeathUI();
	void StartDeathFullBlackFadeIn();
	void HideDeathUI();
	bool IsDeathFullBlackOpaque() const;
	void SetLockOnMarkerSuppressed(bool bSuppressed);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void InitializeCombatWidgets();
	void UpdateCombatWidgets();
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

protected:
	TObjectPtr<class AJunPlayer> JunPlayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UJunCombatHUDWidget> CombatHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UJunLockOnMarkerWidget> LockOnMarkerWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UJunCombatHUDWidget> CombatHUDWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UJunLockOnMarkerWidget> LockOnMarkerWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|LockOn")
	float LockOnMarkerShowDelay = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|LockOn")
	TObjectPtr<class AJunCharacter> PreviousLockOnMarkerTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|LockOn")
	float LockOnMarkerShowDelayRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|LockOn")
	bool bLockOnMarkerSuppressed = false;
};
