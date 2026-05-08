#include "Animation/AnimNotify_FireMonsterArrow.h"

#include "Character/JunMonster.h"

FString UAnimNotify_FireMonsterArrow::GetNotifyName_Implementation() const
{
	return TEXT("FireMonsterArrow");
}

void UAnimNotify_FireMonsterArrow::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AJunMonster* Monster = Cast<AJunMonster>(MeshComp->GetOwner()))
	{
		Monster->FireAttachedArrow(
			HitReactType,
			DamageData,
			DefenseKnockbackData,
			Speed,
			LifeSeconds,
			HomingDuration,
			HomingInterpSpeed,
			HomingTargetHeightOffset);
	}
}
