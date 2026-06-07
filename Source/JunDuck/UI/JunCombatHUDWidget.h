#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunCombatHUDWidget.generated.h"

class UWidget;
class UProgressBar;
class USizeBox;
class UImage;
class UTextBlock;

UCLASS()
class JUNDUCK_API UJunCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetPlayerHealth(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetPotionCount(int32 CurrentPotionCount);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void ToggleControlsVisibility();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Dialogue")
	void ShowDialogue(const FText& DialogueText, const FText& SpeakerName = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Dialogue")
	void HideDialogue();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Dialogue")
	void SetDialogueLine(const FText& DialogueText, const FText& SpeakerName = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void StartTutorialDimBlackFadeIn();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void StartTutorialDimBlackFadeOut();

	UFUNCTION(BlueprintPure, Category = "CombatHUD|Tutorial")
	bool IsTutorialDimBlackOpaque() const;

	UFUNCTION(BlueprintPure, Category = "CombatHUD|Tutorial")
	bool IsTutorialDimBlackHidden() const;

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossHealth(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossHealthVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossLifeState(int32 CurrentLifeCount, int32 MaxLifeCount);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetPlayerPosture(float CurrentPosture, float MaxPosture);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void ResetPlayerPostureVisibilityState();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void StartPlayerPostureBreakHidePresentation();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void SetBossPosture(float CurrentPosture, float MaxPosture);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void HideBossPostureImmediately();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void PlayPlayerPostureBreakGlow();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void PlayBossPostureBreakGlow();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD")
	void ShowBossClearUI();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Death")
	void ShowFakeDeathUI();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Death")
	void HideFakeDeathUI();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Death")
	void ShowRealDeathUI();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Death")
	void StartFullBlackFadeIn();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Death")
	void HideDeathUI();

	UFUNCTION(BlueprintPure, Category = "CombatHUD|Death")
	bool IsFullBlackOpaque() const;

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
	void UpdatePostureVisibility(float DeltaTime);
	void ApplyPlayerPostureFillVisibility(bool bVisible);
	void ApplyBossPostureFillVisibility(bool bVisible);
	void UpdateBossClearUI(float DeltaTime);
	void UpdateDeathUI(float DeltaTime);
	void UpdateDialogueUI(float DeltaTime);
	void UpdateTutorialDimBlackUI(float DeltaTime);
	void ApplyWidgetOpacity(UWidget* Widget, float Opacity, bool bHideWhenZero = true) const;
	void SetDeathRootVisible(bool bVisible);
	void ApplyDialogueVisibility();
	void ApplyDialogueText();
	void ApplyPlayerHealthBars();
	void ApplyBossHealthBars();
	void ApplyPostureBars();
	void ApplyPostureFill(USizeBox* FillCenter, float Percent, float MaxWidth) const;
	void ApplyPostureTint(UImage* FillImage, float Percent) const;
	void ApplyPlayerPostureVisibility(bool bVisible);
	void ApplyBossPostureVisibility(bool bVisible);
	void ApplyBossHealthVisibility();
	void ApplyBossLifeWidgets();
	void ApplyBossLifeWidget(int32 LifeIndex, UWidget* RootWidget, UWidget* GrayWidget, UWidget* RedWidget) const;
	void ApplyPotionWidgets();
	void ApplyControlsVisibility();

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
	bool bBossClearUIActive = false;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PostureHideDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float RedGlowDuration = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowStartScaleX = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowEndScaleX = 2.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowScaleY = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PlayerPostureBreakFrameFadeOutSpeed = 4.f;

	float PlayerDelayedHealthDelayRemainTime = 0.f;
	float BossDelayedHealthDelayRemainTime = 0.f;
	float PlayerRedGlowElapsedTime = -1.f;
	float BossRedGlowElapsedTime = -1.f;
	float PlayerZeroPostureElapsedTime = 0.f;
	float BossZeroPostureElapsedTime = 0.f;
	bool bPlayerPostureEverShown = false;
	bool bBossPostureEverShown = false;
	bool bPlayerPostureBreakHidePresentationActive = false;
	int32 PotionCount = 0;
	bool bControlsVisible = true;
	float PlayerPostureFrameOpacity = 1.f;
	float DimBlackOpacity = 0.f;
	float FullBlackOpacity = 0.f;
	float FakeDeathOpacity = 0.f;
	float RealDeathOpacity = 0.f;
	float TargetDimBlackOpacity = 0.f;
	float TargetFullBlackOpacity = 0.f;
	float TargetFakeDeathOpacity = 0.f;
	float TargetRealDeathOpacity = 0.f;
	float BossClearOpacity = 0.f;
	float TargetBossClearOpacity = 0.f;
	float BossClearHoldRemainTime = 0.f;
	float DialogueBackgroundOpacity = 0.f;
	float TargetDialogueBackgroundOpacity = 0.f;
	float TutorialDimBlackOpacity = 0.f;
	float TargetTutorialDimBlackOpacity = 0.f;
	FText CurrentDialogueText;
	FText CurrentDialogueSpeakerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DialogueBackgroundFadeSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float DialogueBackgroundTargetOpacity = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float TutorialDimBlackFadeSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossClear", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossClearFadeInSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossClear", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossClearFadeOutSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossClear", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossClearHoldDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DeathUIFadeSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float FakeDeathDimBlackOpacity = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float RealDeathDimBlackOpacity = 0.45f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossHealthRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossPostureRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PlayerPostureRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PlayerPosture_Frame;

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
	TObjectPtr<UWidget> Boss_Clear;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Death_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DimBlack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> FullBlack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Real_Death;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Fake_Death;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Die_Select;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Resurrect_Select;

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Potion_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Potion_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Empty_Potion_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Potion_Count;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Controls_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Dialog_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Dialog_Background;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Dialog_Text_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Dialog_Text_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Tutorial_DimBlack;
};
