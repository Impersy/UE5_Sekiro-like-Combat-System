

#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "GameFramework/Actor.h"
#include "Weapon/JunWeaponEffectTypes.h"
#include "WeaponActor.generated.h"

UCLASS()
class JUNDUCK_API AWeaponActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	FORCEINLINE UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

public:
	UFUNCTION(BlueprintCallable)
	void StartAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent = EJunWeaponNiagaraComponent::Trail);

	UFUNCTION(BlueprintCallable)
	void EndAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent = EJunWeaponNiagaraComponent::Trail);

	UFUNCTION(BlueprintCallable)
	void StopAttackTrace();

	UFUNCTION(BlueprintCallable)
	void SetTraceSampleCount(int32 NewTraceSampleCount);

	UFUNCTION(BlueprintCallable)
	void ApplyAttackTraceOverride(const FJunAttackTraceOverrideData& TraceOverrideData);

	UFUNCTION(BlueprintCallable)
	void ClearAttackTraceOverride();

	UFUNCTION(BlueprintCallable)
	void SetAttackHitReactType(EHitReactType NewHitReactType);

	UFUNCTION(BlueprintCallable)
	void SetAttackDamageData(const FJunAttackDamageData& NewDamageData);

	UFUNCTION(BlueprintCallable)
	void SetAttackDefenseKnockbackData(const FJunAttackDefenseKnockbackData& NewDefenseKnockbackData);

	UFUNCTION(BlueprintCallable)
	void SetAttackDefenseRuleData(const FJunAttackDefenseRuleData& NewDefenseRuleData);

	UFUNCTION(BlueprintCallable)
	void ActivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType);

	UFUNCTION(BlueprintCallable)
	void DeactivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType);

	UFUNCTION(BlueprintCallable)
	void DeactivateAllWeaponNiagara();

	UFUNCTION(BlueprintCallable)
	void SetWeaponEffectsEnabled(bool bEnabled);

protected:
	void UpdateAttackTrace();
	FVector GetCurrentTraceEndLocation(const FVector& CurrentTraceStart) const;
	float GetCurrentTraceRadius() const;
	int32 GetCurrentTraceSampleCount(const FVector& CurrentTraceStart, const FVector& CurrentTraceEnd) const;
	void DrawAttackTraceDebug(const FVector& TraceStart, const FVector& TraceEnd, bool bSweepDebug, const FVector& PrevStart = FVector::ZeroVector, const FVector& PrevEnd = FVector::ZeroVector) const;

	void ApplyDamageToHitCharacter(const FHitResult& HitResult, const FVector& SwingDirection);
	class UNiagaraComponent* GetWeaponNiagaraComponent(EJunWeaponNiagaraComponent ComponentType) const;
	class UNiagaraComponent* FindNiagaraComponentByName(FName ComponentName) const;
	void CacheWeaponNiagaraComponents();
	void WarmupWeaponNiagaraComponents();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trace")
	USceneComponent* TraceStartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trace")
	USceneComponent* TraceEndPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trail")
	TObjectPtr<USceneComponent> TrailStartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trail")
	TObjectPtr<USceneComponent> TrailEndPoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName TrailNiagaraComponentName = TEXT("NS_Trail");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName SpecialTrailNiagaraComponentName = TEXT("NS_SP_Trail");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName AuraNiagaraComponentName = TEXT("NS_Aura");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName JigenNiagaraComponentName = TEXT("NS_Jigen");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName LightingAuraNiagaraComponentName = TEXT("Lighting_Aura");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName LightingTrailNiagaraComponentName = TEXT("Lighting_Trail");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName LightingSlashNiagaraComponentName = TEXT("Lighting_Slash");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName BloodTrailNiagaraComponentName = TEXT("Blood_Trail");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceRadius = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	int32 TraceSampleCount = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trace")
	bool bAttackTraceOverrideActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trace")
	FJunAttackTraceOverrideData ActiveAttackTraceOverrideData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool bShowAttackTraceDebugAlways = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool bShowAttackTraceSweepDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	bool bActivateTrailWithAttackTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Warmup")
	bool bWarmupWeaponNiagaraOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Warmup", meta = (ClampMin = "0"))
	float WeaponNiagaraWarmupDeactivateDelay = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX", meta = (ToolTip = "Fallback component used by direct StartAttackTrace calls. AnimNotifyState_AttackTrace can override this per notify."))
	EJunWeaponNiagaraComponent AttackTraceNiagaraComponent = EJunWeaponNiagaraComponent::Trail;

	EJunWeaponNiagaraComponent ActiveAttackTraceNiagaraComponent = EJunWeaponNiagaraComponent::Trail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	bool bWeaponEffectsEnabled = true;

	bool bTraceActive = false;

	EHitReactType AttackHitReactType = EHitReactType::LightHit;

	FJunAttackDamageData AttackDamageData;

	FJunAttackDefenseKnockbackData AttackDefenseKnockbackData;

	FJunAttackDefenseRuleData AttackDefenseRuleData;

	FVector PrevTraceStart = FVector::ZeroVector;
	FVector PrevTraceEnd = FVector::ZeroVector;

	TSet<AActor*> HitActors;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedTrailNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedSpecialTrailNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedAuraNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedJigenNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedLightingAuraNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedLightingTrailNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedLightingSlashNiagaraComponent = nullptr;

	UPROPERTY(Transient)
	mutable TObjectPtr<class UNiagaraComponent> CachedBloodTrailNiagaraComponent = nullptr;

};
