#include "Animation/AnimNotify_WukongExecuteDelayedRangeLink.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongExecuteDelayedRangeLink::GetNotifyName_Implementation() const
{
	return TEXT("WukongExecuteDelayedRangeLink");
}

void UAnimNotify_WukongExecuteDelayedRangeLink::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr;
	if (!Wukong)
	{
		return;
	}

	const float BlendOutOverride = bOverrideBlendOutTime ? FMath::Max(0.f, BlendOutTime) : -1.f;
	const bool bStarted = Wukong->TryExecuteDelayedNormalAttackRangeLinkFromNotify(BlendOutOverride);

	if (bDebugLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WukongDelayedRangeLink] Started=%s OverrideBlendOut=%s BlendOut=%.2f"),
			bStarted ? TEXT("true") : TEXT("false"),
			bOverrideBlendOutTime ? TEXT("true") : TEXT("false"),
			BlendOutOverride);
	}
}
