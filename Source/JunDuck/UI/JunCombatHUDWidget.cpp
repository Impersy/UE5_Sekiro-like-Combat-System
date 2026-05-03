#include "UI/JunCombatHUDWidget.h"

#include "Components/ProgressBar.h"
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
	ApplyBossHealthVisibility();
	ApplyBossLifeWidgets();
	OnBossHealthVisibilityChanged(bBossHealthVisible);
	OnPlayerDelayedHealthChanged(PlayerDelayedHealthPercent);
	OnBossDelayedHealthChanged(BossDelayedHealthPercent);
}

void UJunCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateBossHealthFill(InDeltaTime);
	UpdateDelayedHealthBars(InDeltaTime);
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

void UJunCombatHUDWidget::SetBossHealthVisible(bool bVisible)
{
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

void UJunCombatHUDWidget::ApplyBossHealthVisibility()
{
	if (BossHealthRoot)
	{
		BossHealthRoot->SetVisibility(GetBossHealthSlateVisibility());
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
	const bool bActiveLifeSlot = LifeIndex <= BossMaxLifeCount;
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
