#include "Animation/AnimNotifyState_AttackComboAdvance.h"
#include "Character/JunPlayer.h"

FString UAnimNotifyState_AttackComboAdvance::GetNotifyName_Implementation() const
{
	return ComboType == EJunAttackComboType::HeavyAttack
		? TEXT("HeavyAttackComboAdvance")
		: TEXT("BasicAttackComboAdvance");
}

void UAnimNotifyState_AttackComboAdvance::NotifyBegin(
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

	Player->OnAttackComboAdvanceStateBegin(ComboType);
}

void UAnimNotifyState_AttackComboAdvance::NotifyEnd(
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

	Player->OnAttackComboAdvanceStateEnd(ComboType);
}
