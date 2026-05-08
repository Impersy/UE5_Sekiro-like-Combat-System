#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Character/JunCharacter.h"
#include "AnimNotify_FireMonsterArrow.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_FireMonsterArrow : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	EHitReactType HitReactType = EHitReactType::LightHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	FJunAttackDamageData DamageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	FJunAttackDefenseKnockbackData DefenseKnockbackData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow", meta = (ClampMin = "0"))
	float Speed = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow", meta = (ClampMin = "0.1"))
	float LifeSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Homing", meta = (ClampMin = "0"))
	float HomingDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Homing", meta = (ClampMin = "0"))
	float HomingInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Homing")
	float HomingTargetHeightOffset = 50.f;

	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
