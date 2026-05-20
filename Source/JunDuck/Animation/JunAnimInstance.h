

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "JunAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class JUNDUCK_API UJunAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UJunAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class AJunCharacter> Character;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class UCharacterMovementComponent> MovementComponent;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BlendSpaceSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LocalMoveForward = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LocalMoveRight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsRunning = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float MovementDirectionInterpSpeed = 6.f;

protected:
	virtual void UpdateMovementData(float DeltaSeconds);
	virtual void UpdateMovementDirectionData(float DeltaSeconds);
	virtual void UpdateDerivedData(float DeltaSeconds);

};
