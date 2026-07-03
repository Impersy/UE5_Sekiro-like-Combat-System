#include "Animation/Notifies/Player/AnimNotifyState_BasicAttackComboAdvance.h"
#include "Character/Player/JunPlayer.h"

FString UAnimNotifyState_BasicAttackComboAdvance::GetNotifyName_Implementation() const
{
	return TEXT("BasicAttackComboAdvance");
}

void UAnimNotifyState_BasicAttackComboAdvance::NotifyBegin(
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

	AJunPlayer* Player = Cast<AJunPlayer>(MeshComp->GetOwner());
	if (!Player)
	{
		return;
	}

	Player->OnBasicAttackComboAdvanceStateBegin();
}

void UAnimNotifyState_BasicAttackComboAdvance::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AJunPlayer* Player = Cast<AJunPlayer>(MeshComp->GetOwner());
	if (!Player)
	{
		return;
	}

	Player->OnBasicAttackComboAdvanceStateEnd();
}
