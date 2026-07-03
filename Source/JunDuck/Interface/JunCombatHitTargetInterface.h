#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunCombatHitTargetInterface.generated.h"

struct FJunAttackHitContext;
struct FJunAttackHitResult;

UINTERFACE(MinimalAPI)
class UJunCombatHitTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunCombatHitTargetInterface
{
	GENERATED_BODY()

public:
	virtual FJunAttackHitResult ProcessAttackHit(const FJunAttackHitContext& Context) = 0;
};
