#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HatActor.generated.h"

UCLASS()
class JUNDUCK_API AHatActor : public AActor
{
	GENERATED_BODY()

public:
	AHatActor();

	UFUNCTION(BlueprintCallable)
	UStaticMeshComponent* GetHatMesh() const { return HatMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hat")
	TObjectPtr<USceneComponent> HatRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Hat")
	TObjectPtr<UStaticMeshComponent> HatMesh;
};
