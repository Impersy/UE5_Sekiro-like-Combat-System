#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JunCombatVFXSubsystem.generated.h"

class UNiagaraSystem;

UCLASS()
class JUNDUCK_API UJunCombatVFXSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
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
};
