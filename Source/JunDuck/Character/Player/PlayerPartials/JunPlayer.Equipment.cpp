#include "Character/Player/JunPlayer.h"

#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Equipment/HatActor.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunLockOnTargetInterface.h"
#include "Weapon/WeaponActor.h"
#pragma region Equipment / Public State Queries
// ============================================================================
// Equipment / Public State Queries
// ============================================================================

void AJunPlayer::SpawnAndAttachWeapon()
{
	if (!EquipmentComponent)
	{
		return;
	}

	EquipmentComponent->SpawnAndAttachWeapon(DefaultWeaponClass, WeaponSocketName);
}

void AJunPlayer::SpawnAndAttachScabbard()
{
	if (!EquipmentComponent)
	{
		return;
	}

	EquipmentComponent->SpawnAndAttachScabbard(DefaultScabbardClass, ScabbardSocketName);
}

void AJunPlayer::SpawnAndAttachDefaultHat()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->SpawnAndAttachDefaultHat(DefaultHatClass, HatSocketName);
	}
}

void AJunPlayer::EquipHat(TSubclassOf<AHatActor> NewHatClass)
{
	if (EquipmentComponent)
	{
		EquipmentComponent->EquipHat(NewHatClass, HatSocketName);
	}
}

void AJunPlayer::UnequipHat()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->UnequipHat();
	}
}

bool AJunPlayer::GetPlayerIsFalling()
{
	return GetMovementComponent()->IsFalling();
}

bool AJunPlayer::IsLockOn()
{
	return bLockOnActive;
}

FVector AJunPlayer::GetLockOnMarkerWorldPoint()
{
	return GetRawLockOnBonePoint();
}

bool AJunPlayer::IsGuardOn()
{
	return GetDefenseState() == EJunDefenseState::Guarding;
}

bool AJunPlayer::IsGuardPoseActive()
{
	const bool bAirParryActive = DefenseComponent
		? DefenseComponent->IsAirParrySequenceActive()
		: false;
	return GetDefenseState() != EJunDefenseState::None && !bAirParryActive;
}

bool AJunPlayer::IsBasicAttacking() const
{
	return bIsBasicAttacking;
}

bool AJunPlayer::IsWalking() const
{
	return GetMoveState() != EJunMoveState::Run;
}

bool AJunPlayer::IsSprinting() const
{
	return false;
}

bool AJunPlayer::IsInParrySuccess() const
{
	return CurrentHitState == EJunPlayerHitState::ParrySuccess;
}

bool AJunPlayer::IsGuardBlockReacting() const
{
	return CurrentHitState == EJunPlayerHitState::GuardBlock;
}

bool AJunPlayer::IsJumpStartAnimTriggered() const
{
	return JumpStartAnimTriggerRemainTime > 0.f;
}

EJunMoveState AJunPlayer::GetMoveState() const
{
	const bool bAirParryActive = DefenseComponent
		? DefenseComponent->IsAirParrySequenceActive()
		: false;
	const EJunDefenseState CurrentDefenseState = GetDefenseState();
	if (!bAirParryActive &&
		(CurrentDefenseState == EJunDefenseState::Starting ||
		 CurrentDefenseState == EJunDefenseState::Guarding))
	{
		return EJunMoveState::Guard;
	}

	const bool bHasMoveInput =
		!FMath::IsNearlyZero(DesiredMoveForward) ||
		!FMath::IsNearlyZero(DesiredMoveRight) ||
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);

	if (bWalkRequested && bHasMoveInput)
	{
		return EJunMoveState::Walk;
	}

	return EJunMoveState::Run;
}

#pragma endregion
