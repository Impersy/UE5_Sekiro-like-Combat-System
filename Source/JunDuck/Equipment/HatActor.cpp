#include "Equipment/HatActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AHatActor::AHatActor()
{
	PrimaryActorTick.bCanEverTick = false;

	HatRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HatRoot"));
	RootComponent = HatRoot;

	HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatMesh"));
	HatMesh->SetupAttachment(HatRoot);
	HatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HatMesh->SetGenerateOverlapEvents(false);
}
