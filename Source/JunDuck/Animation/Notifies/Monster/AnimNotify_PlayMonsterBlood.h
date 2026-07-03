#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayMonsterBlood.generated.h"

UENUM(BlueprintType)
enum class EJunMonsterBloodComponent : uint8
{
	Blood1 UMETA(DisplayName = "Blood 1"),
	Blood2 UMETA(DisplayName = "Blood 2"),
	Blood3 UMETA(DisplayName = "Blood 3")
};

UCLASS(meta = (DisplayName = "Play Monster Blood"))
class JUNDUCK_API UAnimNotify_PlayMonsterBlood : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	EJunMonsterBloodComponent BloodComponent = EJunMonsterBloodComponent::Blood1;
};
