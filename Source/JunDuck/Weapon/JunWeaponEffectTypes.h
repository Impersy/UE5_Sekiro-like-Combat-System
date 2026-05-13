#pragma once

#include "CoreMinimal.h"
#include "JunWeaponEffectTypes.generated.h"

UENUM(BlueprintType)
enum class EJunWeaponNiagaraComponent : uint8
{
	Trail,
	SpecialTrail,
	Aura,
	None
};
