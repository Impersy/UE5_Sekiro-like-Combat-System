#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "GameFramework/Actor.h"
#include "ArrowProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class JUNDUCK_API AArrowProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArrowProjectile();

	void InitializeArrow(
		AJunCharacter* InOwnerCharacter,
		EHitReactType InHitReactType,
		const FJunAttackDamageData& InDamageData,
		const FJunAttackDefenseKnockbackData& InDefenseKnockbackData);

	void Fire(
		const FVector& Direction,
		float Speed,
		float LifeSeconds,
		AActor* HomingTarget = nullptr,
		float HomingDuration = 0.f,
		float HomingInterpSpeed = 0.f,
		float HomingTargetHeightOffset = 50.f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnArrowOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<UStaticMeshComponent> ArrowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	float CollisionRadius = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<AJunCharacter> OwnerCharacter;

	EHitReactType HitReactType = EHitReactType::LightHit;
	FJunAttackDamageData DamageData;
	FJunAttackDefenseKnockbackData DefenseKnockbackData;
	TWeakObjectPtr<AActor> CurrentHomingTarget;
	float CurrentSpeed = 0.f;
	float HomingRemainTime = 0.f;
	float HomingTurnInterpSpeed = 0.f;
	float HomingAimHeightOffset = 50.f;
	bool bHasHit = false;
};
