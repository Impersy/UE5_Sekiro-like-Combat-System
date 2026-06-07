#include "Level/JunTutorialNPCPlacementPoint.h"

#include "Character/JunTutorialNPC.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AJunTutorialNPCPlacementPoint::AJunTutorialNPCPlacementPoint()
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

bool AJunTutorialNPCPlacementPoint::MoveNPCToPlacement(AJunTutorialNPC* NPC) const
{
	if (!NPC)
	{
		return false;
	}

	NPC->SetActorLocationAndRotation(
		GetActorLocation(),
		GetActorRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	return true;
}

AJunTutorialNPCPlacementPoint* AJunTutorialNPCPlacementPoint::FindTutorialNPCPlacementPoint(
	const UObject* WorldContextObject,
	FName RequiredTag)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AJunTutorialNPCPlacementPoint> It(World); It; ++It)
	{
		AJunTutorialNPCPlacementPoint* Point = *It;
		if (!Point)
		{
			continue;
		}

		if (!RequiredTag.IsNone() && Point->PlacementTag != RequiredTag)
		{
			continue;
		}

		return Point;
	}

	return nullptr;
}
