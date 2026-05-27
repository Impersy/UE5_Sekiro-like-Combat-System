#include "Animation/AnimNotifyState_WukongAirborneHitReactLock.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_WukongAirborneHitReactLock::GetNotifyName_Implementation() const
{
	return TEXT("WukongAirborneHitReactLock");
}

void UAnimNotifyState_WukongAirborneHitReactLock::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->BeginAirborneHitReactLockWindow();
	}
}

void UAnimNotifyState_WukongAirborneHitReactLock::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->EndAirborneHitReactLockWindow();
	}
}
