#include "Animation/AnimNotifyState_WukongCodeMoveWindow.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_WukongCodeMoveWindow::GetNotifyName_Implementation() const
{
	return TEXT("WukongCodeMoveWindow");
}

void UAnimNotifyState_WukongCodeMoveWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->BeginCodeDrivenAttackMoveWindow(AttackType);
	}
}

void UAnimNotifyState_WukongCodeMoveWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->EndCodeDrivenAttackMoveWindow();
	}
}
