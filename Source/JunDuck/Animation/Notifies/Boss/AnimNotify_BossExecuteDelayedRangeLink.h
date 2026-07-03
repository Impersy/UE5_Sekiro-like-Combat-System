#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossExecuteDelayedRangeLink.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_BossExecuteDelayedRangeLink : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bOverrideBlendOutTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink", meta = (EditCondition = "bOverrideBlendOutTime", ClampMin = "0.0"))
	float BlendOutTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bDebugLog = false;
};
