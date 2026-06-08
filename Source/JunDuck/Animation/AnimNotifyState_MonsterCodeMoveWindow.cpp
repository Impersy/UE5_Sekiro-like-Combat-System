#include "Animation/AnimNotifyState_MonsterCodeMoveWindow.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_MonsterCodeMoveWindow::GetNotifyName_Implementation() const
{
	return TEXT("MonsterCodeMoveWindow");
}

void UAnimNotifyState_MonsterCodeMoveWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->BeginMonsterCodeMoveWindow(CodeMoveData);
	}
}

void UAnimNotifyState_MonsterCodeMoveWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->EndMonsterCodeMoveWindow();
	}
}
