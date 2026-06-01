#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JunBGMManager.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class JUNDUCK_API AJunBGMManager : public AActor
{
	GENERATED_BODY()

public:
	AJunBGMManager();

	virtual void BeginPlay() override;

	static AJunBGMManager* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "BGM")
	void PlayMapBGM();

	UFUNCTION(BlueprintCallable, Category = "BGM")
	void PlayCombatBGM(USoundBase* OverrideCombatBGM = nullptr);

	UFUNCTION(BlueprintCallable, Category = "BGM")
	void ReturnToMapBGM();

	UFUNCTION(BlueprintCallable, Category = "BGM")
	void StopAllBGM();

	UFUNCTION(BlueprintCallable, Category = "BGM|Ducking")
	void SetBGMDucked(bool bDucked);

	UFUNCTION(BlueprintCallable, Category = "BGM|Ducking")
	void SetExecutionBGMDucked(bool bDucked);

protected:
	void PlayBGMComponent(UAudioComponent* AudioComponent, USoundBase* Sound, float FadeInTime, float Volume);
	void FadeOutBGMComponent(UAudioComponent* AudioComponent, float FadeOutTime);
	void ApplyCurrentBGMVolume(float FadeTime);
	float GetCurrentBGMTargetVolume() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<UAudioComponent> MapBGMComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<UAudioComponent> CombatBGMComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Map")
	TObjectPtr<USoundBase> MapBGM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Combat")
	TObjectPtr<USoundBase> DefaultCombatBGM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0"))
	float BGMVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Ducking", meta = (ClampMin = "0"))
	float DuckedBGMVolume = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Ducking", meta = (ClampMin = "0"))
	float DuckFadeTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Ducking|Execution", meta = (ClampMin = "0"))
	float ExecutionDuckedBGMVolume = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Ducking|Execution", meta = (ClampMin = "0"))
	float ExecutionDuckFadeTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0"))
	float MapBGMFadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0"))
	float MapBGMFadeOutTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0"))
	float CombatBGMFadeInTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0"))
	float CombatBGMFadeOutTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	bool bPlayMapBGMOnBeginPlay = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BGM|Ducking")
	bool bBGMDucked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BGM|Ducking|Execution")
	bool bExecutionBGMDucked = false;
};
