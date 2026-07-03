#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "UObject/Interface.h"
#include "JunDefenseReactionTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunDefenseReactionTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunDefenseReactionTargetInterface
{
	GENERATED_BODY()

public:
	virtual void NotifyAttackParriedBy(
		class AJunPlayer* Parrier,
		float PostureScale,
		const FJunAttackDefenseRuleData& DefenseRuleData) = 0;

	virtual void NotifyPlayerParrySucceeded(
		class AJunPlayer* Parrier,
		bool bPerfectParry,
		bool bAirParry) {}
};
