

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "JunGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class JUNDUCK_API UJunGameInstance : public UGameInstance
{
	GENERATED_BODY()
	

public:
	UJunGameInstance(const FObjectInitializer& ObjectInitializer);


public:
	virtual void Init() override;
	virtual void Shutdown() override;

	void RequestRestartFadeIn() { bRestartFadeInRequested = true; }
	bool ConsumeRestartFadeInRequest();

private:
	bool bRestartFadeInRequested = false;
};
