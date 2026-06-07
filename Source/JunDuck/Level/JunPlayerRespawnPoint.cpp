#include "Level/JunPlayerRespawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AJunPlayerRespawnPoint::AJunPlayerRespawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(SceneRoot);
	FacingArrow->ArrowSize = 1.5f;
	FacingArrow->ArrowLength = 120.f;
	FacingArrow->SetRelativeRotation(FRotator::ZeroRotator);
}

AJunPlayerRespawnPoint* AJunPlayerRespawnPoint::FindPlayerRespawnPoint(
	const UObject* WorldContextObject,
	EJunPlayerRespawnPointType PointType,
	FName RequiredTag)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AJunPlayerRespawnPoint> It(World); It; ++It)
	{
		AJunPlayerRespawnPoint* Point = *It;
		if (!Point || Point->RespawnPointType != PointType)
		{
			continue;
		}

		if (!RequiredTag.IsNone() && Point->RespawnPointTag != RequiredTag)
		{
			continue;
		}

		return Point;
	}

	return nullptr;
}

bool AJunPlayerRespawnPoint::MoveActorToRespawnPoint(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	Actor->SetActorLocationAndRotation(
		GetActorLocation(),
		GetActorRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	return true;
}
