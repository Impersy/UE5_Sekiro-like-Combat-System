#include "Animation/AnimNotifyState_DodgeChainWindow.h"

#include "Character/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_DodgeChainWindow::GetNotifyName_Implementation() const
{
	return TEXT("DodgeChainWindow");
}

void UAnimNotifyState_DodgeChainWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->BeginDodgeChainWindow();
	}
}

void UAnimNotifyState_DodgeChainWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->EndDodgeChainWindow();
	}
}
