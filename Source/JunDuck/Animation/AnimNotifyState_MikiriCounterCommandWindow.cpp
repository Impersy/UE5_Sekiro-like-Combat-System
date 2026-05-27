#include "Animation/AnimNotifyState_MikiriCounterCommandWindow.h"

#include "Character/JunCharacter.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_MikiriCounterCommandWindow::GetNotifyName_Implementation() const
{
	return TEXT("MikiriCounterCommandWindow");
}

void UAnimNotifyState_MikiriCounterCommandWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunCharacter* Character = MeshComp ? Cast<AJunCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->BeginMikiriCounterCommandWindow();
	}
}

void UAnimNotifyState_MikiriCounterCommandWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunCharacter* Character = MeshComp ? Cast<AJunCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->EndMikiriCounterCommandWindow();
	}
}
