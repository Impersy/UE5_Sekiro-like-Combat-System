#include "UI/JunDangerMarkerWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"

void UJunDangerMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	DangerMarkerElapsedTime = -1.f;
	DangerMarkerOpacity = 0.f;
	ApplyDangerMarkerState();
	SetVisibility(ESlateVisibility::Hidden);
}

void UJunDangerMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateDangerMarker(InDeltaTime);
}

void UJunDangerMarkerWidget::PlayDangerMarker()
{
	DangerMarkerElapsedTime = 0.f;
	DangerMarkerOpacity = 0.f;
	SetRenderOpacity(1.f);
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (Danger_Marker)
	{
		Danger_Marker->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Danger_Marker->SetRenderOpacity(1.f);
		Danger_Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (DangerMarker)
	{
		DangerMarker->SetRenderOpacity(0.f);
		DangerMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (DangerRedRoot)
	{
		DangerRedRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DangerRedRoot->SetRenderScale(FVector2D(1.f, 1.f));
		DangerRedRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	SetDangerRedSize(DangerRedStartSize);
	if (DangerRedImage)
	{
		DangerRedImage->SetRenderOpacity(1.f);
		DangerRedImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunDangerMarkerWidget::UpdateDangerMarker(float DeltaTime)
{
	if (DangerMarkerElapsedTime < 0.f)
	{
		return;
	}

	DangerMarkerElapsedTime += DeltaTime;
	const float Duration = FMath::Max(DangerMarkerDuration, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(DangerMarkerElapsedTime / Duration, 0.f, 1.f);

	if (DangerMarkerElapsedTime < DangerMarkerFadeInTime)
	{
		DangerMarkerOpacity = DangerMarkerFadeInTime > 0.f
			? FMath::Clamp(DangerMarkerElapsedTime / DangerMarkerFadeInTime, 0.f, 1.f)
			: 1.f;
	}
	else if (DangerMarkerElapsedTime > Duration - DangerMarkerFadeOutTime)
	{
		const float FadeOutElapsed = DangerMarkerElapsedTime - FMath::Max(0.f, Duration - DangerMarkerFadeOutTime);
		DangerMarkerOpacity = DangerMarkerFadeOutTime > 0.f
			? 1.f - FMath::Clamp(FadeOutElapsed / DangerMarkerFadeOutTime, 0.f, 1.f)
			: 0.f;
	}
	else
	{
		DangerMarkerOpacity = 1.f;
	}

	const float EaseAlpha = 1.f - FMath::Square(1.f - Alpha);
	const float RedSize = FMath::Lerp(DangerRedStartSize, DangerRedEndSize, EaseAlpha);
	const float RedOpacity = FMath::Pow(1.f - Alpha, DangerRedFadePower);

	if (DangerRedRoot)
	{
		DangerRedRoot->SetVisibility(RedOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (DangerRedImage)
	{
		DangerRedImage->SetRenderOpacity(RedOpacity);
		DangerRedImage->SetVisibility(RedOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	SetDangerRedSize(RedSize);
	ApplyDangerMarkerState();

	if (Alpha >= 1.f)
	{
		DangerMarkerElapsedTime = -1.f;
		DangerMarkerOpacity = 0.f;
		ApplyDangerMarkerState();
	}
}

void UJunDangerMarkerWidget::ApplyDangerMarkerState()
{
	if (Danger_Marker)
	{
		Danger_Marker->SetRenderOpacity(1.f);
		Danger_Marker->SetVisibility(DangerMarkerElapsedTime >= 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (DangerMarker)
	{
		DangerMarker->SetRenderOpacity(DangerMarkerOpacity);
		DangerMarker->SetVisibility(DangerMarkerOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	else if (!Danger_Marker)
	{
		SetRenderOpacity(DangerMarkerOpacity);
	}

	if (DangerMarkerElapsedTime < 0.f)
	{
		if (DangerRedRoot)
		{
			DangerRedRoot->SetRenderScale(FVector2D(1.f, 1.f));
			DangerRedRoot->SetVisibility(ESlateVisibility::Hidden);
		}
		if (DangerRedImage)
		{
			DangerRedImage->SetRenderOpacity(0.f);
			DangerRedImage->SetVisibility(ESlateVisibility::Hidden);
		}
		if (DangerMarker)
		{
			DangerMarker->SetRenderOpacity(0.f);
			DangerMarker->SetVisibility(ESlateVisibility::Hidden);
		}
		SetDangerRedSize(DangerRedStartSize);
		SetRenderOpacity(0.f);
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunDangerMarkerWidget::SetDangerRedSize(float Size)
{
	if (!DangerRedRoot)
	{
		return;
	}

	const FVector2D ResolvedSize(Size, Size);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DangerRedRoot->Slot))
	{
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetSize(ResolvedSize);
		return;
	}

	if (USizeBox* SizeBox = Cast<USizeBox>(DangerRedRoot.Get()))
	{
		SizeBox->SetWidthOverride(Size);
		SizeBox->SetHeightOverride(Size);
		return;
	}

	if (bUseDangerRedRenderScaleFallback)
	{
		const float BaseSize = FMath::Max(DangerRedStartSize, KINDA_SMALL_NUMBER);
		const float Scale = FMath::Max(0.f, Size / BaseSize);
		DangerRedRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DangerRedRoot->SetRenderScale(FVector2D(Scale, Scale));
	}
}
