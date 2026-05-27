#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Character/JunCharacter.h"
#include "AnimNotify_JumpCounterStompBounce.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_JumpCounterStompBounce : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpCounter|Stomp|Hit")
	EHitReactType HitReactType = EHitReactType::LightHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpCounter|Stomp|Hit")
	FJunAttackDamageData DamageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpCounter|Stomp|Hit")
	FJunAttackDefenseKnockbackData DefenseKnockbackData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpCounter|Stomp")
	float UpVelocity = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpCounter|Stomp")
	float BackwardVelocity = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpCounter|Stomp", meta = (ClampMin = "0.0"))
	float BackwardMoveDuration = 0.15f;
};
