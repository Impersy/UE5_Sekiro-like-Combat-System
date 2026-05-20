#include "Animation/AnimNotifyState_HitReactFacingWindow.h"

#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_HitReactFacingWindow::GetNotifyName_Implementation() const
{
	return TEXT("HitReactFacingWindow");
}

void UAnimNotifyState_HitReactFacingWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->BeginHitReactFacingWindow(FacingInterpSpeed);
		return;
	}

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->BeginHitReactFacingWindow(FacingInterpSpeed);
	}
}

void UAnimNotifyState_HitReactFacingWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->EndHitReactFacingWindow();
		return;
	}

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->EndHitReactFacingWindow();
	}
}
