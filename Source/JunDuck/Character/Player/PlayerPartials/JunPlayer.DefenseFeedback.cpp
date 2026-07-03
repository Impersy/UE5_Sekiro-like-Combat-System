#include "Character/Player/JunPlayer.h"

#include "Camera/CameraShakeBase.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "System/JunCombatVFXSubsystem.h"
#include "Engine/World.h"
#pragma region Defense VFX / Feedback
// ============================================================================
// Defense VFX / Feedback
// ============================================================================

void AJunPlayer::CacheDefenseEffectComponents()
{
	CachedParryEffectLocationComponent = FindSceneComponentByName(ParryEffectLocationComponentName);
	CachedDeflectEffectLocationComponent = FindSceneComponentByName(DeflectEffectLocationComponentName);
	CachedGuardBreakEffectLocationComponent = FindSceneComponentByName(GuardBreakEffectLocationComponentName);
}

USceneComponent* AJunPlayer::FindSceneComponentByName(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == ComponentName)
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void AJunPlayer::PlayDefenseEffect(
	EJunCombatDefenseVFX EffectType,
	TObjectPtr<USceneComponent>& CachedLocationComponent,
	FName LocationComponentName)
{
	if (!CachedLocationComponent)
	{
		CachedLocationComponent = FindSceneComponentByName(LocationComponentName);
	}

	if (UJunCombatVFXSubsystem* VFXSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UJunCombatVFXSubsystem>() : nullptr)
	{
		const USceneComponent* SpawnLocationComponent = CachedLocationComponent ? CachedLocationComponent.Get() : GetMesh();
		VFXSubsystem->SpawnDefenseEffect(EffectType, SpawnLocationComponent);
	}
}

void AJunPlayer::PlayDefenseCameraShake(float Scale)
{
	if (!DefenseCameraShakeClass || Scale <= 0.f)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PlayerController)
	{
		PlayerController->ClientStartCameraShake(
			DefenseCameraShakeClass,
			Scale,
			ECameraShakePlaySpace::CameraLocal,
			FRotator::ZeroRotator);
	}
}

#pragma endregion
