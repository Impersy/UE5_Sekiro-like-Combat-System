#include "Animation/Notifies/Boss/AnimNotifyState_BossLightningSwordAirMove.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_BossLightningSwordAirMove::GetNotifyName_Implementation() const
{
	return TEXT("BossLightningSwordAirMove");
}

void UAnimNotifyState_BossLightningSwordAirMove::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->BeginLightningSwordAirMoveWindow(
			MoveMode,
			TargetHeightOffset,
			RiseSpeed,
			DiveSpeed,
			DiveGravityScale,
			bLockHorizontalVelocity);
	}
}

void UAnimNotifyState_BossLightningSwordAirMove::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->EndLightningSwordAirMoveWindow(MoveMode);
	}
}
