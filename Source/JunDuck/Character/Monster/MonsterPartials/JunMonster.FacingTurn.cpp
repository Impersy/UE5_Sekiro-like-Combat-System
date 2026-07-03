#include "Character/Monster/JunMonster.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

bool AJunMonster::CanStartTurnInternal(bool bRequireCombatState) const
{
	if (!CurrentTarget || bCombatTurnInProgress ||
		bIsAttacking || IsInHitReact() || CurrentState == EMonsterState::Dead)
	{
		return false;
	}

	if (bRequireCombatState &&
		CurrentState != EMonsterState::BattleStart &&
		CurrentState != EMonsterState::Combat)
	{
		return false;
	}

	if (const UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (MonsterAnimInstance->GetCurrentActiveMontage() != nullptr)
		{
			return false;
		}
	}

	return GetVelocity().Size2D() <= CombatTurnMaxGroundSpeed;
}

bool AJunMonster::StartCombatTurnMontage(
	float YawDelta,
	bool bSetPendingState,
	EMonsterState PendingState)
{
	UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* TurnMontage = ChooseCombatTurnMontage(YawDelta);
	if (!MonsterAnimInstance || !TurnMontage)
	{
		return false;
	}

	StopMovementForCombatTurn();
	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);

	LastCombatTurnStartYaw = GetActorRotation().Yaw;
	if (MonsterAnimInstance->Montage_Play(TurnMontage, CombatTurnPlayRate) <= 0.f)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
		return false;
	}

	LastCombatTurnPostPlayYaw = GetActorRotation().Yaw;
	if (bDebugCombatTurnYaw)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[MonsterTurn][Start] Play=%s YawDelta=%.2f Before=%.2f AfterPlay=%.2f AfterPlayDelta=%.2f Velocity=%.2f State=%d Pending=%d"),
			*GetNameSafe(TurnMontage),
			YawDelta,
			LastCombatTurnStartYaw,
			LastCombatTurnPostPlayYaw,
			FMath::FindDeltaAngleDegrees(LastCombatTurnStartYaw, LastCombatTurnPostPlayYaw),
			GetVelocity().Size2D(),
			static_cast<int32>(CurrentState),
			bSetPendingState ? 1 : 0);
	}

	CurrentCombatTurnMontage = TurnMontage;
	bCombatTurnInProgress = true;
	bHasPendingStateAfterCombatTurn = bSetPendingState;
	PendingStateAfterCombatTurn = bSetPendingState ? PendingState : EMonsterState::Idle;
	return true;
}

void AJunMonster::StopMovementForCombatTurn()
{
	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	EndAttackFacingWindow();
	AttackFacingRemainTime = 0.f;
}

bool AJunMonster::TryGetCombatFacingTargetRotation(FRotator& OutTargetRotation) const
{
	if (!CurrentTarget)
	{
		return false;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	OutTargetRotation = ToTarget.Rotation();
	return true;
}
