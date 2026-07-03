


#include "Animation/JunPlayerAnimInstance.h"
#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"

void UJunPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AJunPlayer* Player = Cast<AJunPlayer>(Character);
	if (!Player)
	{
		bIsLockOn = false;
		bHasMoveInput = false;
		RawMoveForward = 0.f;
		RawMoveRight = 0.f;
		bUseRunLocomotion = false;
		bIsSprinting = false;
		bShouldPlayLockOnForwardRunStart = false;
		bJumpStartTriggered = false;
		bSuppressLandingAnim = false;
		bSuppressAirborneAnim = false;
		FootPlacementAlpha = 0.f;
		bUseGuardBase = false;
		DefenseState = EJunDefenseState::None;
		bIsGuardStarting = false;
		bIsGuarding = false;
		bIsGuardEnding = false;
		bIsGuardBlockReacting = false;
		bIsGuardBlockReleasing = false;
		GuardStartRestartSerial = 0;
		bGuardStartRestartRequested = false;
		bGuardStartRepeatRequested = false;
		LastGuardStartRestartSerial = 0;
		LastDefenseState = EJunDefenseState::None;
		LockOnForwardRunStartTriggerRemainTime = 0.f;
		bHadMoveInputLastFrame = false;
		return;
	}

	bIsLockOn = Player->IsLockOn();
	const bool bHasMoveInputThisFrame =
		!FMath::IsNearlyZero(Player->GetDesiredMoveForward()) ||
		!FMath::IsNearlyZero(Player->GetDesiredMoveRight());
	bHasMoveInput = bHasMoveInputThisFrame;
	RawMoveForward = Player->GetDesiredMoveForward();
	RawMoveRight = Player->GetDesiredMoveRight();
	bUseRunLocomotion = Player->ShouldUseRunningAnim();
	bIsSprinting = Player->IsSprinting();
	bJumpStartTriggered = Player->IsJumpStartAnimTriggered();
	bSuppressLandingAnim = Player->ShouldSuppressLandingAnim();
	bSuppressAirborneAnim = Player->ShouldSuppressAirborneAnim();

	float TargetFootPlacementAlpha = 0.f;
	if (const UCharacterMovementComponent* PlayerMovementComponent = Player->GetCharacterMovement())
	{
		if (!PlayerMovementComponent->IsFalling() && PlayerMovementComponent->CurrentFloor.IsWalkableFloor())
		{
			const FVector FloorNormal = PlayerMovementComponent->CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
			const float FloorNormalZ = FMath::Clamp(FloorNormal.Z, -1.f, 1.f);
			const float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FloorNormalZ));
			const float FullAngle = FMath::Max(FootPlacementSlopeStartAngle + KINDA_SMALL_NUMBER, FootPlacementSlopeFullAngle);
			TargetFootPlacementAlpha = FMath::GetMappedRangeValueClamped(
				FVector2D(FootPlacementSlopeStartAngle, FullAngle),
				FVector2D(0.f, 1.f),
				SlopeAngle);
		}
	}

	FootPlacementAlpha = FootPlacementAlphaInterpSpeed > 0.f
		? FMath::FInterpTo(FootPlacementAlpha, TargetFootPlacementAlpha, DeltaSeconds, FootPlacementAlphaInterpSpeed)
		: TargetFootPlacementAlpha;
	if (FMath::IsNearlyEqual(FootPlacementAlpha, TargetFootPlacementAlpha, 0.001f))
	{
		FootPlacementAlpha = TargetFootPlacementAlpha;
	}

	bUseGuardBase = Player->ShouldUseGuardBase();
	DefenseState = Player->GetDefenseState();
	bIsGuardStarting = DefenseState == EJunDefenseState::Starting;
	bIsGuarding = DefenseState == EJunDefenseState::Guarding;
	bIsGuardEnding = DefenseState == EJunDefenseState::Ending;
	bIsGuardBlockReacting = Player->IsGuardBlockReacting();
	bIsGuardBlockReleasing = Player->IsGuardBlockReleasing();

	GuardStartRestartSerial = Player->GetGuardStartRestartSerial();
	const bool bWasAlreadyStarting = LastDefenseState == EJunDefenseState::Starting;
	bGuardStartRestartRequested =
		bIsGuardStarting &&
		bWasAlreadyStarting &&
		GuardStartRestartSerial != LastGuardStartRestartSerial;
	bGuardStartRepeatRequested = bGuardStartRestartRequested;
	LastGuardStartRestartSerial = GuardStartRestartSerial;
	LastDefenseState = DefenseState;

	const bool bStartedMovingThisFrame = !bHadMoveInputLastFrame && bHasMoveInputThisFrame;
	const bool bShouldTriggerLockOnForwardRunStart =
		bStartedMovingThisFrame &&
		bIsLockOn &&
		Player->ShouldUseRunningAnim() &&
		Player->GetDesiredMoveForward() >= LockOnForwardRunStartForwardThreshold &&
		FMath::Abs(Player->GetDesiredMoveRight()) <= LockOnForwardRunStartSideTolerance;

	if (bShouldTriggerLockOnForwardRunStart)
	{
		LockOnForwardRunStartTriggerRemainTime = LockOnForwardRunStartTriggerDuration;
	}
	else if (LockOnForwardRunStartTriggerRemainTime > 0.f)
	{
		LockOnForwardRunStartTriggerRemainTime = FMath::Max(
			0.f,
			LockOnForwardRunStartTriggerRemainTime - DeltaSeconds
		);
	}

	bShouldPlayLockOnForwardRunStart = LockOnForwardRunStartTriggerRemainTime > 0.f;
	bHadMoveInputLastFrame = bHasMoveInputThisFrame;
}

void UJunPlayerAnimInstance::UpdateMovementDirectionData(float DeltaSeconds)
{
	AJunPlayer* Player = Cast<AJunPlayer>(Character);

	if (!Player)
	{
		Super::UpdateMovementDirectionData(DeltaSeconds);
		return;
	}

	const float OriginalInterpSpeed = MovementDirectionInterpSpeed;

	if (Player->GetGuardMoveBlendRemainTime() > 0.f)
	{
		MovementDirectionInterpSpeed = GuardMovementDirectionInterpSpeed;
	}

	Super::UpdateMovementDirectionData(DeltaSeconds);

	MovementDirectionInterpSpeed = OriginalInterpSpeed;
}
