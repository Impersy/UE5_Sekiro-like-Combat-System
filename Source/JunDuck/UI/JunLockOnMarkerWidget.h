#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JunLockOnMarkerWidget.generated.h"

UCLASS()
class JUNDUCK_API UJunLockOnMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LockOnMarker")
	void SetLockOnMarkerVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "ExecutionMarker")
	void SetExecutionReadyMarkerVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "LockOnMarker")
	bool IsLockOnMarkerVisible() const { return bLockOnMarkerVisible; }

	UFUNCTION(BlueprintPure, Category = "LockOnMarker")
	ESlateVisibility GetLockOnMarkerSlateVisibility() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "LockOnMarker")
	void OnLockOnMarkerVisibilityChanged(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "ExecutionMarker")
	void OnExecutionReadyMarkerAlphaChanged(float Alpha);

	void ApplyLockOnMarkerVisibility();
	void ApplyExecutionReadyMarkerAlpha();

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> ExecutionReadyRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOnMarker", meta = (AllowPrivateAccess = "true"))
	bool bLockOnMarkerVisible = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ExecutionMarker", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float ExecutionReadyFadeInterpSpeed = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ExecutionMarker", meta = (AllowPrivateAccess = "true"))
	float ExecutionReadyMarkerAlpha = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ExecutionMarker", meta = (AllowPrivateAccess = "true"))
	float TargetExecutionReadyMarkerAlpha = 0.f;
};
