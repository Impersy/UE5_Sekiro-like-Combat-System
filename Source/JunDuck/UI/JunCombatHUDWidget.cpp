#include "UI/JunCombatHUDWidget.h"

#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"

void UJunCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerDelayedHealthPercent = PlayerHealthPercent;
	BossTargetHealthPercent = BossHealthPercent;
	BossDelayedHealthPercent = BossHealthPercent;
	PlayerDelayedHealthDelayRemainTime = 0.f;
	BossDelayedHealthDelayRemainTime = 0.f;

	ApplyPlayerHealthBars();
	ApplyBossHealthBars();
	ApplyPostureBars();
	ApplyBossHealthVisibility();
	ApplyBossLifeWidgets();
	if (Player_RedGlow_Root)
	{
		Player_RedGlow_Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Player_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		Player_RedGlow_Root->SetRenderOpacity(0.f);
		Player_RedGlow_Root->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Boss_RedGlow_Root)
	{
		Boss_RedGlow_Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Boss_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		Boss_RedGlow_Root->SetRenderOpacity(0.f);
		Boss_RedGlow_Root->SetVisibility(ESlateVisibility::Hidden);
	}
	ApplyWidgetOpacity(Boss_Clear, 0.f);
	HideDeathUI();
	OnBossHealthVisibilityChanged(bBossHealthVisible);
	OnPlayerDelayedHealthChanged(PlayerDelayedHealthPercent);
	OnBossDelayedHealthChanged(BossDelayedHealthPercent);
}

void UJunCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateBossHealthFill(InDeltaTime);
	UpdateDelayedHealthBars(InDeltaTime);
	UpdateRedGlowEffects(InDeltaTime);
	UpdateBossClearUI(InDeltaTime);
	UpdateDeathUI(InDeltaTime);
}

void UJunCombatHUDWidget::SetPlayerHealth(int32 CurrentHealth, int32 MaxHealth)
{
	const float PreviousHealthPercent = PlayerHealthPercent;
	PlayerHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	if (PlayerHealthPercent < PreviousHealthPercent)
	{
		PlayerDelayedHealthDelayRemainTime = DelayedHealthStartDelay;
		PlayerDelayedHealthPercent = FMath::Max(PlayerDelayedHealthPercent, PreviousHealthPercent);
	}
	else if (PlayerHealthPercent > PlayerDelayedHealthPercent)
	{
		PlayerDelayedHealthPercent = PlayerHealthPercent;
		PlayerDelayedHealthDelayRemainTime = 0.f;
		OnPlayerDelayedHealthChanged(PlayerDelayedHealthPercent);
	}

	OnPlayerHealthChanged(CurrentHealth, MaxHealth, PlayerHealthPercent);
	ApplyPlayerHealthBars();
}

void UJunCombatHUDWidget::SetBossHealth(int32 CurrentHealth, int32 MaxHealth)
{
	const float PreviousHealthPercent = BossHealthPercent;
	BossTargetHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	if (BossTargetHealthPercent < BossHealthPercent || FMath::IsNearlyEqual(BossTargetHealthPercent, BossHealthPercent, 0.001f))
	{
		BossHealthPercent = BossTargetHealthPercent;
	}

	if (BossHealthPercent < PreviousHealthPercent)
	{
		BossDelayedHealthDelayRemainTime = DelayedHealthStartDelay;
		BossDelayedHealthPercent = FMath::Max(BossDelayedHealthPercent, PreviousHealthPercent);
	}
	else if (BossTargetHealthPercent > BossHealthPercent)
	{
		BossDelayedHealthDelayRemainTime = 0.f;
		BossDelayedHealthPercent = BossHealthPercent;
		OnBossDelayedHealthChanged(BossDelayedHealthPercent);
	}
	else if (BossTargetHealthPercent > BossDelayedHealthPercent && BossHealthPercent >= BossTargetHealthPercent)
	{
		BossDelayedHealthPercent = BossHealthPercent;
		BossDelayedHealthDelayRemainTime = 0.f;
		OnBossDelayedHealthChanged(BossDelayedHealthPercent);
	}

	OnBossHealthChanged(CurrentHealth, MaxHealth, BossHealthPercent);
	ApplyBossHealthBars();
}

