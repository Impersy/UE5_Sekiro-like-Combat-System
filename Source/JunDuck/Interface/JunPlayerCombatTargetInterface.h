#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunPlayerCombatTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunPlayerCombatTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunPlayerCombatTargetInterface
{
	GENERATED_BODY()

public:
	virtual void ApplyPostureFromPlayer(float Amount) = 0;
	virtual void OnPlayerFakeDeathStarted() = 0;
	virtual void OnPlayerFakeDeathRevived() = 0;
	virtual void OnPlayerRealDeathStarted() = 0;
	virtual void OnPlayerRealDeathRespawned() = 0;
};
