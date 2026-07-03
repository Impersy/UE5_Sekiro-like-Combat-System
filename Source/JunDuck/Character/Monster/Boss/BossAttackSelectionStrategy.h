#pragma once

#include "Character/Monster/Boss/JunBossTypes.h"

class IBossAttackSelectionStrategy
{
public:
	virtual ~IBossAttackSelectionStrategy() = default;
	virtual EBossNormalAttackType SelectNormalAttack(
		const TArray<FBossWeightedNormalAttackCandidate>& Candidates) const = 0;
};

class FWeightedRandomBossAttackSelectionStrategy final : public IBossAttackSelectionStrategy
{
public:
	virtual EBossNormalAttackType SelectNormalAttack(
		const TArray<FBossWeightedNormalAttackCandidate>& Candidates) const override;
};
