#include "Character/Monster/JunMonster.h"

#include "AI/JunAIController.h"
#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunBGMManager.h"
#include "System/JunTimeEffectSubsystem.h"
#include "UI/JunMonsterHUDWidget.h"
#include "Weapon/ArrowProjectile.h"
#include "Weapon/WeaponActor.h"

void AJunMonster::UpdateMonsterCodeMove(float DeltaTime)
{
	if (!bMonsterCodeMoveActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!CurrentTarget || !MovementComponent)
	{
		StopMonsterCodeMove(true);
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		StopMonsterCodeMove(true);
		return;
	}

	const float StopDistance = FMath::Max(ActiveMonsterCodeMoveData.MoveStandOffDistance, ActiveMonsterCodeMoveData.StopDistance);
	if (ToTarget.Size() <= StopDistance)
	{
		StopMonsterCodeMove(true);
		return;
	}

	if (ActiveMonsterCodeMoveData.MaxDistance > 0.f &&
		FVector::Dist2D(MonsterCodeMoveStartLocation, GetActorLocation()) >= ActiveMonsterCodeMoveData.MaxDistance)
	{
		StopMonsterCodeMove(true);
		return;
	}

	if (bMonsterCodeMoveGroundMotionOverrideActive && MonsterCodeMoveGroundMotionOverrideRemainTime > 0.f)
	{
		MonsterCodeMoveGroundMotionOverrideRemainTime = FMath::Max(0.f, MonsterCodeMoveGroundMotionOverrideRemainTime - DeltaTime);
		if (MonsterCodeMoveGroundMotionOverrideRemainTime <= 0.f)
		{
			RestoreMonsterCodeMoveGroundMotionOverride();
		}
	}

	FVector Velocity = MovementComponent->Velocity;
	FVector MoveDirection = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
	if (MoveDirection.IsNearlyZero() || ActiveMonsterCodeMoveData.bTrackTarget)
	{
		MoveDirection = ToTarget.GetSafeNormal();
	}

	if (ActiveMonsterCodeMoveData.bTrackTarget && ActiveMonsterCodeMoveData.TrackInterpSpeed > 0.f)
	{
		const FVector CurrentHorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
		const FVector TargetHorizontalVelocity = ToTarget.GetSafeNormal() * ActiveMonsterCodeMoveData.MoveSpeed;
		const FVector NewHorizontalVelocity = FMath::VInterpTo(
			CurrentHorizontalVelocity,
			TargetHorizontalVelocity,
			DeltaTime,
			ActiveMonsterCodeMoveData.TrackInterpSpeed);
		Velocity.X = NewHorizontalVelocity.X;
		Velocity.Y = NewHorizontalVelocity.Y;
	}
	else
	{
		Velocity.X = MoveDirection.X * ActiveMonsterCodeMoveData.MoveSpeed;
		Velocity.Y = MoveDirection.Y * ActiveMonsterCodeMoveData.MoveSpeed;
	}

	MovementComponent->Velocity = Velocity;

	if (ActiveMonsterCodeMoveData.bFaceTarget)
	{
		SetActorRotation(ToTarget.GetSafeNormal().Rotation());
	}
}

void AJunMonster::StopMonsterCodeMove(bool bClearVelocity)
{
	if (!bMonsterCodeMoveActive)
	{
		RestoreMonsterCodeMoveGroundMotionOverride();
		return;
	}

	bMonsterCodeMoveActive = false;
	MonsterCodeMoveStartLocation = FVector::ZeroVector;
	RestoreMonsterCodeMoveGroundMotionOverride();

	if (bClearVelocity)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			FVector Velocity = MovementComponent->Velocity;
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
			MovementComponent->Velocity = Velocity;
		}
	}
}

void AJunMonster::ApplyMonsterCodeMoveGroundMotionOverride(float Duration)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || bMonsterCodeMoveGroundMotionOverrideActive)
	{
		return;
	}

	MonsterCodeMoveDefaultGroundFriction = MovementComponent->GroundFriction;
	MonsterCodeMoveDefaultBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
	MonsterCodeMoveDefaultBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;

	MovementComponent->GroundFriction = 0.f;
	MovementComponent->BrakingDecelerationWalking = 0.f;
	MovementComponent->BrakingFrictionFactor = 0.f;
	bMonsterCodeMoveGroundMotionOverrideActive = true;
	MonsterCodeMoveGroundMotionOverrideRemainTime = Duration;
}

void AJunMonster::RestoreMonsterCodeMoveGroundMotionOverride()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !bMonsterCodeMoveGroundMotionOverrideActive)
	{
		return;
	}

	MovementComponent->GroundFriction = MonsterCodeMoveDefaultGroundFriction;
	MovementComponent->BrakingDecelerationWalking = MonsterCodeMoveDefaultBrakingDecelerationWalking;
	MovementComponent->BrakingFrictionFactor = MonsterCodeMoveDefaultBrakingFrictionFactor;
	bMonsterCodeMoveGroundMotionOverrideActive = false;
	MonsterCodeMoveGroundMotionOverrideRemainTime = 0.f;
}

void AJunMonster::ResetCurrentAttackRuntimeState()
{
	CurrentAttackMontage = nullptr;
	CurrentAttackSelection = FMonsterAttackSelection();
	bAttackFacingWindowActive = false;
	AttackFacingWindowInterpSpeed = 0.f;
}

