#pragma once

#include "CoreMinimal.h"
#include "Character/JunCharacter.h"
#include "Components/ActorComponent.h"
#include "Weapon/JunWeaponEffectTypes.h"
#include "JunPlayerEquipmentComponent.generated.h"

class AHatActor;
class AWeaponActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JUNDUCK_API UJunPlayerEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunPlayerEquipmentComponent();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	AWeaponActor* SpawnAndAttachWeapon(TSubclassOf<AWeaponActor> WeaponClass, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
	AWeaponActor* SpawnAndAttachScabbard(TSubclassOf<AWeaponActor> ScabbardClass, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Hat")
	AHatActor* SpawnAndAttachDefaultHat(TSubclassOf<AHatActor> HatClass, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Hat")
	AHatActor* EquipHat(TSubclassOf<AHatActor> NewHatClass, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Hat")
	void UnequipHat();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Visibility")
	void SetCameraProximityHidden(bool bHidden);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Visibility")
	void HideWeaponForDrinkPotion();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Visibility")
	void RestoreWeaponAfterDrinkPotion();

	UFUNCTION(BlueprintPure, Category = "Equipment|Visibility")
	bool IsWeaponHiddenForDrinkPotion() const { return bDrinkPotionWeaponHidden; }

	void BeginAttackTrace(
		EHitReactType HitReactType,
		const FJunAttackDamageData& DamageData,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		const FJunAttackDefenseRuleData& DefenseRuleData,
		EJunWeaponNiagaraComponent NiagaraComponent,
		const FJunAttackTraceOverrideData& TraceOverrideData);

	void EndAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent);
	void StopAttackTrace();
	void ActivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType);
	void DeactivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType);

	AWeaponActor* GetEquippedWeapon() const { return EquippedWeapon.Get(); }
	AWeaponActor* GetEquippedScabbard() const { return EquippedScabbard.Get(); }
	AHatActor* GetEquippedHat() const { return EquippedHat.Get(); }

protected:
	template<typename TActor>
	TActor* SpawnEquipmentActor(TSubclassOf<TActor> ActorClass) const;

	USkeletalMeshComponent* GetOwnerMesh() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon")
	TObjectPtr<AWeaponActor> EquippedWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon")
	TObjectPtr<AWeaponActor> EquippedScabbard = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Hat")
	TObjectPtr<AHatActor> EquippedHat = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Visibility")
	bool bCameraProximitySavedWeaponHidden = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Visibility")
	bool bCameraProximitySavedScabbardHidden = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Visibility")
	bool bCameraProximitySavedHatHidden = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Visibility")
	bool bDrinkPotionWeaponHidden = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Visibility")
	bool bDrinkPotionSavedWeaponHidden = false;
};
