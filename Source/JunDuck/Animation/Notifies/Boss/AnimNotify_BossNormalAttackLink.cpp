#include "Animation/Notifies/Boss/AnimNotify_BossNormalAttackLink.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_BossNormalAttackLink::GetNotifyName_Implementation() const
{
	return TEXT("BossNormalAttackLink");
}

void UAnimNotify_BossNormalAttackLink::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		FBossNormalAttackLinkRequest Request;
		Request.DefaultAttackType = NextAttackType;
		Request.LegacyAdditionalAttackTypes = AdditionalAttackCandidates;
		Request.AdditionalCandidates = AdditionalAttackCandidateSettings;
		Request.TriggerChance = TriggerChance;
		Request.DefaultBlendOutTime = BlendOutTime;
		Request.DefaultBlendInTime = BlendInTime;
		Request.bDefaultRequireRange = bRequireRange;
		Request.bDefaultMoveToRangeWhenOutOfRange = bMoveToRangeWhenOutOfRange;
		Request.bDelayMoveToRangeWhenOutOfRange = bDelayMoveToRangeWhenOutOfRange;
		Request.bForceAttackLinkWhenTriggerChanceFails = bForceAttackLinkWhenTriggerChanceFails;
		Request.bDefaultUseTestFilter = bUseTestFilter;
		Request.bExcludeCurrentAttack = bExcludeCurrentAttack;
		Request.bAvoidRecentlyUsedAttack = bAvoidRecentlyUsedAttack;
		Request.bDebugLog = bDebugLog;
		Boss->ProcessNormalAttackLinkRequest(Request);
	}
}
