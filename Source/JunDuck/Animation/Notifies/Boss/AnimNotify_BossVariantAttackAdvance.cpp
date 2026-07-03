#include "Animation/Notifies/Boss/AnimNotify_BossVariantAttackAdvance.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_BossVariantAttackAdvance::GetNotifyName_Implementation() const
{
	return TEXT("BossVariantAttackAdvance");
}

void UAnimNotify_BossVariantAttackAdvance::Notify(
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

	const bool bAdvanced = Boss->AdvanceVariantNormalAttackFromNotify();
	if (bDebugLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossVariantAttackAdvance] Advanced=%s Animation=%s"),
			bAdvanced ? TEXT("true") : TEXT("false"),
			Animation ? *Animation->GetName() : TEXT("None"));
	}
}
