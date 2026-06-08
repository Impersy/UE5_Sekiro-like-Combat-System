#include "UI/JunMonsterHUDWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"

void UJunMonsterHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MonsterTargetHealthPercent = MonsterHealthPercent;
	MonsterDelayedHealthPercent = MonsterHealthPercent;
	DelayedHealthDelayRemainTime = 0.f;

	ApplyHealthBars();
	ApplyPostureBar();
	ApplyLifeWidgets();
	if (Monster_RedGlow_Root)
	{
		Monster_RedGlow_Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Monster_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		Monster_RedGlow_Root->SetRenderOpacity(1.f);
		Monster_RedGlow_Root->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Monster_RedGlow)
	{
		Monster_RedGlow->SetRenderOpacity(0.f);
		Monster_RedGlow->SetVisibility(ESlateVisibility::Hidden);
	}
	ApplyVisibility();
}

void UJunMonsterHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateHealthFill(InDeltaTime);
	UpdateDelayedHealth(InDeltaTime);
	UpdateRedGlow(InDeltaTime);
}

void UJunMonsterHUDWidget::SetMonsterHUDVisible(bool bVisible)
{
	if (bMonsterHUDVisible == bVisible)
	{
		ApplyVisibility();
		return;
	}

	bMonsterHUDVisible = bVisible;
	ApplyVisibility();
}

void UJunMonsterHUDWidget::SetMonsterHealth(int32 CurrentHealth, int32 MaxHealth)
{
	const float PreviousHealthPercent = MonsterHealthPercent;
	MonsterTargetHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	if (MonsterTargetHealthPercent < MonsterHealthPercent ||
		FMath::IsNearlyEqual(MonsterTargetHealthPercent, MonsterHealthPercent, 0.001f))
	{
		MonsterHealthPercent = MonsterTargetHealthPercent;
	}

	if (MonsterHealthPercent < PreviousHealthPercent)
	{
		DelayedHealthDelayRemainTime = DelayedHealthStartDelay;
		MonsterDelayedHealthPercent = FMath::Max(MonsterDelayedHealthPercent, PreviousHealthPercent);
	}
	else if (MonsterTargetHealthPercent > MonsterHealthPercent)
	{
		DelayedHealthDelayRemainTime = 0.f;
		MonsterDelayedHealthPercent = MonsterHealthPercent;
	}
	else if (MonsterTargetHealthPercent > MonsterDelayedHealthPercent && MonsterHealthPercent >= MonsterTargetHealthPercent)
	{
		MonsterDelayedHealthPercent = MonsterHealthPercent;
		DelayedHealthDelayRemainTime = 0.f;
	}

	ApplyHealthBars();
}

void UJunMonsterHUDWidget::SetMonsterPosture(float CurrentPosture, float MaxPosture)
{
	const float PreviousPosturePercent = MonsterPosturePercent;
	MonsterPosturePercent = MaxPosture > 0.f
		? FMath::Clamp(CurrentPosture / MaxPosture, 0.f, 1.f)
		: 0.f;

	if (PreviousPosturePercent < 1.f && MonsterPosturePercent >= 1.f)
	{
		PlayMonsterPostureBreakGlow();
	}

	ApplyPostureBar();
}

void UJunMonsterHUDWidget::SetMonsterLifeState(int32 CurrentLifeCount, int32 MaxLifeCount)
{
	MonsterMaxLifeCount = FMath::Clamp(MaxLifeCount, 0, 3);
	MonsterCurrentLifeCount = FMath::Clamp(CurrentLifeCount, 0, MonsterMaxLifeCount);
	ApplyLifeWidgets();
}

