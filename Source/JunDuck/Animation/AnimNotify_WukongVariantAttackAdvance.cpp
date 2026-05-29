#include "Animation/AnimNotify_WukongVariantAttackAdvance.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongVariantAttackAdvance::GetNotifyName_Implementation() const
{
	return TEXT("WukongVariantAttackAdvance");
}

void UAnimNotify_WukongVariantAttackAdvance::Notify(
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

	const bool bAdvanced = Wukong->AdvanceVariantNormalAttackFromNotify();
	if (bDebugLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WukongVariantAttackAdvance] Advanced=%s Animation=%s"),
			bAdvanced ? TEXT("true") : TEXT("false"),
			Animation ? *Animation->GetName() : TEXT("None"));
	}
}
