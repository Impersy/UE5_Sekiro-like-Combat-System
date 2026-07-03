#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Weapon/JunWeaponEffectTypes.h"
#include "AnimNotify_WeaponNiagaraToggle.generated.h"

UENUM(BlueprintType)
enum class EJunWeaponNiagaraToggleAction : uint8
{
	On UMETA(DisplayName = "On"),
	Off UMETA(DisplayName = "Off")
};

UCLASS(meta = (DisplayName = "Weapon Niagara Toggle"))
class JUNDUCK_API UAnimNotify_WeaponNiagaraToggle : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Niagara")
	EJunWeaponNiagaraComponent ComponentType = EJunWeaponNiagaraComponent::Trail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Niagara")
	EJunWeaponNiagaraToggleAction Action = EJunWeaponNiagaraToggleAction::On;
};
