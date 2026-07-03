#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "JunSpringArmComponent.generated.h"

UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent))
class JUNDUCK_API UJunSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UJunSpringArmComponent();
	void ResetCameraSmoothing();

protected:
	virtual FVector BlendLocations(
		const FVector& DesiredArmLocation,
		const FVector& TraceHitLocation,
		bool bHitSomething,
		float DeltaTime
	) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Collision")
	bool bEnableCollisionArmSmoothing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Collision", meta = (ClampMin = "0.0"))
	float CollisionArmInterpSpeedIn = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Collision", meta = (ClampMin = "0.0"))
	float CollisionArmInterpSpeedOut = 3.f;

private:
	bool bCollisionBlendInitialized = false;
	float SmoothedArmLength = 0.f;
};
