#include "Camera/JunSpringArmComponent.h"

UJunSpringArmComponent::UJunSpringArmComponent()
{
}

void UJunSpringArmComponent::ResetCameraSmoothing()
{
	bCollisionBlendInitialized = false;
	SmoothedArmLength = 0.f;
}

FVector UJunSpringArmComponent::BlendLocations(
	const FVector& DesiredArmLocation,
	const FVector& TraceHitLocation,
	bool bHitSomething,
	float DeltaTime
)
{
	const FVector TargetArmLocation = bHitSomething ? TraceHitLocation : DesiredArmLocation;
	const FVector ArmOrigin = GetComponentLocation();
	const FVector DesiredArmOffset = DesiredArmLocation - ArmOrigin;
	const FVector DesiredArmDirection = DesiredArmOffset.GetSafeNormal();
	const float TargetCollisionArmLength = FVector::Dist(ArmOrigin, TargetArmLocation);

	if (DesiredArmDirection.IsNearlyZero())
	{
		bCollisionBlendInitialized = true;
		SmoothedArmLength = TargetCollisionArmLength;
		return TargetArmLocation;
	}

	if (!bEnableCollisionArmSmoothing || DeltaTime <= 0.f)
	{
		bCollisionBlendInitialized = true;
		SmoothedArmLength = TargetCollisionArmLength;
		return ArmOrigin + (DesiredArmDirection * TargetCollisionArmLength);
	}

	if (!bCollisionBlendInitialized)
	{
		bCollisionBlendInitialized = true;
		SmoothedArmLength = TargetCollisionArmLength;
		return ArmOrigin + (DesiredArmDirection * TargetCollisionArmLength);
	}

	const bool bMovingInward = TargetCollisionArmLength < SmoothedArmLength;
	const float InterpSpeed = bMovingInward ? CollisionArmInterpSpeedIn : CollisionArmInterpSpeedOut;

	SmoothedArmLength = InterpSpeed > 0.f
		? FMath::FInterpTo(SmoothedArmLength, TargetCollisionArmLength, DeltaTime, InterpSpeed)
		: TargetCollisionArmLength;

	return ArmOrigin + (DesiredArmDirection * SmoothedArmLength);
}
