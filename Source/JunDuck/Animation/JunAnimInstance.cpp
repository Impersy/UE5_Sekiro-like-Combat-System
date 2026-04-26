


#include "Animation/JunAnimInstance.h"
#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
#include "JunGameplayTags.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

UJunAnimInstance::UJunAnimInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void UJunAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (Character = Cast<AJunCharacter>(TryGetPawnOwner()))
	{
		MovementComponent = Character->GetCharacterMovement();
	}

}

void UJunAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character == nullptr)
		return;
	if (MovementComponent == nullptr)
		return;

	UpdateMovementData(DeltaSeconds);
	UpdateMovementDirectionData(DeltaSeconds);
	UpdateDerivedData(DeltaSeconds);

}

void UJunAnimInstance::UpdateMovementData(float DeltaSeconds)
{
	Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();

	if (Cast<AJunMonster>(Character))
	{
		bShouldMove = (GroundSpeed > 3.f);
	}
	else
	{
		bShouldMove = (GroundSpeed > 3.f &&
			MovementComponent->GetCurrentAcceleration() != FVector::ZeroVector);
	}

	bIsFalling = MovementComponent->IsFalling();
}

void UJunAnimInstance::UpdateMovementDirectionData(float DeltaSeconds)
{
	float TargetLocalMoveForward = 0.f;
	float TargetLocalMoveRight = 0.f;

	if (const AJunPlayer* Player = Cast<AJunPlayer>(Character))
	{
		if (Player->HasGameplayTag(JunGameplayTags::State_Action_Dodge) || Player->IsDodgeAttacking())
		{
			// Do not drive lock-on locomotion direction from dodge velocity.
			// Otherwise the locomotion BS is already near full input before dodge ends,
			// which makes the return-to-move snap in too hard.
			TargetLocalMoveForward = 0.f;
			TargetLocalMoveRight = 0.f;
		}
		else
		{
		const bool bCanUseDesiredMoveInput =
			!Player->ShouldDeferGuardMoveInput() &&
			!Player->HasGameplayTag(JunGameplayTags::State_Block_Move);

		if (bCanUseDesiredMoveInput)
		{
			TargetLocalMoveForward = Player->GetDesiredMoveForward();
			TargetLocalMoveRight = Player->GetDesiredMoveRight();
		}
		else
		{
			FVector HorizontalVelocity = Velocity;
			HorizontalVelocity.Z = 0.f;

			if (!HorizontalVelocity.IsNearlyZero())
			{
				const FVector MoveDir = HorizontalVelocity.GetSafeNormal();

				TargetLocalMoveForward = FVector::DotProduct(Character->GetActorForwardVector(), MoveDir);
				TargetLocalMoveRight = FVector::DotProduct(Character->GetActorRightVector(), MoveDir);
			}
		}
		}
	}
	else if (const AJunMonster* Monster = Cast<AJunMonster>(Character))
	{
		TargetLocalMoveForward = Monster->GetDesiredMoveForward();
		TargetLocalMoveRight = Monster->GetDesiredMoveRight();

		if (FMath::IsNearlyZero(TargetLocalMoveForward) && FMath::IsNearlyZero(TargetLocalMoveRight))
		{
			FVector HorizontalVelocity = Velocity;
			HorizontalVelocity.Z = 0.f;

			if (!HorizontalVelocity.IsNearlyZero())
			{
				const FVector MoveDir = HorizontalVelocity.GetSafeNormal();

				TargetLocalMoveForward = FVector::DotProduct(Character->GetActorForwardVector(), MoveDir);
				TargetLocalMoveRight = FVector::DotProduct(Character->GetActorRightVector(), MoveDir);

				// Combat strafe often comes from AI/path velocity instead of explicit desired input.
				// Clamp lateral fallback locomotion input so the BS does not jump to full side input.
				TargetLocalMoveRight = FMath::Clamp(TargetLocalMoveRight, -0.4f, 0.4f);
			}
		}

	}
	else
	{
		FVector HorizontalVelocity = Velocity;
		HorizontalVelocity.Z = 0.f;

		if (!HorizontalVelocity.IsNearlyZero())
		{
			const FVector MoveDir = HorizontalVelocity.GetSafeNormal();

			TargetLocalMoveForward = FVector::DotProduct(Character->GetActorForwardVector(), MoveDir);
			TargetLocalMoveRight = FVector::DotProduct(Character->GetActorRightVector(), MoveDir);
		}
	}

	LocalMoveForward = FMath::FInterpTo(
		LocalMoveForward,
		TargetLocalMoveForward,
		DeltaSeconds,
		MovementDirectionInterpSpeed
	);

	LocalMoveRight = FMath::FInterpTo(
		LocalMoveRight,
		TargetLocalMoveRight,
		DeltaSeconds,
		MovementDirectionInterpSpeed
	);
}

void UJunAnimInstance::UpdateDerivedData(float DeltaSeconds)
{
}
