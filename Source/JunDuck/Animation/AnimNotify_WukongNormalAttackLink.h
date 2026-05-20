#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Character/WukongBoss.h"
#include "AnimNotify_WukongNormalAttackLink.generated.h"

USTRUCT(BlueprintType)
struct FWukongNormalAttackLinkCandidate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	EWukongNormalAttackType AttackType = EWukongNormalAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	float BlendOutTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	float BlendInTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	bool bRequireRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	bool bUseTestFilter = true;
};

UCLASS()
class JUNDUCK_API UAnimNotify_WukongNormalAttackLink : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	EWukongNormalAttackType NextAttackType = EWukongNormalAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	TArray<EWukongNormalAttackType> AdditionalAttackCandidates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	TArray<FWukongNormalAttackLinkCandidate> AdditionalAttackCandidateSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	float BlendOutTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	float BlendInTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	bool bRequireRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	bool bUseTestFilter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|AttackLink")
	bool bDebugLog = false;
};
