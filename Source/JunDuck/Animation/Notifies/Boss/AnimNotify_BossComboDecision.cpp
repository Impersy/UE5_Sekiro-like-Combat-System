#include "Animation/Notifies/Boss/AnimNotify_BossComboDecision.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_BossComboDecision::GetNotifyName_Implementation() const
{
	return TEXT("BossComboDecision");
}

void UAnimNotify_BossComboDecision::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->HandleComboDecisionPoint();
	}
}
