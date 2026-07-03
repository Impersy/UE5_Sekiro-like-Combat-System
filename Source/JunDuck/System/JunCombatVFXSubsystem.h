#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JunCombatVFXSubsystem.generated.h"

class UNiagaraSystem;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EJunCombatDefenseVFX : uint8
{
	PerfectParry,
	NormalParry,
	GuardHit,
	GuardBreak
};

UCLASS()
class JUNDUCK_API UJunCombatVFXSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Jun|CombatVFX")
	void SpawnCombatNiagara(
		UNiagaraSystem* NiagaraSystem,
		const FTransform& SpawnTransform,
		float ScaleMultiplier = 1.f,
		bool bAutoDestroy = true);

	UFUNCTION(BlueprintCallable, Category = "Jun|CombatVFX")
	void SpawnCombatNiagaraAtComponent(
		UNiagaraSystem* NiagaraSystem,
		const USceneComponent* LocationComponent,
		float ScaleMultiplier = 1.f,
		bool bAutoDestroy = true);

	UFUNCTION(BlueprintCallable, Category = "Jun|CombatVFX")
	void SpawnDefenseEffect(
		EJunCombatDefenseVFX EffectType,
		const USceneComponent* LocationComponent,
		float ScaleMultiplier = 1.f);

	void SpawnBloodImpact(
		USkeletalMeshComponent* VictimMesh,
		const FHitResult& InitialHit,
		const FVector& SwingDirection,
		const FVector& AttackerLocation,
		bool bPhysicalHitOnly = false);

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> PerfectParryNiagaraSystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> NormalParryNiagaraSystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> GuardHitNiagaraSystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> GuardBreakNiagaraSystem = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraSystem>> BloodNiagaraSystems;

	float BloodScaleMultiplier = 1.f;
	float BloodRetraceBackDistance = 100.f;
	float BloodRetraceForwardDistance = 250.f;
	float BloodSurfaceOffset = 0.f;
	float PhysicalHitBloodSurfaceOffset = 10.f;
	float BloodSurfaceInwardOffset = 20.f;
	float PhysicalHitBloodSurfaceInwardOffset = 0.f;
	FName BloodAnchorBoneName = TEXT("spine_04");
	float BloodAnchorPullAlpha = 0.6f;
	float PhysicalHitBloodAnchorPullAlpha = 0.f;
	float BloodDetachDelay = 0.4f;
	float BloodYawOffset = 0.f;
};
