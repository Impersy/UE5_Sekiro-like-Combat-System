#include "Animation/AnimNotify_WukongComboDecision.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongComboDecision::GetNotifyName_Implementation() const
{
	return TEXT("WukongComboDecision");
}

void UAnimNotify_WukongComboDecision::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->HandleComboDecisionPoint();
	}
}
