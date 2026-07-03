#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MontagePlayRateWindow.generated.h"

class UAnimMontage;

USTRUCT()
struct FMontagePlayRateWindowRuntimeData
{
	GENERATED_BODY()

	TWeakObjectPtr<UAnimMontage> Montage;
	float PreviousPlayRate = 1.f;
};

UCLASS()
class JUNDUCK_API UAnimNotifyState_MontagePlayRateWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayRate")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayRate")
	bool bRestorePreviousPlayRate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayRate", meta = (EditCondition = "!bRestorePreviousPlayRate", EditConditionHides))
	float RestorePlayRate = 1.f;

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

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FMontagePlayRateWindowRuntimeData> ActiveRuntimeDataByMesh;
};
