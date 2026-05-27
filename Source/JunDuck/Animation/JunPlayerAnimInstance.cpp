


#include "Animation/JunPlayerAnimInstance.h"
#include "Character/JunPlayer.h"

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
