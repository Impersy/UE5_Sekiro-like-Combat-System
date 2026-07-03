#include "Animation/Notifies/Combat/AnimNotifyState_AttackFacingWindow.h"

#include "Character/Monster/JunMonster.h"
#include "Character/Player/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_AttackFacingWindow::GetNotifyName_Implementation() const
{
	return TEXT("AttackFacingWindow");
}

void UAnimNotifyState_AttackFacingWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->BeginAttackFacingWindow(FacingInterpSpeed);
		return;
	}

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->BeginAttackFacingWindow(FacingInterpSpeed);
	}
}

void UAnimNotifyState_AttackFacingWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr)
	{
		Monster->EndAttackFacingWindow();
		return;
	}

	if (AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr)
	{
		Player->EndAttackFacingWindow();
	}
}
