#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WukongHitTurnDecision.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_WukongHitTurnDecision : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
