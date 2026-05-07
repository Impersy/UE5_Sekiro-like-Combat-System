#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunCombatHUDWidget.generated.h"

class UWidget;
class UProgressBar;
class USizeBox;
class UImage;

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

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetPlayerPosture(float CurrentPosture, float MaxPosture);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossPosture(float CurrentPosture, float MaxPosture);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void PlayPlayerPostureBreakGlow();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void PlayBossPostureBreakGlow();

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	float GetPlayerHealthPercent() const { return PlayerHealthPercent; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	float GetBossHealthPercent() const { return BossHealthPercent; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	float GetPlayerPosturePercent() const { return PlayerPosturePercent; }

	UFUNCTION(BlueprintPure, Category = "CombatHUD")
	float GetBossPosturePercent() const { return BossPosturePercent; }

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
	void UpdateRedGlowEffects(float DeltaTime);
	void UpdateRedGlowEffect(float DeltaTime, UWidget* RootWidget, float& ElapsedTime);
	void StartRedGlowEffect(UWidget* RootWidget, float& ElapsedTime);
	void ApplyPlayerHealthBars();
	void ApplyBossHealthBars();
	void ApplyPostureBars();
	void ApplyPostureFill(USizeBox* FillCenter, float Percent, float MaxWidth) const;
	void ApplyPostureTint(UImage* FillImage, float Percent) const;
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
	float PlayerPosturePercent = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatHUD", meta = (AllowPrivateAccess = "true"))
	float BossPosturePercent = 0.f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossPostureFillMaxWidth = 540.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PlayerPostureFillMaxWidth = 321.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PostureFillHeight = 8.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float PostureWarningTintThreshold = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true"))
	FLinearColor PostureNormalTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true"))
	FLinearColor PostureWarningTint = FLinearColor(1.f, 0.529412f, 0.364706f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float RedGlowDuration = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowStartScaleX = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowEndScaleX = 2.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowScaleY = 1.f;

	float PlayerDelayedHealthDelayRemainTime = 0.f;
	float BossDelayedHealthDelayRemainTime = 0.f;
	float PlayerRedGlowElapsedTime = -1.f;
	float BossRedGlowElapsedTime = -1.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossHealthRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossPostureRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerDelayedHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossDelayedHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> BossFillCenter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> PlayerFillCenter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BossCenter_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BossLeftCap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BossRightCap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PlayerCenter_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PlayerLeftCap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PlayerRightCap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Boss_RedGlow_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Player_RedGlow_Root;

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
