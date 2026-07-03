#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunTutorialTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunTutorialTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunTutorialTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool ShouldPreventPlayerDeathForTutorial(const class AJunPlayer* Player) const = 0;
	virtual void NotifyPlayerDrinkPotionForTutorial(class AJunPlayer* Player) = 0;
};
