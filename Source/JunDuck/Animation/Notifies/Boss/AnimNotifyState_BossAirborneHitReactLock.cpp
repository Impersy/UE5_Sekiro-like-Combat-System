#include "Animation/Notifies/Boss/AnimNotifyState_BossAirborneHitReactLock.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_BossAirborneHitReactLock::GetNotifyName_Implementation() const
{
	return TEXT("BossAirborneHitReactLock");
}

void UAnimNotifyState_BossAirborneHitReactLock::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->BeginAirborneHitReactLockWindow();
	}
}

void UAnimNotifyState_BossAirborneHitReactLock::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->EndAirborneHitReactLockWindow();
	}
}
