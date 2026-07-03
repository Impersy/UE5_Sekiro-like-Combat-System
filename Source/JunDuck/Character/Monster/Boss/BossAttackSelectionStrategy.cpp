#include "Character/Monster/Boss/BossAttackSelectionStrategy.h"

EBossNormalAttackType FWeightedRandomBossAttackSelectionStrategy::SelectNormalAttack(
	const TArray<FBossWeightedNormalAttackCandidate>& Candidates) const
{
	int32 TotalWeight = 0;
	for (const FBossWeightedNormalAttackCandidate& Candidate : Candidates)
	{
		TotalWeight += FMath::Max(1, Candidate.Weight);
	}

	if (TotalWeight <= 0)
	{
		return EBossNormalAttackType::None;
	}

	int32 Roll = FMath::RandRange(1, TotalWeight);
	for (const FBossWeightedNormalAttackCandidate& Candidate : Candidates)
	{
		Roll -= FMath::Max(1, Candidate.Weight);
		if (Roll <= 0)
		{
			return Candidate.AttackType;
		}
	}

	return Candidates.Last().AttackType;
}
