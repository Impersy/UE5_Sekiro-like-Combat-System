#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunLockOnTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunLockOnTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunLockOnTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool CanBeLockedOnBy(const class AJunPlayer* Player) const = 0;
	virtual FVector GetLockOnCameraTargetPoint() const = 0;
	virtual bool IsLockOnExecutionReady() const = 0;
	virtual void OnLockedOnBy(class AJunPlayer* Player) {}
};
