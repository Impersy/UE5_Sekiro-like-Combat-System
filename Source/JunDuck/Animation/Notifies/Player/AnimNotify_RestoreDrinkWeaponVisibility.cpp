#include "Animation/Notifies/Player/AnimNotify_RestoreDrinkWeaponVisibility.h"
#include "Character/Player/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_RestoreDrinkWeaponVisibility::GetNotifyName_Implementation() const
{
	return TEXT("RestoreDrinkWeaponVisibility");
}

void UAnimNotify_RestoreDrinkWeaponVisibility::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr;
	if (!Player)
	{
		return;
	}

	Player->RestoreDrinkPotionWeaponVisibility();
}
