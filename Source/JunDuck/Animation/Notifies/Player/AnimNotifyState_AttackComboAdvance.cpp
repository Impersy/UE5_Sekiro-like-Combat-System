#include "Animation/Notifies/Player/AnimNotifyState_AttackComboAdvance.h"
#include "Character/Player/JunPlayer.h"

FString UAnimNotifyState_AttackComboAdvance::GetNotifyName_Implementation() const
{
	switch (ComboType)
	{
	case EJunAttackComboType::HeavyAttack:
		return TEXT("HeavyAttackComboAdvance");
	case EJunAttackComboType::Jigen:
		return TEXT("JigenComboAdvance");
	case EJunAttackComboType::BasicAttack:
	default:
		return TEXT("BasicAttackComboAdvance");
	}
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
