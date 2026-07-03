#include "Animation/Notifies/Boss/AnimNotify_BossFinishParrySuccess.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_BossFinishParrySuccess::GetNotifyName_Implementation() const
{
	return TEXT("BossFinishParrySuccess");
}

void UAnimNotify_BossFinishParrySuccess::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->FinishParrySuccessStateFromNotify();
	}
}
