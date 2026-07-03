


#include "AI/JunAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Character/Monster/JunMonster.h"

AJunAIController::AJunAIController(const FObjectInitializer& objectInitializer)
{
}

void AJunAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);



}

void AJunAIController::BeginPlay()
{
	Super::BeginPlay();

	
}

void AJunAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AJunMonster* Monster = Cast<AJunMonster>(GetPawn());
	if (!Monster)
	{
		return;
	}

	FVector2D DesiredLocalMove = FVector2D::ZeroVector;

	if (Monster->GetCurrentState() == EMonsterState::Combat)
	{
		DesiredLocalMove = Monster->GetCombatMoveInput();
	}
	else if (GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		FVector ToMoveDestination = GetImmediateMoveDestination() - Monster->GetActorLocation();
		ToMoveDestination.Z = 0.f;

		if (!ToMoveDestination.IsNearlyZero())
		{
			const FVector MoveDirection = ToMoveDestination.GetSafeNormal();
			DesiredLocalMove.X = FVector::DotProduct(Monster->GetActorForwardVector(), MoveDirection);
			DesiredLocalMove.Y = FVector::DotProduct(Monster->GetActorRightVector(), MoveDirection);
		}
	}

	Monster->SetDesiredMoveAxes(DesiredLocalMove.X, DesiredLocalMove.Y);

}

void AJunAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	AJunMonster* Monster = Cast<AJunMonster>(GetPawn());
	if (!Monster)
	{
		return;
	}

	// Patrol일 때만 다음 점으로
	if (Result.Code == EPathFollowingResult::Success ||
		Result.Code == EPathFollowingResult::Blocked ||
		Result.Code == EPathFollowingResult::Aborted)
	{
		Monster->HandlePatrolMoveCompleted(Result.Code == EPathFollowingResult::Success);
	}
}