void UJunCombatHUDWidget::SetBossLifeState(int32 CurrentLifeCount, int32 MaxLifeCount)
{
	BossMaxLifeCount = FMath::Clamp(MaxLifeCount, 0, 3);
	BossCurrentLifeCount = FMath::Clamp(CurrentLifeCount, 0, BossMaxLifeCount);

	ApplyBossLifeWidgets();
}

void UJunCombatHUDWidget::SetPlayerPosture(float CurrentPosture, float MaxPosture)
{
	PlayerPosturePercent = MaxPosture > 0.f
		? FMath::Clamp(CurrentPosture / MaxPosture, 0.f, 1.f)
		: 0.f;

	ApplyPostureBars();
}

void UJunCombatHUDWidget::SetBossPosture(float CurrentPosture, float MaxPosture)
{
	const float PreviousBossPosturePercent = BossPosturePercent;
	BossPosturePercent = MaxPosture > 0.f
		? FMath::Clamp(CurrentPosture / MaxPosture, 0.f, 1.f)
		: 0.f;

	if (PreviousBossPosturePercent < 1.f && BossPosturePercent >= 1.f)
	{
		PlayBossPostureBreakGlow();
	}

	ApplyPostureBars();
}

void UJunCombatHUDWidget::PlayPlayerPostureBreakGlow()
{
	StartRedGlowEffect(Player_RedGlow_Root, PlayerRedGlowElapsedTime);
}

void UJunCombatHUDWidget::PlayBossPostureBreakGlow()
{
	StartRedGlowEffect(Boss_RedGlow_Root, BossRedGlowElapsedTime);
}

