#include "Animation/Notifies/Player/AnimNotifyState_DodgeAttackWindow.h"

#include "Character/Player/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_DodgeAttackWindow::GetNotifyName_Implementation() const
{
	return TEXT("DodgeAttackWindow");
}

void UAnimNotifyState_DodgeAttackWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->BeginDodgeAttackWindow();
	}
}

void UAnimNotifyState_DodgeAttackWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->EndDodgeAttackWindow();
	}
}
