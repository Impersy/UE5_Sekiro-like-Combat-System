

#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "GameFramework/Actor.h"
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
	void StartAttackTrace();

	UFUNCTION(BlueprintCallable)
	void EndAttackTrace();

	UFUNCTION(BlueprintCallable)
	void StopAttackTrace();

	UFUNCTION(BlueprintCallable)
	void SetTraceSampleCount(int32 NewTraceSampleCount);

	UFUNCTION(BlueprintCallable)
	void SetAttackHitReactType(EHitReactType NewHitReactType);

	UFUNCTION(BlueprintCallable)
	void SetAttackDefenseKnockbackData(const FJunAttackDefenseKnockbackData& NewDefenseKnockbackData);

protected:
	void UpdateAttackTrace();
	void DrawAttackTraceDebug(const FVector& TraceStart, const FVector& TraceEnd, bool bSweepDebug, const FVector& PrevStart = FVector::ZeroVector, const FVector& PrevEnd = FVector::ZeroVector) const;

	void ApplyDamageToHitCharacter(AActor* HitActor, const FVector& SwingDirection);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trace")
	USceneComponent* TraceStartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trace")
	USceneComponent* TraceEndPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceRadius = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	int32 TraceSampleCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool bShowAttackTraceDebugAlways = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool bShowAttackTraceSweepDebug = true;

	bool bTraceActive = false;

	EHitReactType AttackHitReactType = EHitReactType::LightHit;

	FJunAttackDefenseKnockbackData AttackDefenseKnockbackData;

	FVector PrevTraceStart = FVector::ZeroVector;
	FVector PrevTraceEnd = FVector::ZeroVector;

	TSet<AActor*> HitActors;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BasicDamage = 1;
};
