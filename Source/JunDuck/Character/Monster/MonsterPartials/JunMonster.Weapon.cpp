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

void AJunMonster::SpawnAndAttachWeapon()
{
	// 몬스터 무기는 BeginPlay에서 한 번 스폰해서 소켓에 붙이고,
	// 공격 trace window 때만 활성화한다.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (DefaultWeaponClass)
	{
		EquippedWeapon = World->SpawnActor<AWeaponActor>(
			DefaultWeaponClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster weapon."));
		}
		else
		{
			const FName InitialWeaponSocketName = bStartWithCutsceneWait ? SheathedWeaponSocketName : WeaponSocketName;
			AttachWeaponToSocket(InitialWeaponSocketName);
		}
	}

	if (DefaultScabbardClass)
	{
		EquippedScabbard = World->SpawnActor<AWeaponActor>(
			DefaultScabbardClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedScabbard)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster scabbard."));
		}
		else
		{
			EquippedScabbard->StopAttackTrace();
			EquippedScabbard->SetActorTickEnabled(false);
			EquippedScabbard->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				ScabbardSocketName
			);
		}
	}

	if (DefaultBowClass)
	{
		EquippedBow = World->SpawnActor<AWeaponActor>(
			DefaultBowClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedBow)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster bow."));
		}
		else
		{
			EquippedBow->StopAttackTrace();
			EquippedBow->SetActorTickEnabled(false);
			EquippedBow->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				BowSocketName
			);
			EquippedBow->SetActorHiddenInGame(true);
		}
	}

	if (DefaultKickWeaponClass)
	{
		EquippedKickWeapon = World->SpawnActor<AWeaponActor>(
			DefaultKickWeaponClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedKickWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster kick weapon."));
		}
		else
		{
			EquippedKickWeapon->StopAttackTrace();
			EquippedKickWeapon->SetTraceSampleCount(KickWeaponTraceSampleCount);
			EquippedKickWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				KickWeaponSocketName
			);
		}
	}

	if (DefaultKickWeaponRightClass)
	{
		EquippedKickWeaponRight = World->SpawnActor<AWeaponActor>(
			DefaultKickWeaponRightClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedKickWeaponRight)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster right kick weapon."));
		}
		else
		{
			EquippedKickWeaponRight->StopAttackTrace();
			EquippedKickWeaponRight->SetTraceSampleCount(KickWeaponTraceSampleCount);
			EquippedKickWeaponRight->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				KickWeaponRightSocketName
			);
		}
	}
}

void AJunMonster::AttachWeaponToSocket(FName SocketName)
{
	if (!EquippedWeapon || !GetMesh() || SocketName.IsNone())
	{
		return;
	}

	EquippedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName
	);

	const bool bAttachedToHand = SocketName == WeaponSocketName;
	bWeaponAttachedToHand = bAttachedToHand;
	EquippedWeapon->SetWeaponEffectsEnabled(bWeaponAttachedToHand && !EquippedWeapon->IsHidden());
}

void AJunMonster::AttachWeaponToHandSocket()
{
	AttachWeaponToSocket(WeaponSocketName);
}

void AJunMonster::AttachWeaponToSheathedSocket()
{
	AttachWeaponToSocket(SheathedWeaponSocketName);
}

void AJunMonster::SetWeaponVisible(bool bVisible)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(!bVisible);
		EquippedWeapon->SetWeaponEffectsEnabled(bVisible && bWeaponAttachedToHand);
	}
}

void AJunMonster::SetBowVisible(bool bVisible)
{
	if (EquippedBow)
	{
		EquippedBow->SetActorHiddenInGame(!bVisible);
	}
}

void AJunMonster::SpawnAttachedArrow()
{
	if (!DefaultArrowProjectileClass || !GetWorld() || !GetMesh())
	{
		return;
	}

	if (AttachedArrow)
	{
		AttachedArrow->Destroy();
		AttachedArrow = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AttachedArrow = GetWorld()->SpawnActor<AArrowProjectile>(
		DefaultArrowProjectileClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!AttachedArrow)
	{
		return;
	}

	AttachedArrow->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ArrowSocketName
	);
}

void AJunMonster::FireAttachedArrow(
	EHitReactType HitReactType,
	const FJunAttackDamageData& DamageData,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	float Speed,
	float LifeSeconds,
	float HomingDuration,
	float HomingInterpSpeed,
	float HomingTargetHeightOffset)
{
	if (!AttachedArrow)
	{
		SpawnAttachedArrow();
	}

	if (!AttachedArrow)
	{
		return;
	}

	FVector AimPoint = CurrentTarget ? CurrentTarget->GetActorLocation() : (GetActorLocation() + GetActorForwardVector() * 1000.f);
	AimPoint.Z += 50.f;
	const FVector FireDirection = (AimPoint - AttachedArrow->GetActorLocation()).GetSafeNormal();

	AttachedArrow->InitializeArrow(this, HitReactType, DamageData, DefenseKnockbackData, DefenseRuleData);
	AttachedArrow->Fire(FireDirection, Speed, LifeSeconds, CurrentTarget.Get(), HomingDuration, HomingInterpSpeed, HomingTargetHeightOffset);
	AttachedArrow = nullptr;
}

// Hit react

