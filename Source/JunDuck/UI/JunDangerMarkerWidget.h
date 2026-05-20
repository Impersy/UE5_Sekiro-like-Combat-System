#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunDangerMarkerWidget.generated.h"

class UImage;
class UWidget;

UCLASS()
class JUNDUCK_API UJunDangerMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DangerMarker")
	void PlayDangerMarker();

	UFUNCTION(BlueprintPure, Category = "DangerMarker")
	bool IsDangerMarkerActive() const { return DangerMarkerElapsedTime >= 0.f; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateDangerMarker(float DeltaTime);
	void ApplyDangerMarkerState();
	void SetDangerRedSize(float Size);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DangerMarkerDuration = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DangerMarkerFadeInTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DangerMarkerFadeOutTime = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DangerRedStartSize = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float DangerRedEndSize = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DangerRedFadePower = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DangerMarker", meta = (AllowPrivateAccess = "true"))
	bool bUseDangerRedRenderScaleFallback = true;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Danger_Marker;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DangerMarker;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DangerRedRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> DangerRedImage;

	float DangerMarkerElapsedTime = -1.f;
	float DangerMarkerOpacity = 0.f;
};
