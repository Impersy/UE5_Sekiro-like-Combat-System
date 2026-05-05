#include "Animation/AnimNotify_WukongHitTurnDecision.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongHitTurnDecision::GetNotifyName_Implementation() const
{
	return TEXT("WukongHitTurnDecision");
}

void UAnimNotify_WukongHitTurnDecision::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->HandleHitTurnDecisionPoint();
	}
}
