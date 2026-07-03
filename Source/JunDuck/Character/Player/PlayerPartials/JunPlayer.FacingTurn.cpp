#include "Character/Player/JunPlayer.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunAttackTargetInterface.h"
#include "Kismet/KismetMathLibrary.h"

float AJunPlayer::EvaluateFreeRunRotationRate() const
{
	const bool bUseCameraTurnRate = bFreeCameraYawInputActiveThisFrame;
	const float MinTurnRate = FMath::Max(
		0.f,
		bUseCameraTurnRate ? FreeRunCameraTurnMinRotationRate : FreeRunMinTurnRotationRate);
	const float MaxTurnRate = FMath::Max(
		MinTurnRate,
		bUseCameraTurnRate ? FreeRunCameraTurnMaxRotationRate : FreeRunMaxTurnRotationRate);
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || CameraMode != EJunCameraMode::Free ||
		bLockOnActive || !MovementComponent->bOrientRotationToMovement || !Controller)
	{
		return MaxTurnRate;
	}

	FVector2D MoveInput(DesiredMoveForward, DesiredMoveRight);
	if (MoveInput.IsNearlyZero())
	{
		MoveInput = FVector2D(PendingMoveForward, PendingMoveRight);
	}
	if (MoveInput.IsNearlyZero())
	{
		return MaxTurnRate;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Pitch = 0.f;
	ControlRotation.Roll = 0.f;
	const FVector DesiredWorldDirection =
		(UKismetMathLibrary::GetForwardVector(ControlRotation) * MoveInput.X +
		 UKismetMathLibrary::GetRightVector(ControlRotation) * MoveInput.Y).GetSafeNormal2D();
	if (DesiredWorldDirection.IsNearlyZero())
	{
		return MaxTurnRate;
	}

	const float TurnAngle = FMath::Abs(FMath::FindDeltaAngleDegrees(
		GetActorRotation().Yaw,
		DesiredWorldDirection.Rotation().Yaw));
	const float AngleAlpha = FMath::Clamp(TurnAngle / 180.f, 0.f, 1.f);
	const float CurvedAlpha = FMath::Pow(AngleAlpha, FMath::Max(0.01f, FreeRunTurnAngleExponent));
	return FMath::Lerp(MinTurnRate, MaxTurnRate, CurvedAlpha);
}

UAnimMontage* AJunPlayer::EvaluateLockOnTurnMontage() const
{
	return CanStartLockOnTurn()
		? ChooseLockOnTurnMontage(GetLockOnTargetYawDelta())
		: nullptr;
}

bool AJunPlayer::TryResolveAttackFacingTargetRotation(FRotator& OutTargetRotation)
{
	if (!TargetActor)
	{
		TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();
	}
	if (!TargetActor)
	{
		EndAttackFacingWindow();
		return false;
	}

	const IJunAttackTargetInterface* AttackTarget = Cast<IJunAttackTargetInterface>(TargetActor.Get());
	const FVector TargetPoint = AttackTarget
		? AttackTarget->GetAttackTargetPoint(this)
		: TargetActor->GetActorLocation();
	FVector Direction = TargetPoint - GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OutTargetRotation = Direction.Rotation();
	return true;
}

bool AJunPlayer::TryResolveHitReactFacingTargetRotation(FRotator& OutTargetRotation)
{
	if (!CanUseHitReactFacingWindow() || !HitReactFacingTarget)
	{
		EndHitReactFacingWindow();
		return false;
	}

	FVector Direction = HitReactFacingTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OutTargetRotation = Direction.Rotation();
	return true;
}

void AJunPlayer::ApplyPlayerFacingRotation(
	const FRotator& TargetRotation,
	float DeltaTime,
	float InterpSpeed)
{
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		TargetRotation,
		DeltaTime,
		InterpSpeed));
}
