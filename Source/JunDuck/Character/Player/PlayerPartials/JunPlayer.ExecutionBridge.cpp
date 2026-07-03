#include "Character/Player/JunPlayer.h"

#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"
#pragma region Execution
// ============================================================================
// Execution
// ============================================================================

bool AJunPlayer::TryStartExecution()
{
	return ExecutionComponent && ExecutionComponent->TryStartExecution();
}

void AJunPlayer::InterruptActionsForExecution()
{
	InterruptActionsForHitReaction(FMath::Max(0.f, ExecutionActionCancelBlendOutTime));
}

void AJunPlayer::TriggerExecutionTargetMontage()
{
	if (ExecutionComponent)
	{
		ExecutionComponent->TriggerExecutionTargetMontage();
	}
}

void AJunPlayer::FinishExecution(bool bInterrupted)
{
	if (ExecutionComponent)
	{
		ExecutionComponent->FinishExecution(bInterrupted);
	}
}

void AJunPlayer::FinishDefenseForCancel()
{
	if (DefenseComponent)
	{
		DefenseComponent->FinishDefenseForCancel();
	}
}

#pragma endregion
