#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/JunCharacter.h"
#include "AnimNotifyState_KickTrace.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotifyState_KickTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KickTrace")
	EHitReactType HitReactType = EHitReactType::LightHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KickTrace")
	FJunAttackDefenseKnockbackData DefenseKnockbackData;

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
