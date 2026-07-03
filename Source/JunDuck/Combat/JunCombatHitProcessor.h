#pragma once

#include "Character/JunCharacter.h"

class JUNDUCK_API FJunCombatHitProcessor
{
public:
	static FJunAttackHitResult ProcessHit(
		AJunCharacter& Target,
		const FJunAttackHitContext& Context);

private:
	static void ProcessCommonFeedback(
		AJunCharacter& Target,
		const FJunAttackHitContext& Context,
		const FJunAttackHitResult& Result);
};
