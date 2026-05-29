#include "Animation/AnimNotifyState_WukongLightningSwordAirMove.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_WukongLightningSwordAirMove::GetNotifyName_Implementation() const
{
	return TEXT("WukongLightningSwordAirMove");
}

void UAnimNotifyState_WukongLightningSwordAirMove::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->BeginLightningSwordAirMoveWindow(
			MoveMode,
			TargetHeightOffset,
			RiseSpeed,
			DiveSpeed,
			DiveGravityScale,
			bLockHorizontalVelocity);
	}
}

void UAnimNotifyState_WukongLightningSwordAirMove::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->EndLightningSwordAirMoveWindow(MoveMode);
	}
}
