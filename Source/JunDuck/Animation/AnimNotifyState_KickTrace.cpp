#include "Animation/AnimNotifyState_KickTrace.h"
#include "Character/JunCharacter.h"

FString UAnimNotifyState_KickTrace::GetNotifyName_Implementation() const
{
	return TEXT("KickTrace");
}

void UAnimNotifyState_KickTrace::NotifyBegin(
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

	Character->BeginKickAttackTraceWindow(HitReactType, DamageData, DefenseKnockbackData);
}

void UAnimNotifyState_KickTrace::NotifyEnd(
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

	Character->EndKickAttackTraceWindow();
}
