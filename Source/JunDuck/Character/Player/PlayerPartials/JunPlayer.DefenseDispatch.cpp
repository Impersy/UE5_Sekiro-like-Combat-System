#include "Character/Player/JunPlayer.h"

#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#pragma region Defense Resolution / Incoming Hit Dispatch
// ============================================================================
// Defense Resolution / Incoming Hit Dispatch
// ============================================================================

bool AJunPlayer::TryHandleSpecialDefense(
	AActor* DamageCauser,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (CombatReactionComponent)
	{
		return CombatReactionComponent->TryHandleSpecialDefense(DamageCauser, DefenseRuleData);
	}

	return false;
}

void AJunPlayer::NotifyDefenseReactionTarget(
	AActor* DamageCauser,
	bool bPerfectParry,
	bool bAirParry,
	float PostureScale,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->NotifyDefenseReactionTarget(
			DamageCauser,
			bPerfectParry,
			bAirParry,
			PostureScale,
			DefenseRuleData,
			bCanBuildAttackerPostureOnParry);
	}
}

void AJunPlayer::HandlePerfectParrySuccess(
	AActor* DamageCauser,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->HandlePerfectParrySuccess(
			DamageCauser,
			DefenseRuleData,
			bCanBuildAttackerPostureOnParry);
	}
}

void AJunPlayer::HandleNormalParrySuccess(
	AActor* DamageCauser,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->HandleNormalParrySuccess(
			DamageCauser,
			DefenseRuleData,
			bCanBuildAttackerPostureOnParry);
	}
}

void AJunPlayer::HandleGuardBlockSuccess()
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->HandleGuardBlockSuccess();
	}
}

void AJunPlayer::HandleDamageHitReact(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->HandleDamageHitReact(
			HitType,
			DamageAmount,
			DamageCauser,
			SwingDirection,
			DefenseRuleData);
	}
}

#pragma endregion
