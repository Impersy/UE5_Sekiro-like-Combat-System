#include "Animation/AnimNotify_WukongFinishParrySuccess.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongFinishParrySuccess::GetNotifyName_Implementation() const
{
	return TEXT("WukongFinishParrySuccess");
}

void UAnimNotify_WukongFinishParrySuccess::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->FinishParrySuccessStateFromNotify();
	}
}
