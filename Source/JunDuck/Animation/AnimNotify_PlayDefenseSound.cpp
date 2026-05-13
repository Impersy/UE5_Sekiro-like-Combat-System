#include "Animation/AnimNotify_PlayDefenseSound.h"

#include "Character/JunCharacter.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_PlayDefenseSound::GetNotifyName_Implementation() const
{
	return TEXT("PlayDefenseSound");
}

void UAnimNotify_PlayDefenseSound::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AJunCharacter* Character = MeshComp ? Cast<AJunCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->HandleDefenseSoundNotify();
	}
}
