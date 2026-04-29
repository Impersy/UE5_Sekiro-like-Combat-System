


#include "Animation/AnimNotifyState_AttackTrace.h"
#include "Character/JunCharacter.h"

FString UAnimNotifyState_AttackTrace::GetNotifyName_Implementation() const
{
	return TEXT("AttackTrace");
}

void UAnimNotifyState_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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

	Character->BeginAttackTraceWindow(HitReactType, DefenseKnockbackData);
}

void UAnimNotifyState_AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	Character->EndAttackTraceWindow();
}
