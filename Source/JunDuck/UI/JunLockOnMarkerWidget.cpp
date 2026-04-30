#include "UI/JunLockOnMarkerWidget.h"

#include "Components/Widget.h"

void UJunLockOnMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyLockOnMarkerVisibility();
	OnLockOnMarkerVisibilityChanged(bLockOnMarkerVisible);
}

void UJunLockOnMarkerWidget::SetLockOnMarkerVisible(bool bVisible)
{
	if (bLockOnMarkerVisible == bVisible)
	{
		ApplyLockOnMarkerVisibility();
		return;
	}

	bLockOnMarkerVisible = bVisible;
	ApplyLockOnMarkerVisibility();
	OnLockOnMarkerVisibilityChanged(bLockOnMarkerVisible);
}

ESlateVisibility UJunLockOnMarkerWidget::GetLockOnMarkerSlateVisibility() const
{
	return bLockOnMarkerVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
}

void UJunLockOnMarkerWidget::ApplyLockOnMarkerVisibility()
{
	SetVisibility(GetLockOnMarkerSlateVisibility());
}
