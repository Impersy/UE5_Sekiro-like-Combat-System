#include "Animation/AnimNotifyState_JumpCounterStompJumpAttackWindow.h"

#include "Character/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_JumpCounterStompJumpAttackWindow::GetNotifyName_Implementation() const
{
	return TEXT("JumpCounterStompJumpAttackWindow");
}

void UAnimNotifyState_JumpCounterStompJumpAttackWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->BeginJumpCounterStompJumpAttackWindow();
	}
}

void UAnimNotifyState_JumpCounterStompJumpAttackWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->EndJumpCounterStompJumpAttackWindow();
	}
}
