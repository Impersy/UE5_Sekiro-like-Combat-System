

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/JunCharacter.h"
#include "AnimNotifyState_AttackTrace.generated.h"

/**
 * 
 */
UCLASS()
class JUNDUCK_API UAnimNotifyState_AttackTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace")
	EHitReactType HitReactType = EHitReactType::LightHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace")
	FJunAttackDamageData DamageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace")
	FJunAttackDefenseKnockbackData DefenseKnockbackData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace|VFX")
	EJunWeaponNiagaraComponent AttackTraceNiagaraComponent = EJunWeaponNiagaraComponent::Trail;

	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
