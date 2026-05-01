#include "UI/JunCombatHUDWidget.h"

#include "Components/ProgressBar.h"
#include "Components/Widget.h"

void UJunCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerDelayedHealthPercent = PlayerHealthPercent;
	BossDelayedHealthPercent = BossHealthPercent;
	PlayerDelayedHealthDelayRemainTime = 0.f;
	BossDelayedHealthDelayRemainTime = 0.f;

	ApplyPlayerHealthBars();
	ApplyBossHealthBars();
	ApplyBossHealthVisibility();
	OnBossHealthVisibilityChanged(bBossHealthVisible);
	OnPlayerDelayedHealthChanged(PlayerDelayedHealthPercent);
	OnBossDelayedHealthChanged(BossDelayedHealthPercent);
}

void UJunCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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
	BossHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	if (BossHealthPercent < PreviousHealthPercent)
	{
		BossDelayedHealthDelayRemainTime = DelayedHealthStartDelay;
		BossDelayedHealthPercent = FMath::Max(BossDelayedHealthPercent, PreviousHealthPercent);
	}
	else if (BossHealthPercent > BossDelayedHealthPercent)
	{
		BossDelayedHealthPercent = BossHealthPercent;
		BossDelayedHealthDelayRemainTime = 0.f;
		OnBossDelayedHealthChanged(BossDelayedHealthPercent);
	}

	OnBossHealthChanged(CurrentHealth, MaxHealth, BossHealthPercent);
	ApplyBossHealthBars();
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
