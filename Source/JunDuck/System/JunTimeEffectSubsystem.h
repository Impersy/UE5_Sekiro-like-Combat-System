#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JunTimeEffectSubsystem.generated.h"

UENUM(BlueprintType)
enum class EJunTimeEffectType : uint8
{
	None,
	HitStop,
	SlowMotion
};

UCLASS()
class JUNDUCK_API UJunTimeEffectSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Jun|TimeEffect")
	bool RequestHitStop(
		float Duration = 0.04f,
		float TimeScale = 0.05f,
		int32 Priority = 0,
		bool bOverrideLowerOrEqualPriority = true);

	UFUNCTION(BlueprintCallable, Category = "Jun|TimeEffect")
	bool RequestHitStopForActors(
		const TArray<AActor*>& AffectedActors,
		float Duration = 0.04f,
		float TimeScale = 0.05f,
		int32 Priority = 0,
		bool bOverrideLowerOrEqualPriority = true);

	UFUNCTION(BlueprintCallable, Category = "Jun|TimeEffect")
	bool RequestSlowMotion(
		float Duration,
		float TimeScale,
		float BlendInTime = 0.f,
		float BlendOutTime = 0.f,
		int32 Priority = 0,
		bool bOverrideLowerOrEqualPriority = true);

	UFUNCTION(BlueprintCallable, Category = "Jun|TimeEffect")
	void CancelTimeEffect();

	UFUNCTION(BlueprintPure, Category = "Jun|TimeEffect")
	bool IsTimeEffectActive() const { return bTimeEffectActive; }

	UFUNCTION(BlueprintPure, Category = "Jun|TimeEffect")
	EJunTimeEffectType GetActiveTimeEffectType() const { return ActiveEffectType; }

private:
	bool CanStartEffect(int32 Priority, bool bOverrideLowerOrEqualPriority) const;
	bool StartEffect(
		EJunTimeEffectType EffectType,
		float Duration,
		float TimeScale,
		float BlendInTime,
		float BlendOutTime,
		int32 Priority,
		bool bOverrideLowerOrEqualPriority,
		const TArray<AActor*>& AffectedActors = TArray<AActor*>());
	void ApplyGlobalTimeScale(float TimeScale);
	void RestoreOriginalTimeScale();
	void ApplyActorTimeScale(float TimeScale);
	void RestoreActorTimeScales();

private:
	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	bool bTimeEffectActive = false;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	EJunTimeEffectType ActiveEffectType = EJunTimeEffectType::None;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	int32 ActivePriority = 0;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	float RequestedDuration = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	float RequestedTimeScale = 1.f;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	float RequestedBlendInTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	float RequestedBlendOutTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	float EffectStartRealTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Jun|TimeEffect")
	float OriginalGlobalTimeScale = 1.f;

	TMap<TWeakObjectPtr<AActor>, float> OriginalActorCustomTimeScales;
};
