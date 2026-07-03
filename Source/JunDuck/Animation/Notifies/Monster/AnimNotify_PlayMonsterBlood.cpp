#include "Animation/Notifies/Monster/AnimNotify_PlayMonsterBlood.h"

#include "Character/Monster/JunMonster.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"

FString UAnimNotify_PlayMonsterBlood::GetNotifyName_Implementation() const
{
	switch (BloodComponent)
	{
	case EJunMonsterBloodComponent::Blood2:
		return TEXT("Monster Blood 2");
	case EJunMonsterBloodComponent::Blood3:
		return TEXT("Monster Blood 3");
	default:
		return TEXT("Monster Blood 1");
	}
}

void UAnimNotify_PlayMonsterBlood::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AJunMonster* Monster = MeshComp ? Cast<AJunMonster>(MeshComp->GetOwner()) : nullptr;
	if (!Monster)
	{
		return;
	}

	FName TargetComponentName = TEXT("Blood_1");
	if (BloodComponent == EJunMonsterBloodComponent::Blood2)
	{
		TargetComponentName = TEXT("Blood_2");
	}
	else if (BloodComponent == EJunMonsterBloodComponent::Blood3)
	{
		TargetComponentName = TEXT("Blood_3");
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	Monster->GetComponents<UNiagaraComponent>(NiagaraComponents);
	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent && NiagaraComponent->GetFName() == TargetComponentName)
		{
			NiagaraComponent->Activate(true);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("PlayMonsterBlood notify could not find Niagara component '%s' on %s."),
		*TargetComponentName.ToString(),
		*GetNameSafe(Monster));
}
