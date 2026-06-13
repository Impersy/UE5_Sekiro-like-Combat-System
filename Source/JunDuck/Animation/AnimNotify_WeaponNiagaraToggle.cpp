#include "Animation/AnimNotify_WeaponNiagaraToggle.h"

#include "Animation/AnimMontage.h"
#include "Character/JunCharacter.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	FString GetWeaponNiagaraComponentLabel(const EJunWeaponNiagaraComponent ComponentType)
	{
		if (const UEnum* Enum = StaticEnum<EJunWeaponNiagaraComponent>())
		{
			return Enum->GetDisplayNameTextByValue(static_cast<int64>(ComponentType)).ToString();
		}

		return TEXT("Weapon Niagara");
	}
}

FString UAnimNotify_WeaponNiagaraToggle::GetNotifyName_Implementation() const
{
	const TCHAR* ActionLabel = Action == EJunWeaponNiagaraToggleAction::On ? TEXT("On") : TEXT("Off");
	return FString::Printf(TEXT("%s %s"), *GetWeaponNiagaraComponentLabel(ComponentType), ActionLabel);
}

void UAnimNotify_WeaponNiagaraToggle::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || ComponentType == EJunWeaponNiagaraComponent::None)
	{
		return;
	}

	AJunCharacter* Character = Cast<AJunCharacter>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	if (Action == EJunWeaponNiagaraToggleAction::On)
	{
		Character->BeginPersistentWeaponNiagara(ComponentType, Cast<UAnimMontage>(Animation));
	}
	else
	{
		Character->EndPersistentWeaponNiagara(ComponentType);
	}
}
