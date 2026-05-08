#include "Animation/AnimNotify_SpawnMonsterArrow.h"

#include "Character/JunMonster.h"

FString UAnimNotify_SpawnMonsterArrow::GetNotifyName_Implementation() const
{
	return TEXT("SpawnMonsterArrow");
}

void UAnimNotify_SpawnMonsterArrow::Notify(
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
		Monster->SpawnAttachedArrow();
	}
}
