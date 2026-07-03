#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunMikiriCounterTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunMikiriCounterTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunMikiriCounterTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool IsMikiriCounterThreatActiveFor(const class AJunPlayer* Player) const = 0;
	virtual bool CanBeMikiriCounteredBy(const class AJunPlayer* Player) const = 0;
	virtual bool NotifyMikiriCounteredBy(class AJunPlayer* CounterPlayer) = 0;
};
