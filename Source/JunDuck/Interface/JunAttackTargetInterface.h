#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunAttackTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunAttackTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunAttackTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool CanBeAttackTargetedBy(const class AJunPlayer* Player) const = 0;
	virtual FVector GetAttackTargetPoint(const class AJunPlayer* Player) const = 0;
};
