#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JunExecutionTargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UJunExecutionTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class JUNDUCK_API IJunExecutionTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool IsExecutionReady() const = 0;
	virtual bool CanBeExecutedBy(const class AJunPlayer* Player) const = 0;
	virtual bool TryBeginExecutionBy(class AJunPlayer* Player) = 0;
	virtual void TriggerPendingExecutionMontage(bool bApplyResultImmediately = true) = 0;
	virtual void ApplyPendingExecutionResult() = 0;
	virtual void CancelPendingExecution() = 0;
	virtual bool HasExecutionResultApplied() const = 0;
	virtual bool IsFinalExecution() const = 0;
	virtual FVector GetExecutionCameraTargetPoint() const = 0;
	virtual bool ShouldIgnoreExecutorCapsuleCollisionDuringExecution() const = 0;
	virtual void ReduceExecutionCapsuleRadius() = 0;
	virtual void RestoreExecutionCapsuleRadius() = 0;
};
