#include "UI/JunCombatHUDWidget.h"

#include "Components/Widget.h"

void UJunCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BossHealthRoot)
	{
		BossHealthRoot->SetVisibility(GetBossHealthSlateVisibility());
	}
	OnBossHealthVisibilityChanged(bBossHealthVisible);
}

void UJunCombatHUDWidget::SetPlayerHealth(int32 CurrentHealth, int32 MaxHealth)
{
	PlayerHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	OnPlayerHealthChanged(CurrentHealth, MaxHealth, PlayerHealthPercent);
}

void UJunCombatHUDWidget::SetBossHealth(int32 CurrentHealth, int32 MaxHealth)
{
	BossHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	OnBossHealthChanged(CurrentHealth, MaxHealth, BossHealthPercent);
}

void UJunCombatHUDWidget::SetBossHealthVisible(bool bVisible)
{
	if (bBossHealthVisible == bVisible)
	{
		if (BossHealthRoot)
		{
			BossHealthRoot->SetVisibility(GetBossHealthSlateVisibility());
		}
		OnBossHealthVisibilityChanged(bBossHealthVisible);
		return;
	}

	bBossHealthVisible = bVisible;
	if (BossHealthRoot)
	{
		BossHealthRoot->SetVisibility(GetBossHealthSlateVisibility());
	}
	OnBossHealthVisibilityChanged(bBossHealthVisible);
}

ESlateVisibility UJunCombatHUDWidget::GetBossHealthSlateVisibility() const
{
	return bBossHealthVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
}
