#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Camera/CameraShakeBase.h"
#include "AnimNotify_CameraShake.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_CameraShake : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShakeClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0"))
	float Scale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	FRotator UserPlaySpaceRot = FRotator::ZeroRotator;
};
