#include "Character/Monster/JunMonster.h"

#include "AI/JunAIController.h"
#include "Character/Player/JunPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Weapon/ArrowProjectile.h"
#include "Weapon/WeaponActor.h"

namespace
{
	bool IsInvalidCombatTarget(const AActor* Target)
	{
		if (!Target)
		{
			return true;
		}

		if (const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(Target))
		{
			if (TargetCharacter->Is_Dead())
			{
				return true;
			}
		}

		if (const AJunPlayer* TargetPlayer = Cast<AJunPlayer>(Target))
		{
			if (TargetPlayer->IsInDeathSequence())
			{
				return true;
			}
		}

		return false;
	}
}

void AJunMonster::TryFindTarget()
{
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);

	if (!PlayerActor)
	{
		return;
	}

	AJunCharacter* TargetCharacter = Cast<AJunCharacter>(PlayerActor);

	if (!TargetCharacter)
	{
		return;
	}

	const bool bEnemy = IsEnemyTo(TargetCharacter);
	if (!bEnemy)
	{
		return;
	}

	const bool bDetect = CanDetectTarget(TargetCharacter);
	if (!bDetect)
	{
		return;
	}

	CurrentTarget = TargetCharacter;
}

bool AJunMonster::CanDetectTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(Target);
	if (TargetCharacter && TargetCharacter->Is_Dead())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.DetectRange);
}

bool AJunMonster::CanKeepTarget(AActor* Target) const
{
	if (IsInvalidCombatTarget(Target))
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.LoseTargetRange);
}

bool AJunMonster::CanRemainInCombat(AActor* Target) const
{
	if (IsInvalidCombatTarget(Target))
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.CombatBreakRange);
}

bool AJunMonster::CanAttackTarget() const
{
	if (bExecutionReady || bBeingExecuted)
	{
		return false;
	}

	if (CurrentState != EMonsterState::Combat)
	{
		return false;
	}

	const FMonsterAttackSelection AttackSelection = ChooseNextAttackSelection();
	if (IsInvalidCombatTarget(CurrentTarget) || !AttackSelection.Montage)
	{
		return false;
	}

	if (bIsAttacking || IsInHitReact() || Is_Dead() || IsCombatTurnPlaying())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return DistSq <= FMath::Square(AttackSelection.AttackRange);
}

void AJunMonster::ChooseNextPatrolLocation()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	APawn* ControlledPawn = AICon->GetPawn();
	if (!ControlledPawn)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(ControlledPawn->GetWorld());
	if (!NavSystem)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	FNavLocation RandomLocation;
	const bool bFound = NavSystem->GetRandomReachablePointInRadius(
		HomeLocation,
		AIData.PatrolRadius,
		RandomLocation
	);

	if (!bFound)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	FNavLocation ProjectedLocation;
	const bool bProjected = NavSystem->ProjectPointToNavigation(
		RandomLocation.Location,
		ProjectedLocation,
		FVector(100.f, 100.f, 300.f)
	);

	PatrolTargetLocation = bProjected ? ProjectedLocation.Location : RandomLocation.Location;
}

void AJunMonster::MoveToLocation(const FVector& Dest, float AcceptanceRadius)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return;
	}

	FNavLocation ProjectedLocation;
	const bool bProjected = NavigationSystem->ProjectPointToNavigation(
		Dest,
		ProjectedLocation,
		FVector(200.f, 200.f, 500.f)
	);

	if (!bProjected)
	{
		return;
	}

	const float ResolvedAcceptanceRadius =
		AcceptanceRadius >= 0.f ? AcceptanceRadius : AIData.PatrolAcceptRadius;

	AICon->MoveToLocation(ProjectedLocation.Location, ResolvedAcceptanceRadius);
}

bool AJunMonster::TryResolveReachableLocationToward(const FVector& DesiredLocation, FVector& OutReachableLocation) const
{
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return false;
	}

	const FVector StartLocation = GetActorLocation();
	const FVector QueryExtent(100.f, 100.f, 300.f);
	constexpr int32 NumSamples = 12;

	for (int32 SampleIndex = NumSamples; SampleIndex >= 0; --SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(NumSamples);
		const FVector SampleLocation = FMath::Lerp(StartLocation, DesiredLocation, Alpha);

		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(SampleLocation, ProjectedLocation, QueryExtent))
		{
			OutReachableLocation = ProjectedLocation.Location;
			return true;
		}
	}

	return false;
}

float AJunMonster::GetEffectiveReturnReachedDistance() const
{
	const float CapsuleRadius = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.f;

	return ReturnAcceptRadius + CapsuleRadius + ReturnReachedTolerance;
}

bool AJunMonster::HasReachedReturnTarget() const
{
	return FVector::Dist2D(GetActorLocation(), ReturnTargetLocation) <= GetEffectiveReturnReachedDistance();
}

void AJunMonster::ResetCombatTurnState()
{
	bCombatTurnInProgress = false;
	bHasPendingStateAfterCombatTurn = false;
	CurrentCombatTurnMontage = nullptr;
	PendingStateAfterCombatTurn = EMonsterState::Idle;
}

void AJunMonster::MoveToTarget(AActor* Target)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon || !Target)
	{
		return;
	}

	AICon->MoveToActor(Target, AIData.ChaseMoveAcceptanceRadius);
}

void AJunMonster::StartChase(AActor* NewTarget)
{
	if (!NewTarget)
	{
		return;
	}

	// 이미 같은 타겟을 상위 전투 상태에서 추적/전투 중이면 상태를 되감지 않는다.
	if (CurrentTarget == NewTarget &&
		(CurrentState == EMonsterState::Chase ||
		 CurrentState == EMonsterState::BattleStart ||
		 CurrentState == EMonsterState::Combat))
	{
		return;
	}

	// 전투 중에 새 타겟 갱신이 와도, 전투 유지 가능 범위라면 현재 상위 상태를 유지한다.
	if (CurrentState == EMonsterState::BattleStart || CurrentState == EMonsterState::Combat)
	{
		CurrentTarget = NewTarget;
		bIsHasTarget = true;
		return;
	}

	CurrentTarget = NewTarget;
	bIsHasTarget = false;
	SetMonsterState(EMonsterState::Chase);
}

void AJunMonster::StopAIMovement()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	AICon->StopMovement();
}

void AJunMonster::ClearCurrentTarget()
{
	CurrentTarget = nullptr;
	bIsHasTarget = false;
}

bool AJunMonster::TryHandleDeadCurrentTarget(EMonsterState NextStateIfDead)
{
	const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(CurrentTarget);
	if (!TargetCharacter || !TargetCharacter->Is_Dead())
	{
		return false;
	}

	ClearCurrentTarget();
	SetMonsterState(NextStateIfDead);
	return true;
}

void AJunMonster::StopAllAttackTraces()
{
	StopMonsterCodeMove(true);
	EndAttackTraceWindow();
	EndKickAttackTraceWindow();

	if (EquippedScabbard)
	{
		EquippedScabbard->StopAttackTrace();
	}

	if (AttachedArrow)
	{
		AttachedArrow->Destroy();
		AttachedArrow = nullptr;
	}
}

