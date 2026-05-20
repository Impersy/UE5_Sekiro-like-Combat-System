

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/HighLightInterface.h"
#include "GameplayTagContainer.h"
#include "JunDefine.h"
#include "Weapon/JunWeaponEffectTypes.h"
#include "JunCharacter.generated.h"

UENUM(BlueprintType)
enum class ETeamType : uint8
{
	Player,
	Enemy,
	Neutral
};

UENUM(BlueprintType)
enum class EHitReactType : uint8
{
	None,
	LightHit,
	HeavyHit_A,
	HeavyHit_B,
	HeavyHit_C,
	LargeHit_Short,
	LargeHit_Long,
	Airborne,
	Knockdown,
	Execution,
	Dead
};

UENUM(BlueprintType)
enum class ECharacterHitReactDirection : uint8
{
	Back_B,
	Front_F,
	Front_FL,
	Front_FR,
	Left_L,
	Right_R
};

UENUM(BlueprintType)
enum class ECharacterKnockbackDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EJunDefenseSoundType : uint8
{
	None,
	PerfectParry,
	NormalParry,
	GuardHit
};

USTRUCT(BlueprintType)
struct FJunAttackTraceOverrideData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace")
	bool bUseTraceOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride", ClampMin = "0.0"))
	float TraceEndExtension = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride"))
	bool bOverrideTraceRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride && bOverrideTraceRadius", ClampMin = "0.0"))
	float TraceRadius = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride"))
	bool bOverrideTraceSampleCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride && bOverrideTraceSampleCount", ClampMin = "2"))
	int32 TraceSampleCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride"))
	bool bAutoAdjustSampleCountForExtension = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride && bAutoAdjustSampleCountForExtension", ClampMin = "1.0"))
	float TraceSampleSpacing = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride && bAutoAdjustSampleCountForExtension", ClampMin = "2"))
	int32 MaxAutoTraceSampleCount = 32;
};

USTRUCT(BlueprintType)
struct FJunDefenseKnockbackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	float Strength = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	float BrakingDeceleration = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	float GroundFriction = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	float BrakingFrictionFactor = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	float OverrideDuration = 0.15f;
};

USTRUCT(BlueprintType)
struct FJunAttackDefenseKnockbackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReactKnockback")
	FJunDefenseKnockbackData HitReact = { 180.f, 120.f, 0.45f, 0.2f, 0.18f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	FJunDefenseKnockbackData GuardBlock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DefenseKnockback")
	FJunDefenseKnockbackData ParrySuccess = { 500.f, 100.f, 0.35f, 0.12f, 0.12f };
};

USTRUCT(BlueprintType)
struct FJunAttackDamageData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float BaseDamage = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float PoiseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float GuardStaminaDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ParryStaminaDamage = 0.f;

	float GetFinalDamage() const
	{
		return FMath::Max(0.f, BaseDamage * DamageMultiplier);
	}
};

UCLASS()
class JUNDUCK_API AJunCharacter : public ACharacter, public IHighLightInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AJunCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	virtual void OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker);

	virtual void HighLight() override;
	virtual void UnHighLight() override;

	virtual void HandleGameplayEventNotify(FGameplayTag EventTag);

	void HandleDefenseSoundNotify();

public:
	virtual void BeginAttackTraceWindow(
		EHitReactType HitReactType = EHitReactType::LightHit,
		const FJunAttackDamageData& DamageData = FJunAttackDamageData(),
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData = FJunAttackDefenseKnockbackData(),
		EJunWeaponNiagaraComponent NiagaraComponent = EJunWeaponNiagaraComponent::Trail,
		const FJunAttackTraceOverrideData& TraceOverrideData = FJunAttackTraceOverrideData());

	virtual void EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent = EJunWeaponNiagaraComponent::Trail);

	virtual void BeginKickAttackTraceWindow(
		EHitReactType HitReactType = EHitReactType::LightHit,
		const FJunAttackDamageData& DamageData = FJunAttackDamageData(),
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData = FJunAttackDefenseKnockbackData());

	virtual void EndKickAttackTraceWindow();

	virtual void BeginWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType);
	virtual void EndWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType);

	virtual FVector GetLockOnTargetPoint() const;

public:
	UFUNCTION(BlueprintCallable)
	bool HasGameplayTag(FGameplayTag Tag) const;

	bool Is_Invincible() const;
	bool Is_Dead() const;

	FGameplayTag GetTeamTag() const;
	bool IsSameTeam(const AJunCharacter* Other) const;
	bool IsEnemyTo(const AJunCharacter* Other) const;

	UFUNCTION(BlueprintPure, Category = "Character|Health")
	int32 GetHp() const { return Hp; }

	UFUNCTION(BlueprintPure, Category = "Character|Health")
	int32 GetMaxHp() const { return MaxHp; }

protected:
	UFUNCTION(BlueprintCallable)
	void AddGameplayTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTag(FGameplayTag Tag);

	void SetTeamTag(FGameplayTag NewTeamTag);
	void SetPendingDefenseSoundType(EJunDefenseSoundType NewSoundType, bool bPlayImmediately = false);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FGameplayTagContainer OwnedGameplayTags;

	UPROPERTY(BlueprintReadOnly)
	bool bHighLighted = false;
 
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Hp = 200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxHp = 200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FinalDamage = 1;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	TArray<TObjectPtr<class USoundBase>> PerfectParrySounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	TArray<TObjectPtr<class USoundBase>> NormalParrySounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	TArray<TObjectPtr<class USoundBase>> GuardHitSounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound|Defense")
	EJunDefenseSoundType PendingDefenseSoundType = EJunDefenseSoundType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	bool bPlayDefenseSoundImmediately = false;

	void PlayRandomDefenseSound(const TArray<TObjectPtr<class USoundBase>>& Sounds) const;
	void PlayDefenseSoundByType(EJunDefenseSoundType SoundType) const;


};
