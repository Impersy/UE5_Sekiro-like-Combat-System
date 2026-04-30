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

	UFUNCTION(BlueprintPure, Category = "LockOnMarker")
	bool IsLockOnMarkerVisible() const { return bLockOnMarkerVisible; }

	UFUNCTION(BlueprintPure, Category = "LockOnMarker")
	ESlateVisibility GetLockOnMarkerSlateVisibility() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "LockOnMarker")
	void OnLockOnMarkerVisibilityChanged(bool bVisible);

	void ApplyLockOnMarkerVisibility();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOnMarker", meta = (AllowPrivateAccess = "true"))
	bool bLockOnMarkerVisible = false;
};
