#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunCombatHUDWidget.generated.h"

class UWidget;
class UProgressBar;
class USizeBox;
class UImage;
class UTextBlock;
class UPanelWidget;
class UMediaPlayer;
class UMediaSource;

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

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Dialogue")
	void SetDialogueCombatUIHidden(bool bHidden);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void StartTutorialDimBlackFadeIn();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void StartTutorialDimBlackFadeOut();

	UFUNCTION(BlueprintPure, Category = "CombatHUD|Tutorial")
	bool IsTutorialDimBlackOpaque() const;

	UFUNCTION(BlueprintPure, Category = "CombatHUD|Tutorial")
	bool IsTutorialDimBlackHidden() const;

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void ShowPlayerPostureGuide();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void HidePlayerPostureGuide();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void ShowMonsterPostureGuide();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void ShowDangerAttackGuide();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void ShowAirParryGuide();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void HideTutorialGuides();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void ShowTutorialTask(int32 TutorialTaskType);

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Tutorial")
	void HideTutorialTask();

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
	void StartRespawnFadeOut();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Death")
	void HideDeathUI();

	UFUNCTION(BlueprintPure, Category = "CombatHUD|Death")
	bool IsFullBlackOpaque() const;

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Pause")
	void ShowPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|Pause")
	void HidePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "CombatHUD|MapNotice")
	void ShowMapNotice(int32 NoticeIndex);

	UFUNCTION(BlueprintPure, Category = "CombatHUD|MapNotice")
	bool IsMapNoticeActive() const { return ActiveMapNoticeIndex != 0 || bInitialMapNoticePending; }

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
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleTutorialGuideMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleTutorialGuideSubMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleRestartHovered();

	UFUNCTION()
	void HandleRestartUnhovered();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleQuitHovered();

	UFUNCTION()
	void HandleQuitUnhovered();

	UFUNCTION()
	void HandleQuitClicked();

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
	void UpdatePauseUI(float DeltaTime);
	void UpdateMapNoticeUI(float DeltaTime);
	void ApplyMapNoticeVisibility();
	void BeginRestartFadeToBlack();
	void BeginRestartFadeFromBlack();
	void SetPauseOptionsVisible(bool bVisible);
	void ApplyPauseOptionsOpacity() const;
	void ApplyWidgetOpacity(UWidget* Widget, float Opacity, bool bHideWhenZero = true) const;
	void SetDeathRootVisible(bool bVisible);
	void ApplyDialogueVisibility();
	void ApplyDialogueText();
	void CacheAndHideDialogueCombatWidget(UWidget* Widget);
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
	UWidget* FindTutorialTaskWidget(int32 TutorialTaskType) const;
	void SetTutorialTaskWidgetVisibility(UWidget* TaskWidget, bool bVisible) const;

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
	bool bRespawnFullBlackFadeOutActive = false;
	float BossClearOpacity = 0.f;
	float TargetBossClearOpacity = 0.f;
	float BossClearHoldRemainTime = 0.f;
	float DialogueBackgroundOpacity = 0.f;
	float TargetDialogueBackgroundOpacity = 0.f;
	float TutorialDimBlackOpacity = 0.f;
	float TargetTutorialDimBlackOpacity = 0.f;
	float PauseDimBlackOpacity = 0.f;
	float TargetPauseDimBlackOpacity = 0.f;
	float PauseOptionsOpacity = 1.f;
	float TargetPauseOptionsOpacity = 1.f;
	float RestartFullBlackHoldRemainTime = 0.f;
	float LevelIntroBlackHoldRemainTime = 0.f;
	bool bRestartFadeToBlackActive = false;
	bool bRestartFadeFromBlackActive = false;
	bool bRestartLevelRequested = false;
	int32 ActiveMapNoticeIndex = 0;
	float MapNoticeOpacity = 0.f;
	float TargetMapNoticeOpacity = 0.f;
	float MapNoticeHoldRemainTime = 0.f;
	float InitialMapNoticeDelayRemainTime = 0.f;
	bool bMapNoticeHolding = false;
	bool bInitialMapNoticePending = false;
	FText CurrentDialogueText;
	FText CurrentDialogueSpeakerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DialogueBackgroundFadeSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float DialogueBackgroundTargetOpacity = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float TutorialDimBlackFadeSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Pause", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PauseDimBlackFadeSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Pause", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float PauseDimBlackTargetOpacity = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Pause", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RestartFadeToBlackSpeed = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Pause", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RestartFadeFromBlackSpeed = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Pause", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float LevelIntroBlackHoldDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|MapNotice", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float MapNoticeFadeSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|MapNotice", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float MapNoticeHoldDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|MapNotice", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float InitialMapNoticeDelay = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Pause", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RestartFullBlackHoldDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|PlayerPostureGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaPlayer> PlayerPostureGuideMediaPlayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|PlayerPostureGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaSource> PlayerPostureGuideMediaSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|MonsterPostureGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaSource> MonsterPostureGuideMediaSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|DangerAttackGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaSource> DangerAttackGuideMediaSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|DangerAttackGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaPlayer> DangerAttackGuideSubMediaPlayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|DangerAttackGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaSource> DangerAttackGuideSubMediaSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Tutorial|AirParryGuide", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaSource> AirParryGuideMediaSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossClear", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossClearFadeInSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossClear", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossClearFadeOutSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|BossClear", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BossClearHoldDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DeathUIFadeSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RespawnFullBlackFadeOutSpeed = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float FakeDeathDimBlackOpacity = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CombatHUD|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float RealDeathDimBlackOpacity = 0.45f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossHealthRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PlayerHealthRoot;

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
	TObjectPtr<UWidget> Weapon_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Potion_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Empty_Potion_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Potion_Count;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Controls_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Controls_Toggle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Clear_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Dialog_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Dialog_Background;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Dialog_Text_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Dialog_Text_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Key_E;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Next_Text;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Tutorial_DimBlack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Tutorial_PlayerPosture_Guide;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Video_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Tutorial_MonsterPosture_Guide;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Video_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Tutorial_DangerAttack_Guide;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Video_3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Video_3_Sub;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Tutorial_AirParry_Guide;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Video_4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Tutorial_Task;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Task_BackGround;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Pause_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Pause_DimBlack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Restart_Game;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UnderLine_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Quit_Game;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UnderLine_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Map_Notice_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Map_Notice_UnderLine;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Notice_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Notice_2;

	TMap<TWeakObjectPtr<UWidget>, ESlateVisibility> DialogueCombatWidgetVisibilities;
	bool bDialogueCombatUIHidden = false;
};
