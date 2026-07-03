

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_SendGamplayEvent.generated.h"

/**
 * 
 */
UCLASS()
class JUNDUCK_API UAnimNotify_SendGamplayEvent : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UAnimNotify_SendGamplayEvent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag EventTag;




};
