#include "Combat/JunCombatHitProcessor.h"

#include "Interface/JunCombatHitTargetInterface.h"
#include "System/JunCombatVFXSubsystem.h"

FJunAttackHitResult FJunCombatHitProcessor::ProcessHit(
	AJunCharacter& Target,
	const FJunAttackHitContext& Context)
{
	IJunCombatHitTargetInterface* CombatHitTarget = Cast<IJunCombatHitTargetInterface>(&Target);
	if (!ensure(CombatHitTarget))
	{
		return FJunAttackHitResult{};
	}

	const FJunAttackHitResult Result = CombatHitTarget->ProcessAttackHit(Context);
	ProcessCommonFeedback(Target, Context, Result);
	return Result;
}

void FJunCombatHitProcessor::ProcessCommonFeedback(
	AJunCharacter& Target,
	const FJunAttackHitContext& Context,
	const FJunAttackHitResult& Result)
{
	if (!Result.ShouldSpawnBlood())
	{
		return;
	}

	UWorld* World = Target.GetWorld();
	UJunCombatVFXSubsystem* VFXSubsystem = World ? World->GetSubsystem<UJunCombatVFXSubsystem>() : nullptr;
	if (!VFXSubsystem)
	{
		return;
	}

	const FVector AttackerLocation = Context.Attacker
		? Context.Attacker->GetActorLocation()
		: Target.GetActorLocation() - Context.SwingDirection;
	VFXSubsystem->SpawnBloodImpact(
		Target.GetMesh(),
		Context.HitResult,
		Context.SwingDirection,
		AttackerLocation,
		Result.bPhysicalReactionOnly);
}
