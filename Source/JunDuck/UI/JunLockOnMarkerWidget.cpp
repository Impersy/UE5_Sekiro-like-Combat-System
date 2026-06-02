#include "UI/JunLockOnMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

void UJunLockOnMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyLockOnMarkerVisibility();
	ApplyExecutionReadyMarkerAlpha();
	OnLockOnMarkerVisibilityChanged(bLockOnMarkerVisible);
	OnExecutionReadyMarkerAlphaChanged(ExecutionReadyMarkerAlpha);
}

void UJunLockOnMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float NewAlpha = FMath::FInterpTo(
		ExecutionReadyMarkerAlpha,
		TargetExecutionReadyMarkerAlpha,
		InDeltaTime,
		ExecutionReadyFadeInterpSpeed
	);

	if (!FMath::IsNearlyEqual(NewAlpha, ExecutionReadyMarkerAlpha, 0.001f))
	{
		ExecutionReadyMarkerAlpha = NewAlpha;
		ApplyExecutionReadyMarkerAlpha();
		OnExecutionReadyMarkerAlphaChanged(ExecutionReadyMarkerAlpha);
	}
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

void UJunLockOnMarkerWidget::SetExecutionReadyMarkerVisible(bool bVisible)
{
	TargetExecutionReadyMarkerAlpha = bVisible ? 1.f : 0.f;
}

ESlateVisibility UJunLockOnMarkerWidget::GetLockOnMarkerSlateVisibility() const
{
	return bLockOnMarkerVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
}

void UJunLockOnMarkerWidget::ApplyLockOnMarkerVisibility()
{
	ApplyLockOnMarkerVisualVisibility();

	const bool bExecutionReadyMarkerVisible =
		ExecutionReadyMarkerAlpha > 0.01f ||
		TargetExecutionReadyMarkerAlpha > 0.f;
	SetVisibility(
		(bLockOnMarkerVisible || bExecutionReadyMarkerVisible)
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Hidden);
}

void UJunLockOnMarkerWidget::ApplyLockOnMarkerVisualVisibility()
{
	const ESlateVisibility TargetVisibility = GetLockOnMarkerSlateVisibility();

	if (!LockOnMarkerRoot)
	{
		LockOnMarkerRoot = ResolveOptionalWidget(TEXT("LockOnMarkerRoot"));
	}
	if (!LockOnMarker)
	{
		LockOnMarker = ResolveOptionalWidget(TEXT("LockOnMarker"));
	}

	if (LockOnMarkerRoot)
	{
		LockOnMarkerRoot->SetVisibility(TargetVisibility);
	}
	if (LockOnMarker && LockOnMarker != LockOnMarkerRoot)
	{
		LockOnMarker->SetVisibility(TargetVisibility);
	}
}

UWidget* UJunLockOnMarkerWidget::ResolveOptionalWidget(FName WidgetName) const
{
	return WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
}

void UJunLockOnMarkerWidget::ApplyExecutionReadyMarkerAlpha()
{
	const bool bShouldShow = ExecutionReadyMarkerAlpha > 0.01f || TargetExecutionReadyMarkerAlpha > 0.f;
	ApplyLockOnMarkerVisibility();
	if (ExecutionReadyRoot)
	{
		ExecutionReadyRoot->SetRenderOpacity(ExecutionReadyMarkerAlpha);
		ExecutionReadyRoot->SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
