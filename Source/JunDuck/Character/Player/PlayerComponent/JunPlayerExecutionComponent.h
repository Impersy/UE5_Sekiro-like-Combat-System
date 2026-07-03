#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JunPlayerExecutionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JUNDUCK_API UJunPlayerExecutionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunPlayerExecutionComponent();

	virtual void BeginPlay() override;

	bool TryStartExecution();
	bool IsExecuting() const { return bIsExecuting; }
	AActor* GetCurrentExecutionTarget() const { return CurrentExecutionTarget.Get(); }
	class UAnimMontage* GetCurrentExecutionMontage() const { return CurrentExecutionMontage.Get(); }

	void TriggerExecutionTargetMontage();
	void FinishExecution(bool bInterrupted = false);

private:
	bool CanStartExecution(const AActor* ExecutionTargetActor) const;
	void StartExecution(AActor* ExecutionTargetActor);
	void SetExecutionCapsuleCollisionIgnore(AActor* ExecutionTargetActor, bool bIgnore);
	void ReduceExecutionCapsuleRadius(AActor* ExecutionTargetActor);
	void RestoreExecutionCapsuleRadius(AActor* ExecutionTargetActor);
	class UAnimMontage* GetExecutionMontageForTarget(const class IJunExecutionTargetInterface* ExecutionTarget) const;

	UFUNCTION()
	void OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	TObjectPtr<class AJunPlayer> OwnerPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Execution")
	bool bIsExecuting = false;

	UPROPERTY(VisibleAnywhere, Category = "Execution")
	TObjectPtr<AActor> CurrentExecutionTarget = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Execution")
	TObjectPtr<class UAnimMontage> CurrentExecutionMontage = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Execution")
	float SavedExecutionCapsuleRadius = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Execution")
	bool bExecutionCapsuleRadiusReduced = false;
};
