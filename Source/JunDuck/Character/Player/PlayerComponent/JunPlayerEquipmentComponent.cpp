#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"

#include "Equipment/HatActor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/WeaponActor.h"

UJunPlayerEquipmentComponent::UJunPlayerEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AWeaponActor* UJunPlayerEquipmentComponent::SpawnAndAttachWeapon(TSubclassOf<AWeaponActor> WeaponClass, FName SocketName)
{
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh || !WeaponClass || SocketName.IsNone())
	{
		return EquippedWeapon.Get();
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	EquippedWeapon = SpawnEquipmentActor<AWeaponActor>(WeaponClass);
	if (!EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn weapon."));
		return nullptr;
	}

	EquippedWeapon->AttachToComponent(
		OwnerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName
	);

	return EquippedWeapon.Get();
}

AWeaponActor* UJunPlayerEquipmentComponent::SpawnAndAttachScabbard(TSubclassOf<AWeaponActor> ScabbardClass, FName SocketName)
{
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh || !ScabbardClass || SocketName.IsNone())
	{
		return EquippedScabbard.Get();
	}

	if (EquippedScabbard)
	{
		EquippedScabbard->Destroy();
		EquippedScabbard = nullptr;
	}

	EquippedScabbard = SpawnEquipmentActor<AWeaponActor>(ScabbardClass);
	if (!EquippedScabbard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn scabbard."));
		return nullptr;
	}

	EquippedScabbard->StopAttackTrace();
	EquippedScabbard->SetActorTickEnabled(false);
	EquippedScabbard->AttachToComponent(
		OwnerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName
	);

	return EquippedScabbard.Get();
}

AHatActor* UJunPlayerEquipmentComponent::SpawnAndAttachDefaultHat(TSubclassOf<AHatActor> HatClass, FName SocketName)
{
	return HatClass ? EquipHat(HatClass, SocketName) : EquippedHat.Get();
}

AHatActor* UJunPlayerEquipmentComponent::EquipHat(TSubclassOf<AHatActor> NewHatClass, FName SocketName)
{
	UnequipHat();

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh || !NewHatClass || SocketName.IsNone())
	{
		return nullptr;
	}

	EquippedHat = SpawnEquipmentActor<AHatActor>(NewHatClass);
	if (!EquippedHat)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn hat."));
		return nullptr;
	}

	EquippedHat->AttachToComponent(
		OwnerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName
	);

	return EquippedHat.Get();
}

void UJunPlayerEquipmentComponent::UnequipHat()
{
	if (EquippedHat)
	{
		EquippedHat->Destroy();
		EquippedHat = nullptr;
	}
}

void UJunPlayerEquipmentComponent::SetCameraProximityHidden(bool bHidden)
{
	if (bHidden)
	{
		bCameraProximitySavedWeaponHidden = EquippedWeapon ? EquippedWeapon->IsHidden() : false;
		bCameraProximitySavedScabbardHidden = EquippedScabbard ? EquippedScabbard->IsHidden() : false;
		bCameraProximitySavedHatHidden = EquippedHat ? EquippedHat->IsHidden() : false;

		if (EquippedWeapon)
		{
			EquippedWeapon->SetActorHiddenInGame(true);
		}
		if (EquippedScabbard)
		{
			EquippedScabbard->SetActorHiddenInGame(true);
		}
		if (EquippedHat)
		{
			EquippedHat->SetActorHiddenInGame(true);
		}
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(bCameraProximitySavedWeaponHidden || bDrinkPotionWeaponHidden);
	}
	if (EquippedScabbard)
	{
		EquippedScabbard->SetActorHiddenInGame(bCameraProximitySavedScabbardHidden);
	}
	if (EquippedHat)
	{
		EquippedHat->SetActorHiddenInGame(bCameraProximitySavedHatHidden);
	}
}

void UJunPlayerEquipmentComponent::HideWeaponForDrinkPotion()
{
	bDrinkPotionWeaponHidden = false;
	bDrinkPotionSavedWeaponHidden = false;

	if (!EquippedWeapon)
	{
		return;
	}

	bDrinkPotionSavedWeaponHidden = EquippedWeapon->IsHidden();
	EquippedWeapon->SetActorHiddenInGame(true);
	EquippedWeapon->SetWeaponEffectsEnabled(false);
	bDrinkPotionWeaponHidden = true;
}

void UJunPlayerEquipmentComponent::RestoreWeaponAfterDrinkPotion()
{
	if (!bDrinkPotionWeaponHidden)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(bDrinkPotionSavedWeaponHidden);
		EquippedWeapon->SetWeaponEffectsEnabled(!bDrinkPotionSavedWeaponHidden);
	}

	bDrinkPotionWeaponHidden = false;
}

void UJunPlayerEquipmentComponent::BeginAttackTrace(
	EHitReactType HitReactType,
	const FJunAttackDamageData& DamageData,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	EJunWeaponNiagaraComponent NiagaraComponent,
	const FJunAttackTraceOverrideData& TraceOverrideData)
{
	if (!EquippedWeapon)
	{
		return;
	}

	AWeaponActor::FAttackTraceConfig TraceConfig;
	TraceConfig.HitReactType = HitReactType;
	TraceConfig.DamageData = DamageData;
	TraceConfig.DefenseKnockbackData = DefenseKnockbackData;
	TraceConfig.DefenseRuleData = DefenseRuleData;
	TraceConfig.OverrideData = TraceOverrideData;
	TraceConfig.NiagaraComponent = NiagaraComponent;
	EquippedWeapon->StartAttackTrace(TraceConfig);
}

void UJunPlayerEquipmentComponent::EndAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->EndAttackTrace(NiagaraComponent);
	}
}

void UJunPlayerEquipmentComponent::StopAttackTrace()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}
}

void UJunPlayerEquipmentComponent::ActivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->ActivateWeaponNiagara(ComponentType);
	}
}

void UJunPlayerEquipmentComponent::DeactivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->DeactivateWeaponNiagara(ComponentType);
	}
}

USkeletalMeshComponent* UJunPlayerEquipmentComponent::GetOwnerMesh() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	return OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
}

template<typename TActor>
TActor* UJunPlayerEquipmentComponent::SpawnEquipmentActor(TSubclassOf<TActor> ActorClass) const
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return World->SpawnActor<TActor>(
		ActorClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);
}
