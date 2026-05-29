#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/WukongBoss.h"
#include "AnimNotifyState_WukongLightningSwordAirMove.generated.h"

UCLASS()
class JUNDUCK_API UAnimNotifyState_WukongLightningSwordAirMove : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|LightningSword")
	EWukongLightningSwordAirMoveMode MoveMode = EWukongLightningSwordAirMoveMode::LiftAndHover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|LightningSword", meta = (ClampMin = "0.0"))
	float TargetHeightOffset = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|LightningSword", meta = (ClampMin = "0.0"))
	float RiseSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|LightningSword", meta = (ClampMin = "0.0"))
	float DiveSpeed = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|LightningSword", meta = (ClampMin = "0.0"))
	float DiveGravityScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wukong|LightningSword")
	bool bLockHorizontalVelocity = true;

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
