
#pragma once

#include "CoreMinimal.h"
#include "Animation/JunAnimInstance.h"
#include "Character/JunPlayer.h"
#include "JunPlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class JUNDUCK_API UJunPlayerAnimInstance : public UJunAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	virtual void UpdateMovementDirectionData(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bIsLockOn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
	bool bHasMoveInput = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
	float RawMoveForward = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
	float RawMoveRight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
	bool bUseRunLocomotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
	bool bShouldPlayLockOnForwardRunStart = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Jump")
	bool bJumpStartTriggered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Jump")
	bool bSuppressLandingAnim = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Jump")
	bool bSuppressAirborneAnim = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bUseGuardBase = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	EJunDefenseState DefenseState = EJunDefenseState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bIsGuardStarting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bIsGuarding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bIsGuardEnding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bIsGuardBlockReacting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bIsGuardBlockReleasing = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	int32 GuardStartRestartSerial = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bGuardStartRestartRequested = false;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Guard")
	bool bGuardStartRepeatRequested = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Guard")
	float GuardMovementDirectionInterpSpeed = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement")
	float LockOnForwardRunStartForwardThreshold = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement")
	float LockOnForwardRunStartSideTolerance = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement")
	float LockOnForwardRunStartTriggerDuration = 0.1f;

private:
	float LockOnForwardRunStartTriggerRemainTime = 0.f;
	bool bHadMoveInputLastFrame = false;
	int32 LastGuardStartRestartSerial = 0;
	EJunDefenseState LastDefenseState = EJunDefenseState::None;
};
