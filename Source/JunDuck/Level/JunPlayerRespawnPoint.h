#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JunPlayerRespawnPoint.generated.h"

class UArrowComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EJunPlayerRespawnPointType : uint8
{
	TutorialStart,
	BossRespawn
};

UCLASS()
class JUNDUCK_API AJunPlayerRespawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AJunPlayerRespawnPoint();

	UFUNCTION(BlueprintPure, Category = "RespawnPoint")
	FTransform GetRespawnTransform() const { return GetActorTransform(); }

	UFUNCTION(BlueprintPure, Category = "RespawnPoint")
	FName GetRespawnPointTag() const { return RespawnPointTag; }

	UFUNCTION(BlueprintPure, Category = "RespawnPoint")
	EJunPlayerRespawnPointType GetRespawnPointType() const { return RespawnPointType; }

	UFUNCTION(BlueprintCallable, Category = "RespawnPoint", meta = (WorldContext = "WorldContextObject"))
	static AJunPlayerRespawnPoint* FindPlayerRespawnPoint(
		const UObject* WorldContextObject,
		EJunPlayerRespawnPointType PointType,
		FName RequiredTag = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "RespawnPoint")
	bool MoveActorToRespawnPoint(AActor* Actor) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RespawnPoint")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RespawnPoint")
	TObjectPtr<UArrowComponent> FacingArrow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RespawnPoint")
	FName RespawnPointTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RespawnPoint")
	EJunPlayerRespawnPointType RespawnPointType = EJunPlayerRespawnPointType::BossRespawn;
};
