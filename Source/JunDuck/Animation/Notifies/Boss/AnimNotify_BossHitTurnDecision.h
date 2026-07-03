#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossHitTurnDecision.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_BossHitTurnDecision : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
