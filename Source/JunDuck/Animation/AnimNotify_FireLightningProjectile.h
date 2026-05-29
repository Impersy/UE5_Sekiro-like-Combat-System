#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Character/JunCharacter.h"
#include "Weapon/ArrowProjectile.h"
#include "AnimNotify_FireLightningProjectile.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_FireLightningProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	TSubclassOf<AArrowProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	FName SpawnSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	FVector SpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	EHitReactType HitReactType = EHitReactType::Lighting_Shock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	FJunAttackDamageData DamageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	FJunAttackDefenseKnockbackData DefenseKnockbackData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	FJunAttackDefenseRuleData DefenseRuleData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Player Counter")
	bool bPlayerRequiresLastIncomingHitReactType = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Player Counter", meta = (EditCondition = "bPlayerRequiresLastIncomingHitReactType"))
	EHitReactType RequiredPlayerLastIncomingHitReactType = EHitReactType::Lighting_Shock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning", meta = (ClampMin = "0"))
	float Speed = 2600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning", meta = (ClampMin = "0.1"))
	float LifeSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Homing")
	bool bUseHoming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Homing", meta = (ClampMin = "0"))
	float HomingDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Homing", meta = (ClampMin = "0"))
	float HomingInterpSpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Homing")
	float HomingTargetHeightOffset = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning|Target", meta = (ClampMin = "0"))
	float TargetSearchRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
	bool bDebugLog = false;

private:
	AJunCharacter* FindBestTarget(AJunCharacter* OwnerCharacter) const;
};
