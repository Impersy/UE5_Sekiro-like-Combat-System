#include "Animation/Notifies/Monster/AnimNotify_AttachMonsterWeaponToHand.h"

#include "Character/Monster/JunMonster.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_AttachMonsterWeaponToHand::GetNotifyName_Implementation() const
{
	return TEXT("AttachMonsterWeaponToHand");
}

void UAnimNotify_AttachMonsterWeaponToHand::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->HandleEquipWeaponNotify();
	}
}
