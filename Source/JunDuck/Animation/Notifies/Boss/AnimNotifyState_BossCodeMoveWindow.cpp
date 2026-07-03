#include "Animation/Notifies/Boss/AnimNotifyState_BossCodeMoveWindow.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_BossCodeMoveWindow::GetNotifyName_Implementation() const
{
	return TEXT("BossCodeMoveWindow");
}

void UAnimNotifyState_BossCodeMoveWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->BeginCodeDrivenAttackMoveWindow(AttackType);
	}
}

void UAnimNotifyState_BossCodeMoveWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->EndCodeDrivenAttackMoveWindow();
	}
}
