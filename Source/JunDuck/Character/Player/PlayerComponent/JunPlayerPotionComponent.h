#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JunPlayerPotionComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JUNDUCK_API UJunPlayerPotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunPlayerPotionComponent();

	void RefillCharges(int32 MaxCharges);
	bool HasCharges() const { return CurrentCharges > 0; }
	bool ConsumeCharge();

	int32 GetCurrentCharges() const { return CurrentCharges; }
	bool IsDrinking() const { return bDrinking; }
	bool GetSavedWalkRequested() const { return bSavedWalkRequested; }
	void BeginDrink(bool bCurrentWalkRequested);
	bool EndDrink(bool& bOutSavedWalkRequested);

	bool StartHeal(int32& InOutHp, int32 MaxHp, float HealRatio, float HealDuration);
	bool UpdateHeal(float DeltaTime, int32& InOutHp, int32 MaxHp);
	void CancelHeal();
	bool HasPendingHeal() const { return HealRemainTime > 0.f || PendingHealAmount > 0.f || HealAccumulator > 0.f; }

protected:
	void ResetHealState();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
	int32 CurrentCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
	bool bDrinking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
	bool bSavedWalkRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
	float HealRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
	float PendingHealAmount = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
	float HealAccumulator = 0.f;
};
