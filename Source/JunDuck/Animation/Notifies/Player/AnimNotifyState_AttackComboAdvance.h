#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_AttackComboAdvance.generated.h"

UENUM(BlueprintType)
enum class EJunAttackComboType : uint8
{
	BasicAttack,
	HeavyAttack,
	Jigen
};

UCLASS()
class JUNDUCK_API UAnimNotifyState_AttackComboAdvance : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	EJunAttackComboType ComboType = EJunAttackComboType::BasicAttack;

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
};
