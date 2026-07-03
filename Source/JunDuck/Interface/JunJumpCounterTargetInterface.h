#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "UObject/Interface.h"
#include "JunJumpCounterTargetInterface.generated.h"

struct JUNDUCK_API FJunJumpCounterStompPlacementSettings
{
	float ForwardOffset = 0.f;
	float HeightOffset = 0.f;
	float MinVisualDistance = 0.f;
	float CapsuleMargin = 0.f;
	bool bIgnoreTargetCollisionDuringFollowUp = false;
};

UINTERFACE(MinimalAPI)
class UJunJumpCounterTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunJumpCounterTargetInterface
{
	GENERATED_BODY()

public:
	virtual FVector ResolveJumpCounterStompAnchorLocation(
		const class AJunPlayer* Player,
		const FJunJumpCounterStompPlacementSettings& PlacementSettings) const = 0;
	virtual bool IsJumpCounterStompThreatActive() const = 0;
	virtual bool CanStartJumpCounterStompFrom(const class AJunPlayer* Player, float MaxStartDistance) const = 0;
	virtual void SetJumpCounterStompCollisionIgnored(class AJunPlayer* Player, bool bIgnore) = 0;
	virtual void ApplyJumpCounterStompResult(
		class AJunPlayer* Player,
		EHitReactType HitReactType,
		const FJunAttackDamageData& DamageData,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		float PostureAmount) = 0;
};
