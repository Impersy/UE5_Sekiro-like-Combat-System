#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunMonsterHUDWidget.generated.h"

class UImage;
class UProgressBar;
class USizeBox;
class UWidget;

UCLASS()
class JUNDUCK_API UJunMonsterHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MonsterHUD")
	void SetMonsterHUDVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "MonsterHUD")
	void SetMonsterHealth(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "MonsterHUD")
	void SetMonsterPosture(float CurrentPosture, float MaxPosture);

	UFUNCTION(BlueprintCallable, Category = "MonsterHUD")
	void SetMonsterLifeState(int32 CurrentLifeCount, int32 MaxLifeCount);

	UFUNCTION(BlueprintCallable, Category = "MonsterHUD")
	void PlayMonsterPostureBreakGlow();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateHealthFill(float DeltaTime);
	void UpdateDelayedHealth(float DeltaTime);
	void UpdateRedGlow(float DeltaTime);
	void ApplyHealthBars();
	void ApplyPostureBar();
	void ApplyPostureTint(UImage* FillImage, float Percent) const;
	void ApplyVisibility();
	void ApplyLifeWidgets();
	void ApplyLifeWidget(int32 LifeIndex, UWidget* RootWidget, UWidget* GrayWidget, UWidget* RedWidget) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	bool bMonsterHUDVisible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	float MonsterHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	float MonsterTargetHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	float MonsterDelayedHealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	float MonsterPosturePercent = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	int32 MonsterCurrentLifeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterHUD", meta = (AllowPrivateAccess = "true"))
	int32 MonsterMaxLifeCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float HealthFillInterpSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DelayedHealthStartDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DelayedHealthInterpSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PostureFillMaxWidth = 127.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float PostureFillHeight = 7.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Posture", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float PostureWarningTintThreshold = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Posture", meta = (AllowPrivateAccess = "true"))
	FLinearColor PostureNormalTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|Posture", meta = (AllowPrivateAccess = "true"))
	FLinearColor PostureWarningTint = FLinearColor(1.f, 0.529412f, 0.364706f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float RedGlowDuration = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowStartScaleX = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowEndScaleX = 2.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterHUD|PostureGlow", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float RedGlowScaleY = 1.f;

	float DelayedHealthDelayRemainTime = 0.f;
	float RedGlowElapsedTime = -1.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Monster_HUD;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MonsterHealthRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MonsterDelayedHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MonsterHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MonsterPostureRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MonsterPosture_Frame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MonsterHorizontalBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MonsterLeftCap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> MonsterFillCenter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MonsterCenter_Image;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MonsterRightCap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MonsterCenter_White_Line;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Monster_RedGlow_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Monster_RedGlow;

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
