#include "Animation/Notifies/Boss/AnimNotify_BossHitTurnDecision.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_BossHitTurnDecision::GetNotifyName_Implementation() const
{
	return TEXT("BossHitTurnDecision");
}

void UAnimNotify_BossHitTurnDecision::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->HandleHitTurnDecisionPoint();
	}
}
