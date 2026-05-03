#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunCombatHUDWidget.generated.h"

class UWidget;
class UProgressBar;

UCLASS()
class JUNDUCK_API UJunCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetPlayerHealth(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossHealth(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossHealthVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossLifeState(int32 CurrentLifeCount, int32 MaxLifeCount);

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	float GetPlayerHealthPercent() const { return PlayerHealthPercent; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	float GetBossHealthPercent() const { return BossHealthPercent; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	int32 GetBossCurrentLifeCount() const { return BossCurrentLifeCount; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	int32 GetBossMaxLifeCount() const { return BossMaxLifeCount; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	bool IsBossHealthVisible() const { return bBossHealthVisible; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	ESlateVisibility GetBossHealthSlateVisibility() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "CombatHUD")
	void OnPlayerHealthChanged(int32 CurrentHealth, int32 MaxHealth, float HealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "CombatHUD")
	void OnPlayerDelayedHealthChanged(float DelayedHealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "CombatHUD")
	void OnBossHealthChanged(int32 CurrentHealth, int32 MaxHealth, float HealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "CombatHUD")
	void OnBossDelayedHealthChanged(float DelayedHealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "CombatHUD")
	void OnBossHealthVisibilityChanged(bool bVisible);

	void UpdateDelayedHealthBars(float DeltaTime);
	void UpdateDelayedHealthBar(
		float DeltaTime,
		float CurrentHealthPercent,
		float& DelayedHealthPercent,
		float& DelayRemainTime,
		void (UJunCombatHUDWidget::*OnDelayedHealthChanged)(float));

private:
	void UpdateBossHealthFill(float DeltaTime);
	void ApplyPlayerHealthBars();
	void ApplyBossHealthBars();
	void ApplyBossHealthVisibility();
	void ApplyBossLifeWidgets();
	void ApplyBossLifeWidget(int32 LifeIndex, UWidget* RootWidget, UWidget* GrayWidget, UWidget* RedWidget) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	float PlayerHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	float PlayerDelayedHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	float BossHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	float BossTargetHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	float BossDelayedHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	bool bBossHealthVisible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	int32 BossCurrentLifeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	int32 BossMaxLifeCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|DelayedHealth", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DelayedHealthStartDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|DelayedHealth", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DelayedHealthInterpSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossHealth", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossHealthFillInterpSpeed = 2.5f;

	float PlayerDelayedHealthDelayRemainTime = 0.f;
	float BossDelayedHealthDelayRemainTime = 0.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossHealthRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerDelayedHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossDelayedHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_1_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_1_Gray;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_1_Red;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_2_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_2_Gray;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_2_Red;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_3_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_3_Gray;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Life_3_Red;
};
