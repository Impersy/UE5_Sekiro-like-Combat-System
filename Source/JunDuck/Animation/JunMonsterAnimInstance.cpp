


#include "Animation/JunMonsterAnimInstance.h"
#include "Character/Monster/JunMonster.h"
#include "Character/Monster/Boss/JunBoss.h"
#include "GameFramework/CharacterMovementComponent.h"

void UJunMonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AJunMonster* Monster = Cast<AJunMonster>(Character);
	if (!Monster)
	{
		bHasTarget = false;
		bIsGuard = false;
		bUseGuardBase = false;
		bIsInCombat = false;
		bIsAttacking = false;
		bIsRunning = false;
		bUseRunLocomotion = false;
		FootPlacementAlpha = 0.f;
		return;
	}

	bHasTarget = Monster->HasCombatTarget();
	bIsGuard = Monster->IsGuardOn();
	bUseGuardBase = bIsGuard;
	bIsInCombat = Monster->IsInCombat();
	bIsAttacking = Monster->IsAttacking();
	bIsRunning = Monster->IsRunning();
	bUseRunLocomotion = Monster->ShouldUseRunLocomotion();

	float TargetFootPlacementAlpha = 0.f;
	if (const UCharacterMovementComponent* MonsterMovementComponent = Monster->GetCharacterMovement())
	{
		if (!MonsterMovementComponent->IsFalling() && MonsterMovementComponent->CurrentFloor.IsWalkableFloor())
		{
			const FVector FloorNormal = MonsterMovementComponent->CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
			const float FloorNormalZ = FMath::Clamp(FloorNormal.Z, -1.f, 1.f);
			const float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FloorNormalZ));
			const float FullAngle = FMath::Max(FootPlacementSlopeStartAngle + KINDA_SMALL_NUMBER, FootPlacementSlopeFullAngle);
			TargetFootPlacementAlpha = FMath::GetMappedRangeValueClamped(
				FVector2D(FootPlacementSlopeStartAngle, FullAngle),
				FVector2D(0.f, 1.f),
				SlopeAngle);
		}
	}

	if (const AJunBoss* Boss = Cast<AJunBoss>(Monster))
	{
		if (Boss->ShouldReduceFootPlacementAlpha())
		{
			TargetFootPlacementAlpha = FMath::Min(TargetFootPlacementAlpha, BossEvadeFootPlacementMaxAlpha);
		}
	}

	FootPlacementAlpha = FootPlacementAlphaInterpSpeed > 0.f
		? FMath::FInterpTo(FootPlacementAlpha, TargetFootPlacementAlpha, DeltaSeconds, FootPlacementAlphaInterpSpeed)
		: TargetFootPlacementAlpha;
	if (FMath::IsNearlyEqual(FootPlacementAlpha, TargetFootPlacementAlpha, 0.001f))
	{
		FootPlacementAlpha = TargetFootPlacementAlpha;
	}
}
