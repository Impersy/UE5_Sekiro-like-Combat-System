#include "Character/Player/PlayerComponent/JunPlayerPotionComponent.h"

UJunPlayerPotionComponent::UJunPlayerPotionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJunPlayerPotionComponent::RefillCharges(int32 MaxCharges)
{
	CurrentCharges = FMath::Max(0, MaxCharges);
}

bool UJunPlayerPotionComponent::ConsumeCharge()
{
	if (CurrentCharges <= 0)
	{
		return false;
	}

	CurrentCharges = FMath::Max(0, CurrentCharges - 1);
	return true;
}

void UJunPlayerPotionComponent::BeginDrink(bool bCurrentWalkRequested)
{
	bDrinking = true;
	bSavedWalkRequested = bCurrentWalkRequested;
}

bool UJunPlayerPotionComponent::EndDrink(bool& bOutSavedWalkRequested)
{
	const bool bWasDrinking = bDrinking;
	bOutSavedWalkRequested = bSavedWalkRequested;
	bDrinking = false;
	bSavedWalkRequested = false;
	return bWasDrinking;
}

bool UJunPlayerPotionComponent::StartHeal(int32& InOutHp, int32 MaxHp, float HealRatio, float HealDuration)
{
	if (MaxHp <= 0 || HealRatio <= 0.f)
	{
		return false;
	}

	const int32 HealAmount = FMath::RoundToInt(static_cast<float>(MaxHp) * HealRatio);
	if (HealAmount <= 0)
	{
		return false;
	}

	if (HealDuration <= 0.f)
	{
		InOutHp = FMath::Clamp(InOutHp + HealAmount, 0, MaxHp);
		ResetHealState();
		return true;
	}

	PendingHealAmount += static_cast<float>(HealAmount);
	HealRemainTime = FMath::Max(HealRemainTime, HealDuration);
	return false;
}

bool UJunPlayerPotionComponent::UpdateHeal(float DeltaTime, int32& InOutHp, int32 MaxHp)
{
	if (HealRemainTime <= 0.f || PendingHealAmount <= 0.f)
	{
		return false;
	}

	if (InOutHp >= MaxHp)
	{
		ResetHealState();
		return true;
	}

	const float StepRatio = HealRemainTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(DeltaTime / HealRemainTime, 0.f, 1.f)
		: 1.f;
	const float StepHeal = PendingHealAmount * StepRatio;
	PendingHealAmount = FMath::Max(0.f, PendingHealAmount - StepHeal);
	HealRemainTime = FMath::Max(0.f, HealRemainTime - DeltaTime);

	HealAccumulator += StepHeal;
	const int32 HealToApply = FMath::FloorToInt(HealAccumulator);
	if (HealToApply > 0)
	{
		const int32 PreviousHp = InOutHp;
		InOutHp = FMath::Clamp(InOutHp + HealToApply, 0, MaxHp);
		HealAccumulator -= static_cast<float>(InOutHp - PreviousHp);
	}

	if (HealRemainTime <= 0.f || PendingHealAmount <= KINDA_SMALL_NUMBER)
	{
		const int32 FinalHealToApply = FMath::CeilToInt(PendingHealAmount + HealAccumulator);
		if (FinalHealToApply > 0)
		{
			InOutHp = FMath::Clamp(InOutHp + FinalHealToApply, 0, MaxHp);
		}

		ResetHealState();
		return true;
	}

	return false;
}

void UJunPlayerPotionComponent::CancelHeal()
{
	ResetHealState();
}

void UJunPlayerPotionComponent::ResetHealState()
{
	HealRemainTime = 0.f;
	PendingHealAmount = 0.f;
	HealAccumulator = 0.f;
}