void UJunMonsterHUDWidget::PlayMonsterPostureBreakGlow()
{
	if (!Monster_RedGlow_Root && !Monster_RedGlow)
	{
		return;
	}

	RedGlowElapsedTime = 0.f;
	if (Monster_RedGlow_Root)
	{
		Monster_RedGlow_Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Monster_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		Monster_RedGlow_Root->SetRenderOpacity(1.f);
		Monster_RedGlow_Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Monster_RedGlow)
	{
		Monster_RedGlow->SetRenderOpacity(1.f);
		Monster_RedGlow->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunMonsterHUDWidget::UpdateHealthFill(float DeltaTime)
{
	if (MonsterHealthPercent >= MonsterTargetHealthPercent)
	{
		return;
	}

	MonsterHealthPercent = HealthFillInterpSpeed > 0.f
		? FMath::FInterpTo(MonsterHealthPercent, MonsterTargetHealthPercent, DeltaTime, HealthFillInterpSpeed)
		: MonsterTargetHealthPercent;

	if (FMath::IsNearlyEqual(MonsterHealthPercent, MonsterTargetHealthPercent, 0.001f))
	{
		MonsterHealthPercent = MonsterTargetHealthPercent;
	}

	if (MonsterDelayedHealthPercent < MonsterHealthPercent)
	{
		MonsterDelayedHealthPercent = MonsterHealthPercent;
	}

	ApplyHealthBars();
}

void UJunMonsterHUDWidget::UpdateDelayedHealth(float DeltaTime)
{
	if (MonsterDelayedHealthPercent <= MonsterHealthPercent)
	{
		return;
	}

	if (DelayedHealthDelayRemainTime > 0.f)
	{
		DelayedHealthDelayRemainTime = FMath::Max(0.f, DelayedHealthDelayRemainTime - DeltaTime);
		return;
	}

	MonsterDelayedHealthPercent = DelayedHealthInterpSpeed > 0.f
		? FMath::FInterpTo(MonsterDelayedHealthPercent, MonsterHealthPercent, DeltaTime, DelayedHealthInterpSpeed)
		: MonsterHealthPercent;

	if (FMath::IsNearlyEqual(MonsterDelayedHealthPercent, MonsterHealthPercent, 0.001f))
	{
		MonsterDelayedHealthPercent = MonsterHealthPercent;
	}

	ApplyHealthBars();
}

void UJunMonsterHUDWidget::UpdateRedGlow(float DeltaTime)
{
	if ((!Monster_RedGlow_Root && !Monster_RedGlow) || RedGlowElapsedTime < 0.f)
	{
		return;
	}

	RedGlowElapsedTime += DeltaTime;
	const float Alpha = RedGlowDuration > 0.f
		? FMath::Clamp(RedGlowElapsedTime / RedGlowDuration, 0.f, 1.f)
		: 1.f;
	const float EaseAlpha = 1.f - FMath::Square(1.f - Alpha);
	const float ScaleX = FMath::Lerp(RedGlowStartScaleX, RedGlowEndScaleX, EaseAlpha);
	const float Opacity = FMath::Pow(1.f - Alpha, 0.65f);

	const ESlateVisibility GlowVisibility = Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (Monster_RedGlow_Root)
	{
		Monster_RedGlow_Root->SetRenderScale(FVector2D(ScaleX, RedGlowScaleY));
		Monster_RedGlow_Root->SetRenderOpacity(1.f);
		Monster_RedGlow_Root->SetVisibility(GlowVisibility);
	}
	if (Monster_RedGlow)
	{
		Monster_RedGlow->SetRenderOpacity(Opacity);
		Monster_RedGlow->SetVisibility(GlowVisibility);
	}

	if (Alpha >= 1.f)
	{
		RedGlowElapsedTime = -1.f;
		if (Monster_RedGlow_Root)
		{
			Monster_RedGlow_Root->SetRenderOpacity(1.f);
			Monster_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
			Monster_RedGlow_Root->SetVisibility(ESlateVisibility::Hidden);
		}
		if (Monster_RedGlow)
		{
			Monster_RedGlow->SetRenderOpacity(0.f);
			Monster_RedGlow->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UJunMonsterHUDWidget::ApplyHealthBars()
{
	if (MonsterHealthBar)
	{
		MonsterHealthBar->SetPercent(MonsterHealthPercent);
	}

	if (MonsterDelayedHealthBar)
	{
		MonsterDelayedHealthBar->SetPercent(MonsterDelayedHealthPercent);
	}
}

void UJunMonsterHUDWidget::ApplyPostureBar()
{
	const ESlateVisibility TargetVisibility = bMonsterHUDVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (MonsterPostureRoot)
	{
		MonsterPostureRoot->SetVisibility(TargetVisibility);
	}
	if (MonsterPosture_Frame)
	{
		MonsterPosture_Frame->SetVisibility(TargetVisibility);
	}
	if (MonsterHorizontalBox)
	{
		MonsterHorizontalBox->SetVisibility(TargetVisibility);
	}
	if (MonsterLeftCap)
	{
		MonsterLeftCap->SetVisibility(TargetVisibility);
	}
	if (MonsterRightCap)
	{
		MonsterRightCap->SetVisibility(TargetVisibility);
	}
	if (MonsterCenter_White_Line)
	{
		MonsterCenter_White_Line->SetVisibility(TargetVisibility);
	}

	if (MonsterFillCenter)
	{
		MonsterFillCenter->SetWidthOverride(FMath::Max(0.f, PostureFillMaxWidth * MonsterPosturePercent));
		MonsterFillCenter->SetHeightOverride(PostureFillHeight);
		MonsterFillCenter->SetVisibility(TargetVisibility);
	}

	if (MonsterCenter_Image)
	{
		MonsterCenter_Image->SetVisibility(TargetVisibility);
		ApplyPostureTint(MonsterCenter_Image, MonsterPosturePercent);
	}
	ApplyPostureTint(MonsterLeftCap, MonsterPosturePercent);
	ApplyPostureTint(MonsterRightCap, MonsterPosturePercent);
}

void UJunMonsterHUDWidget::ApplyPostureTint(UImage* FillImage, float Percent) const
{
	if (!FillImage)
	{
		return;
	}

	const FLinearColor TargetTint = Percent >= PostureWarningTintThreshold
		? PostureWarningTint
		: PostureNormalTint;
	FillImage->SetColorAndOpacity(TargetTint);
}

void UJunMonsterHUDWidget::ApplyVisibility()
{
	const ESlateVisibility TargetVisibility = bMonsterHUDVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (Monster_HUD)
	{
		Monster_HUD->SetVisibility(TargetVisibility);
	}
	if (MonsterHealthRoot)
	{
		MonsterHealthRoot->SetVisibility(TargetVisibility);
	}

	ApplyPostureBar();
	ApplyLifeWidgets();
}

void UJunMonsterHUDWidget::ApplyLifeWidgets()
{
	ApplyLifeWidget(1, Life_1_Root, Life_1_Gray, Life_1_Red);
	ApplyLifeWidget(2, Life_2_Root, Life_2_Gray, Life_2_Red);
	ApplyLifeWidget(3, Life_3_Root, Life_3_Gray, Life_3_Red);
}

void UJunMonsterHUDWidget::ApplyLifeWidget(int32 LifeIndex, UWidget* RootWidget, UWidget* GrayWidget, UWidget* RedWidget) const
{
	const bool bActiveLifeSlot = bMonsterHUDVisible && LifeIndex <= MonsterMaxLifeCount;
	const bool bLifeRemaining = LifeIndex <= MonsterCurrentLifeCount;

	if (RootWidget)
	{
		RootWidget->SetVisibility(bActiveLifeSlot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (RedWidget)
	{
		RedWidget->SetVisibility(bActiveLifeSlot && bLifeRemaining ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (GrayWidget)
	{
		GrayWidget->SetVisibility(bActiveLifeSlot && !bLifeRemaining ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
