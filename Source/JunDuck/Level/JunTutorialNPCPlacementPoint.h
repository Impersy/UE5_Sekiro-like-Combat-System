#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JunTutorialNPCPlacementPoint.generated.h"

class AJunTutorialNPC;
class UArrowComponent;
class USceneComponent;

UCLASS()
class JUNDUCK_API AJunTutorialNPCPlacementPoint : public AActor
{
	GENERATED_BODY()

public:
	AJunTutorialNPCPlacementPoint();

	UFUNCTION(BlueprintPure, Category = "TutorialNPCPlacement")
	FTransform GetPlacementTransform() const { return GetActorTransform(); }

	UFUNCTION(BlueprintPure, Category = "TutorialNPCPlacement")
	FName GetPlacementTag() const { return PlacementTag; }

	UFUNCTION(BlueprintCallable, Category = "TutorialNPCPlacement")
	bool MoveNPCToPlacement(AJunTutorialNPC* NPC) const;

	UFUNCTION(BlueprintCallable, Category = "TutorialNPCPlacement", meta = (WorldContext = "WorldContextObject"))
	static AJunTutorialNPCPlacementPoint* FindTutorialNPCPlacementPoint(
		const UObject* WorldContextObject,
		FName RequiredTag = NAME_None);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPCPlacement")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TutorialNPCPlacement")
	TObjectPtr<UArrowComponent> FacingArrow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TutorialNPCPlacement")
	FName PlacementTag = NAME_None;
};
