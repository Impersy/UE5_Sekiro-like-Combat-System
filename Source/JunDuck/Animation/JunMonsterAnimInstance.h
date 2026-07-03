

#pragma once

#include "CoreMinimal.h"
#include "Animation/JunAnimInstance.h"
#include "JunMonsterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class JUNDUCK_API UJunMonsterAnimInstance : public UJunAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Monster")
	bool bHasTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster")
	bool bIsGuard = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Guard")
	bool bUseGuardBase = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster")
	bool bIsInCombat = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Movement")
	bool bUseRunLocomotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|IK")
	float FootPlacementAlpha = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|IK", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float FootPlacementSlopeStartAngle = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|IK", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float FootPlacementSlopeFullAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|IK", meta = (ClampMin = "0.0"))
	float FootPlacementAlphaInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BossEvadeFootPlacementMaxAlpha = 0.3f;
};