void UJunCombatHUDWidget::ShowBossClearUI()
{
	bBossClearUIActive = true;
	BossClearHoldRemainTime = BossClearHoldDuration;
	TargetBossClearOpacity = 1.f;
	SetBossHealthVisible(false);
	if (BossPostureRoot)
	{
		BossPostureRoot->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Boss_Clear)
	{
		Boss_Clear->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::ShowFakeDeathUI()
{
	SetDeathRootVisible(true);
	TargetDimBlackOpacity = FakeDeathDimBlackOpacity;
	TargetFakeDeathOpacity = 1.f;
	TargetRealDeathOpacity = 0.f;
	TargetFullBlackOpacity = 0.f;

	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::HideFakeDeathUI()
{
	TargetFakeDeathOpacity = 0.f;
	TargetDimBlackOpacity = 0.f;
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::ShowRealDeathUI()
{
	SetDeathRootVisible(true);
	TargetDimBlackOpacity = RealDeathDimBlackOpacity;
	TargetFakeDeathOpacity = 0.f;
	TargetRealDeathOpacity = 1.f;
	TargetFullBlackOpacity = 0.f;
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::StartFullBlackFadeIn()
{
	SetDeathRootVisible(true);
	TargetFullBlackOpacity = 1.f;
}

void UJunCombatHUDWidget::HideDeathUI()
{
	DimBlackOpacity = 0.f;
	FullBlackOpacity = 0.f;
	FakeDeathOpacity = 0.f;
	RealDeathOpacity = 0.f;
	TargetDimBlackOpacity = 0.f;
	TargetFullBlackOpacity = 0.f;
	TargetFakeDeathOpacity = 0.f;
	TargetRealDeathOpacity = 0.f;
	ApplyWidgetOpacity(DimBlack, 0.f);
	ApplyWidgetOpacity(FullBlack, 0.f);
	ApplyWidgetOpacity(Fake_Death, 0.f);
	ApplyWidgetOpacity(Real_Death, 0.f);
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	SetDeathRootVisible(false);
}

bool UJunCombatHUDWidget::IsFullBlackOpaque() const
{
	return FullBlackOpacity >= 0.99f;
}

void UJunCombatHUDWidget::SetBossHealthVisible(bool bVisible)
{
	if (bBossClearUIActive)
	{
		bVisible = false;
	}

	if (bBossHealthVisible == bVisible)
	{
		ApplyBossHealthVisibility();
		OnBossHealthVisibilityChanged(bBossHealthVisible);
		return;
	}

	bBossHealthVisible = bVisible;
	ApplyBossHealthVisibility();
	OnBossHealthVisibilityChanged(bBossHealthVisible);
}

ESlateVisibility UJunCombatHUDWidget::GetBossHealthSlateVisibility() const
{
	return bBossHealthVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
}

void UJunCombatHUDWidget::UpdateDelayedHealthBars(float DeltaTime)
{
	UpdateDelayedHealthBar(
		DeltaTime,
		PlayerHealthPercent,
		PlayerDelayedHealthPercent,
		PlayerDelayedHealthDelayRemainTime,
		&UJunCombatHUDWidget::OnPlayerDelayedHealthChanged);

	UpdateDelayedHealthBar(
		DeltaTime,
		BossHealthPercent,
		BossDelayedHealthPercent,
		BossDelayedHealthDelayRemainTime,
		&UJunCombatHUDWidget::OnBossDelayedHealthChanged);
}

void UJunCombatHUDWidget::UpdateBossHealthFill(float DeltaTime)
{
	if (BossHealthPercent >= BossTargetHealthPercent)
	{
		return;
	}

	const float PreviousHealthPercent = BossHealthPercent;
	BossHealthPercent = BossHealthFillInterpSpeed > 0.f
		? FMath::FInterpTo(BossHealthPercent, BossTargetHealthPercent, DeltaTime, BossHealthFillInterpSpeed)
		: BossTargetHealthPercent;

	if (FMath::IsNearlyEqual(BossHealthPercent, BossTargetHealthPercent, 0.001f))
	{
		BossHealthPercent = BossTargetHealthPercent;
	}

	if (!FMath::IsNearlyEqual(PreviousHealthPercent, BossHealthPercent, 0.0001f))
	{
		if (BossDelayedHealthPercent < BossHealthPercent)
		{
			BossDelayedHealthPercent = BossHealthPercent;
			OnBossDelayedHealthChanged(BossDelayedHealthPercent);
		}

		ApplyBossHealthBars();
	}
}

void UJunCombatHUDWidget::UpdateRedGlowEffects(float DeltaTime)
{
	UpdateRedGlowEffect(DeltaTime, Player_RedGlow_Root, PlayerRedGlowElapsedTime);
	UpdateRedGlowEffect(DeltaTime, Boss_RedGlow_Root, BossRedGlowElapsedTime);
}

void UJunCombatHUDWidget::UpdateRedGlowEffect(float DeltaTime, UWidget* RootWidget, float& ElapsedTime)
{
	if (!RootWidget || ElapsedTime < 0.f)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	const float Alpha = RedGlowDuration > 0.f
		? FMath::Clamp(ElapsedTime / RedGlowDuration, 0.f, 1.f)
		: 1.f;
	const float EaseAlpha = 1.f - FMath::Square(1.f - Alpha);
	const float ScaleX = FMath::Lerp(RedGlowStartScaleX, RedGlowEndScaleX, EaseAlpha);
	const float Opacity = FMath::Pow(1.f - Alpha, 0.65f);

	RootWidget->SetRenderScale(FVector2D(ScaleX, RedGlowScaleY));
	RootWidget->SetRenderOpacity(Opacity);
	RootWidget->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	if (Alpha >= 1.f)
	{
		ElapsedTime = -1.f;
		RootWidget->SetRenderOpacity(0.f);
		RootWidget->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		RootWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::StartRedGlowEffect(UWidget* RootWidget, float& ElapsedTime)
{
	if (!RootWidget)
	{
		return;
	}

	ElapsedTime = 0.f;
	RootWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	RootWidget->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
	RootWidget->SetRenderOpacity(1.f);
	RootWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UJunCombatHUDWidget::UpdateDeathUI(float DeltaTime)
{
	const auto InterpOpacity = [this, DeltaTime](float Current, float Target)
	{
		return DeathUIFadeSpeed > 0.f
			? FMath::FInterpTo(Current, Target, DeltaTime, DeathUIFadeSpeed)
			: Target;
	};

	DimBlackOpacity = InterpOpacity(DimBlackOpacity, TargetDimBlackOpacity);
	FullBlackOpacity = InterpOpacity(FullBlackOpacity, TargetFullBlackOpacity);
	FakeDeathOpacity = InterpOpacity(FakeDeathOpacity, TargetFakeDeathOpacity);
	RealDeathOpacity = InterpOpacity(RealDeathOpacity, TargetRealDeathOpacity);

	if (FMath::IsNearlyEqual(DimBlackOpacity, TargetDimBlackOpacity, 0.005f))
	{
		DimBlackOpacity = TargetDimBlackOpacity;
	}
	if (FMath::IsNearlyEqual(FullBlackOpacity, TargetFullBlackOpacity, 0.005f))
	{
		FullBlackOpacity = TargetFullBlackOpacity;
	}
	if (FMath::IsNearlyEqual(FakeDeathOpacity, TargetFakeDeathOpacity, 0.005f))
	{
		FakeDeathOpacity = TargetFakeDeathOpacity;
	}
	if (FMath::IsNearlyEqual(RealDeathOpacity, TargetRealDeathOpacity, 0.005f))
	{
		RealDeathOpacity = TargetRealDeathOpacity;
	}

	ApplyWidgetOpacity(DimBlack, DimBlackOpacity);
	ApplyWidgetOpacity(FullBlack, FullBlackOpacity);
	ApplyWidgetOpacity(Fake_Death, FakeDeathOpacity);
	ApplyWidgetOpacity(Real_Death, RealDeathOpacity);

	const bool bAnyDeathUIVisible =
		DimBlackOpacity > 0.f ||
		FullBlackOpacity > 0.f ||
		FakeDeathOpacity > 0.f ||
		RealDeathOpacity > 0.f ||
		TargetDimBlackOpacity > 0.f ||
		TargetFullBlackOpacity > 0.f ||
		TargetFakeDeathOpacity > 0.f ||
		TargetRealDeathOpacity > 0.f;
	SetDeathRootVisible(bAnyDeathUIVisible);
}

void UJunCombatHUDWidget::UpdateBossClearUI(float DeltaTime)
{
	const bool bFadingOut = TargetBossClearOpacity < BossClearOpacity;
	const float InterpSpeed = bFadingOut ? BossClearFadeOutSpeed : BossClearFadeInSpeed;
	BossClearOpacity = InterpSpeed > 0.f
		? FMath::FInterpTo(BossClearOpacity, TargetBossClearOpacity, DeltaTime, InterpSpeed)
		: TargetBossClearOpacity;

	if (FMath::IsNearlyEqual(BossClearOpacity, TargetBossClearOpacity, 0.005f))
	{
		BossClearOpacity = TargetBossClearOpacity;
	}

	if (BossClearHoldRemainTime > 0.f && BossClearOpacity >= 0.99f)
	{
		BossClearHoldRemainTime = FMath::Max(0.f, BossClearHoldRemainTime - DeltaTime);
		if (BossClearHoldRemainTime <= 0.f)
		{
			TargetBossClearOpacity = 0.f;
		}
	}

	ApplyWidgetOpacity(Boss_Clear, BossClearOpacity);
}

void UJunCombatHUDWidget::ApplyWidgetOpacity(UWidget* Widget, float Opacity, bool bHideWhenZero) const
{
	if (!Widget)
	{
		return;
	}

	Widget->SetRenderOpacity(Opacity);
	Widget->SetVisibility(!bHideWhenZero || Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UJunCombatHUDWidget::SetDeathRootVisible(bool bVisible)
{
	if (Death_Root)
	{
		Death_Root->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::UpdateDelayedHealthBar(
	float DeltaTime,
	float CurrentHealthPercent,
	float& DelayedHealthPercent,
	float& DelayRemainTime,
	void (UJunCombatHUDWidget::*OnDelayedHealthChanged)(float))
{
	if (DelayedHealthPercent <= CurrentHealthPercent)
	{
		return;
	}

	if (DelayRemainTime > 0.f)
	{
		DelayRemainTime = FMath::Max(0.f, DelayRemainTime - DeltaTime);
		return;
	}

	const float PreviousDelayedHealthPercent = DelayedHealthPercent;
	DelayedHealthPercent = FMath::FInterpTo(
		DelayedHealthPercent,
		CurrentHealthPercent,
		DeltaTime,
		DelayedHealthInterpSpeed
	);

	if (FMath::IsNearlyEqual(DelayedHealthPercent, CurrentHealthPercent, 0.001f))
	{
		DelayedHealthPercent = CurrentHealthPercent;
	}

	if (!FMath::IsNearlyEqual(PreviousDelayedHealthPercent, DelayedHealthPercent, 0.0001f))
	{
		(this->*OnDelayedHealthChanged)(DelayedHealthPercent);
		ApplyPlayerHealthBars();
		ApplyBossHealthBars();
	}
}

void UJunCombatHUDWidget::ApplyPlayerHealthBars()
{
	if (PlayerHealthBar)
	{
		PlayerHealthBar->SetPercent(PlayerHealthPercent);
	}

	if (PlayerDelayedHealthBar)
	{
		PlayerDelayedHealthBar->SetPercent(PlayerDelayedHealthPercent);
	}
}

void UJunCombatHUDWidget::ApplyBossHealthBars()
{
	if (BossHealthBar)
	{
		BossHealthBar->SetPercent(BossHealthPercent);
	}

	if (BossDelayedHealthBar)
	{
		BossDelayedHealthBar->SetPercent(BossDelayedHealthPercent);
	}
}

void UJunCombatHUDWidget::ApplyPostureBars()
{
	ApplyPostureFill(BossFillCenter, BossPosturePercent, BossPostureFillMaxWidth);
	ApplyPostureFill(PlayerFillCenter, PlayerPosturePercent, PlayerPostureFillMaxWidth);
	ApplyPostureTint(BossCenter_Image, BossPosturePercent);
	ApplyPostureTint(BossLeftCap, BossPosturePercent);
	ApplyPostureTint(BossRightCap, BossPosturePercent);
	ApplyPostureTint(PlayerCenter_Image, PlayerPosturePercent);
	ApplyPostureTint(PlayerLeftCap, PlayerPosturePercent);
	ApplyPostureTint(PlayerRightCap, PlayerPosturePercent);
}

void UJunCombatHUDWidget::ApplyPostureFill(USizeBox* FillCenter, float Percent, float MaxWidth) const
{
	if (!FillCenter)
	{
		return;
	}

	FillCenter->SetWidthOverride(FMath::Max(0.f, MaxWidth * FMath::Clamp(Percent, 0.f, 1.f)));
	FillCenter->SetHeightOverride(PostureFillHeight);
}

void UJunCombatHUDWidget::ApplyPostureTint(UImage* FillImage, float Percent) const
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

void UJunCombatHUDWidget::ApplyBossHealthVisibility()
{
	if (BossHealthRoot)
	{
		BossHealthRoot->SetVisibility(GetBossHealthSlateVisibility());
	}

	if (BossPostureRoot)
	{
		BossPostureRoot->SetVisibility(GetBossHealthSlateVisibility());
	}
}

void UJunCombatHUDWidget::ApplyBossLifeWidgets()
{
	ApplyBossLifeWidget(1, Life_1_Root, Life_1_Gray, Life_1_Red);
	ApplyBossLifeWidget(2, Life_2_Root, Life_2_Gray, Life_2_Red);
	ApplyBossLifeWidget(3, Life_3_Root, Life_3_Gray, Life_3_Red);
}

void UJunCombatHUDWidget::ApplyBossLifeWidget(int32 LifeIndex, UWidget* RootWidget, UWidget* GrayWidget, UWidget* RedWidget) const
{
	const bool bActiveLifeSlot = bBossHealthVisible && LifeIndex <= BossMaxLifeCount;
	const bool bLifeRemaining = LifeIndex <= BossCurrentLifeCount;

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
