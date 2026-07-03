#include "Animation/Notifies/Boss/AnimNotify_BossExecuteDelayedRangeLink.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_BossExecuteDelayedRangeLink::GetNotifyName_Implementation() const
{
	return TEXT("BossExecuteDelayedRangeLink");
}

void UAnimNotify_BossExecuteDelayedRangeLink::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr;
	if (!Boss)
	{
		return;
	}

	const float BlendOutOverride = bOverrideBlendOutTime ? FMath::Max(0.f, BlendOutTime) : -1.f;
	const EBossNormalAttackLinkResult Result = Boss->TryExecuteDelayedNormalAttackRangeLinkFromNotify(BlendOutOverride);

	if (bDebugLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossDelayedRangeLink] Result=%s OverrideBlendOut=%s BlendOut=%.2f"),
			LexToString(Result),
			bOverrideBlendOutTime ? TEXT("true") : TEXT("false"),
			BlendOutOverride);
	}
}
