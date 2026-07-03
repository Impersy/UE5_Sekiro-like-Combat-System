#include "Animation/Notifies/Combat/AnimNotifyState_SwordTrail.h"

#include "Character/JunCharacter.h"

FString UAnimNotifyState_SwordTrail::GetNotifyName_Implementation() const
{
	return TEXT("SwordTrail");
}

void UAnimNotifyState_SwordTrail::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AJunCharacter* Character = Cast<AJunCharacter>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	Character->BeginWeaponNiagaraWindow(ComponentType);
}

void UAnimNotifyState_SwordTrail::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AJunCharacter* Character = Cast<AJunCharacter>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	Character->EndWeaponNiagaraWindow(ComponentType);
}
