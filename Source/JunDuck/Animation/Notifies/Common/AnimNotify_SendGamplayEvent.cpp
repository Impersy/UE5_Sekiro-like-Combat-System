


#include "Animation/Notifies/Common/AnimNotify_SendGamplayEvent.h"
#include "Character/JunCharacter.h"

UAnimNotify_SendGamplayEvent::UAnimNotify_SendGamplayEvent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void UAnimNotify_SendGamplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || !EventTag.IsValid())
    {
        return;
    }

    AActor* OwnerActor = MeshComp->GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    AJunCharacter* Character = Cast<AJunCharacter>(OwnerActor);
    if (!Character)
    {
        return;
    }

    Character->HandleGameplayEventNotify(EventTag);


}

FString UAnimNotify_SendGamplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? EventTag.ToString() : TEXT("SendGameplayEvent");
}
