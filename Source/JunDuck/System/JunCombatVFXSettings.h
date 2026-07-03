#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JunCombatVFXSettings.generated.h"

class UNiagaraSystem;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Jun Combat VFX"))
class JUNDUCK_API UJunCombatVFXSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, Category = "Defense")
	TSoftObjectPtr<UNiagaraSystem> PerfectParry;

	UPROPERTY(Config, EditAnywhere, Category = "Defense")
	TSoftObjectPtr<UNiagaraSystem> NormalParry;

	UPROPERTY(Config, EditAnywhere, Category = "Defense")
	TSoftObjectPtr<UNiagaraSystem> GuardHit;

	UPROPERTY(Config, EditAnywhere, Category = "Defense")
	TSoftObjectPtr<UNiagaraSystem> GuardBreak;

	UPROPERTY(Config, EditAnywhere, Category = "Blood")
	TSoftObjectPtr<UNiagaraSystem> SplashMetal;

	UPROPERTY(Config, EditAnywhere, Category = "Blood")
	TSoftObjectPtr<UNiagaraSystem> SplashBigMetal;

	UPROPERTY(Config, EditAnywhere, Category = "Blood")
	TSoftObjectPtr<UNiagaraSystem> SplashBurstMetal;

	UPROPERTY(Config, EditAnywhere, Category = "Blood", meta = (ClampMin = "0.0"))
	float BloodScaleMultiplier = 1.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Trace", meta = (ClampMin = "0.0"))
	float BloodRetraceBackDistance = 100.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Trace", meta = (ClampMin = "0.0"))
	float BloodRetraceForwardDistance = 250.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Trace", meta = (ClampMin = "0.0"))
	float BloodSurfaceOffset = 0.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Trace", meta = (ClampMin = "0.0"))
	float PhysicalHitBloodSurfaceOffset = 10.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Trace", meta = (ClampMin = "0.0"))
	float BloodSurfaceInwardOffset = 20.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Trace", meta = (ClampMin = "0.0"))
	float PhysicalHitBloodSurfaceInwardOffset = 0.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Placement")
	FName BloodAnchorBoneName = TEXT("spine_04");

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Placement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BloodAnchorPullAlpha = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Placement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhysicalHitBloodAnchorPullAlpha = 0.f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Attachment", meta = (ClampMin = "0.0"))
	float BloodDetachDelay = 0.4f;

	UPROPERTY(Config, EditAnywhere, Category = "Blood|Rotation")
	float BloodYawOffset = 0.f;
};
