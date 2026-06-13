#include "System/JunCombatVFXSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "System/JunCombatVFXSettings.h"
#include "TimerManager.h"

void UJunCombatVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	BloodNiagaraSystems.Reset();
	const UJunCombatVFXSettings* Settings = GetDefault<UJunCombatVFXSettings>();
	if (!Settings)
	{
		return;
	}

	const TSoftObjectPtr<UNiagaraSystem> BloodAssets[] =
	{
		Settings->SplashMetal,
		Settings->SplashBigMetal,
		Settings->SplashBurstMetal
	};

	for (const TSoftObjectPtr<UNiagaraSystem>& BloodAsset : BloodAssets)
	{
		if (UNiagaraSystem* NiagaraSystem = BloodAsset.LoadSynchronous())
		{
			BloodNiagaraSystems.Add(NiagaraSystem);
		}
	}

	BloodScaleMultiplier = FMath::Max(0.f, Settings->BloodScaleMultiplier);
	BloodRetraceBackDistance = FMath::Max(0.f, Settings->BloodRetraceBackDistance);
	BloodRetraceForwardDistance = FMath::Max(0.f, Settings->BloodRetraceForwardDistance);
	BloodSurfaceOffset = FMath::Max(0.f, Settings->BloodSurfaceOffset);
	PhysicalHitBloodSurfaceOffset = FMath::Max(0.f, Settings->PhysicalHitBloodSurfaceOffset);
	BloodSurfaceInwardOffset = FMath::Max(0.f, Settings->BloodSurfaceInwardOffset);
	PhysicalHitBloodSurfaceInwardOffset = FMath::Max(0.f, Settings->PhysicalHitBloodSurfaceInwardOffset);
	BloodAnchorBoneName = Settings->BloodAnchorBoneName;
	BloodAnchorPullAlpha = FMath::Clamp(Settings->BloodAnchorPullAlpha, 0.f, 1.f);
	PhysicalHitBloodAnchorPullAlpha = FMath::Clamp(Settings->PhysicalHitBloodAnchorPullAlpha, 0.f, 1.f);
	BloodDetachDelay = FMath::Max(0.f, Settings->BloodDetachDelay);
	BloodYawOffset = Settings->BloodYawOffset;
}

void UJunCombatVFXSubsystem::SpawnCombatNiagara(
	UNiagaraSystem* NiagaraSystem,
	const FTransform& SpawnTransform,
	float ScaleMultiplier,
	bool bAutoDestroy)
{
	UWorld* World = GetWorld();
	if (!World || !NiagaraSystem)
	{
		return;
	}

	FVector Scale = SpawnTransform.GetScale3D();
	Scale *= FMath::Max(0.f, ScaleMultiplier);

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		NiagaraSystem,
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		Scale,
		bAutoDestroy,
		true);
}

void UJunCombatVFXSubsystem::SpawnCombatNiagaraAtComponent(
	UNiagaraSystem* NiagaraSystem,
	const USceneComponent* LocationComponent,
	float ScaleMultiplier,
	bool bAutoDestroy)
{
	if (!LocationComponent)
	{
		return;
	}

	SpawnCombatNiagara(
		NiagaraSystem,
		LocationComponent->GetComponentTransform(),
		ScaleMultiplier,
		bAutoDestroy);
}

void UJunCombatVFXSubsystem::SpawnBloodImpact(
	USkeletalMeshComponent* VictimMesh,
	const FHitResult& InitialHit,
	const FVector& SwingDirection,
	const FVector& AttackerLocation,
	bool bPhysicalHitOnly)
{
	UWorld* World = GetWorld();
	if (!World || !VictimMesh || BloodNiagaraSystems.IsEmpty())
	{
		return;
	}

	UNiagaraSystem* BloodSystem = BloodNiagaraSystems[FMath::RandRange(0, BloodNiagaraSystems.Num() - 1)];
	if (!BloodSystem)
	{
		return;
	}

	FVector TraceDirection = SwingDirection.GetSafeNormal();
	const FVector AttackerToVictim = (VictimMesh->GetComponentLocation() - AttackerLocation).GetSafeNormal();
	if (TraceDirection.IsNearlyZero() ||
		(!AttackerToVictim.IsNearlyZero() && FVector::DotProduct(TraceDirection, AttackerToVictim) < 0.1f))
	{
		TraceDirection = AttackerToVictim;
	}
	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = VictimMesh->GetForwardVector();
	}

	const FVector InitialImpactLocation = InitialHit.bBlockingHit
		? InitialHit.ImpactPoint
		: InitialHit.Location;
	const FVector TraceStart = InitialImpactLocation - TraceDirection * BloodRetraceBackDistance;
	const FVector TraceEnd = InitialImpactLocation + TraceDirection * BloodRetraceForwardDistance;

	FHitResult MeshHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JunBloodMeshRetrace), false);
	QueryParams.bReturnPhysicalMaterial = false;
	const bool bMeshHit = VictimMesh->LineTraceComponent(MeshHit, TraceStart, TraceEnd, QueryParams);

	const float ActiveSurfaceOffset = bPhysicalHitOnly
		? PhysicalHitBloodSurfaceOffset
		: BloodSurfaceOffset;
	const float ActiveSurfaceInwardOffset = bPhysicalHitOnly
		? PhysicalHitBloodSurfaceInwardOffset
		: BloodSurfaceInwardOffset;
	FVector SurfaceLocation = bMeshHit
		? MeshHit.ImpactPoint + MeshHit.ImpactNormal.GetSafeNormal() * (ActiveSurfaceOffset - ActiveSurfaceInwardOffset)
		: InitialImpactLocation;
	const FName BoneName = bMeshHit ? MeshHit.BoneName : InitialHit.BoneName;
	const float ActiveAnchorPullAlpha = bPhysicalHitOnly
		? PhysicalHitBloodAnchorPullAlpha
		: BloodAnchorPullAlpha;
	if (!BloodAnchorBoneName.IsNone() &&
		ActiveAnchorPullAlpha > 0.f &&
		VictimMesh->GetBoneIndex(BloodAnchorBoneName) != INDEX_NONE)
	{
		SurfaceLocation = FMath::Lerp(
			SurfaceLocation,
			VictimMesh->GetBoneLocation(BloodAnchorBoneName),
			ActiveAnchorPullAlpha);
	}

	FVector EffectDirection = SwingDirection.GetSafeNormal();
	if (EffectDirection.IsNearlyZero())
	{
		EffectDirection = TraceDirection;
	}
	EffectDirection.Z = 0.f;
	const float SpawnYaw = (EffectDirection.IsNearlyZero() ? TraceDirection.Rotation().Yaw : EffectDirection.Rotation().Yaw) + BloodYawOffset;

	UNiagaraComponent* BloodComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		BloodSystem,
		VictimMesh,
		BoneName,
		SurfaceLocation,
		FRotator(0.f, SpawnYaw, 0.f),
		FVector(BloodScaleMultiplier),
		EAttachLocation::KeepWorldPosition,
		true,
		ENCPoolMethod::None,
		true,
		true);

	if (!BloodComponent || BloodDetachDelay <= 0.f)
	{
		if (BloodComponent)
		{
			BloodComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakBloodComponent = BloodComponent;
	FTimerDelegate DetachDelegate;
	DetachDelegate.BindLambda([WeakBloodComponent]()
	{
		if (UNiagaraComponent* Component = WeakBloodComponent.Get())
		{
			Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}
	});

	FTimerHandle DetachTimerHandle;
	World->GetTimerManager().SetTimer(DetachTimerHandle, DetachDelegate, BloodDetachDelay, false);
}
