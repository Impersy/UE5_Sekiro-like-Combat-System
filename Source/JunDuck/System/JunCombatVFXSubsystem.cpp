#include "System/JunCombatVFXSubsystem.h"

#include "NiagaraFunctionLibrary.h"

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
