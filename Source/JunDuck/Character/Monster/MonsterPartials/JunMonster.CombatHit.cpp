#include "Character/Monster/JunMonster.h"

FJunAttackHitResult AJunMonster::ProcessAttackHit(const FJunAttackHitContext& Context)
{
	return ReceiveHit(
		Context.HitReactType,
		Context.DamageData.GetFinalDamage(),
		Context.Attacker,
		Context.SwingDirection,
		Context.DefenseKnockbackData,
		Context.DefenseRuleData,
		Context.DamageData.PoiseDamage);
}
