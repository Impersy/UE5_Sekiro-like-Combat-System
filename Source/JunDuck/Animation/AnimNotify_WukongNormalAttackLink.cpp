#include "Animation/AnimNotify_WukongNormalAttackLink.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongNormalAttackLink::GetNotifyName_Implementation() const
{
	return TEXT("WukongNormalAttackLink");
}

void UAnimNotify_WukongNormalAttackLink::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		const bool bStarted = Wukong->TryStartNormalAttackLinkFromNotify(
			NextAttackType,
			TriggerChance,
			BlendOutTime,
			BlendInTime,
			bRequireRange,
			bUseTestFilter
		);

		if (bDebugLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WukongAttackLink] Started=%s NextAttack=%d Chance=%.2f"),
				bStarted ? TEXT("true") : TEXT("false"),
				static_cast<int32>(NextAttackType),
				TriggerChance);
		}
	}
}
