#include "Character/Player/JunPlayer.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"

FJunAttackHitResult AJunPlayer::ProcessAttackHit(const FJunAttackHitContext& Context)
{
	return CombatReactionComponent
		? CombatReactionComponent->ReceiveHit(
			Context.HitReactType,
			Context.DamageData.GetFinalDamage(),
			Context.Attacker,
			Context.SwingDirection,
			Context.DefenseKnockbackData,
			Context.DefenseRuleData,
			Context.bCanBuildAttackerPostureOnParry)
		: Super::ProcessAttackHit(Context);
}
