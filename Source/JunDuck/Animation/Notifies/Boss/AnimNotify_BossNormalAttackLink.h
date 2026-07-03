#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Character/Monster/Boss/JunBoss.h"
#include "AnimNotify_BossNormalAttackLink.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotify_BossNormalAttackLink : public UAnimNotify
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
	EBossNormalAttackType NextAttackType = EBossNormalAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	TArray<EBossNormalAttackType> AdditionalAttackCandidates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	TArray<FBossNormalAttackLinkCandidate> AdditionalAttackCandidateSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TriggerChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	float BlendOutTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	float BlendInTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bRequireRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bMoveToRangeWhenOutOfRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bDelayMoveToRangeWhenOutOfRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bForceAttackLinkWhenTriggerChanceFails = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bUseTestFilter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bExcludeCurrentAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bAvoidRecentlyUsedAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AttackLink")
	bool bDebugLog = false;
};
