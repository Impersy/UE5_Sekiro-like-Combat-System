

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
	Lighting_Shock,
	Airborne,
	Knockdown,
	Execution,
	Dead
};

UENUM(BlueprintType)
enum class EJunAirHitReactType : uint8
{
	None,
	Light,
	Heavy
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
enum class EJunFrontHitReactDirectionOverride : uint8
{
	Auto,
	Front_F,
	Front_FL,
	Front_FR
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

UENUM(BlueprintType)
enum class EJunDangerAttackType : uint8
{
	None,
	JumpCounter,
	MikiriCounter
};

USTRUCT(BlueprintType)
struct FJunAttackTraceOverrideData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace")
	bool bUseTraceOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackTrace", meta = (EditCondition = "bUseTraceOverride"))
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
struct FJunAttackDefenseRuleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	bool bCanBeParried = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	bool bCanBeGuarded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	bool bCanBeDodgedByInvincibility = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rebound")
	bool bCanReboundOnPerfectParry = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rebound", meta = (EditCondition = "bCanReboundOnPerfectParry", ClampMin = "0.0"))
	float ReboundBlendTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rebound", meta = (EditCondition = "bCanReboundOnPerfectParry", ClampMin = "0.0", ClampMax = "1.0"))
	float ReboundChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger")
	EJunDangerAttackType DangerAttackType = EJunDangerAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	EJunFrontHitReactDirectionOverride FrontHitReactDirectionOverride = EJunFrontHitReactDirectionOverride::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Hit")
	EJunAirHitReactType AirHitReactType = EJunAirHitReactType::Light;
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

USTRUCT(BlueprintType)
struct FJunPhysicalHitReactionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit")
	bool bEnable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit")
	FName RootBoneName = TEXT("spine_01");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlendWeight = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit", meta = (ClampMin = "0.0"))
	float Duration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit")
	float ImpulseStrength = 3500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit", meta = (ClampMin = "0.0"))
	float OrientationStrength = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit", meta = (ClampMin = "0.0"))
	float AngularVelocityStrength = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit", meta = (ClampMin = "0.0"))
	float MaxAngularForce = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit")
	float MinImpulseZ = -0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit")
	float MaxImpulseZ = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Additional Roots")
	bool bUseAdditionalRoots = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Additional Roots", meta = (EditCondition = "bUseAdditionalRoots"))
	TArray<FName> AdditionalRootBoneNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Additional Roots", meta = (EditCondition = "bUseAdditionalRoots", ClampMin = "0.0", ClampMax = "1.0"))
	float AdditionalRootBlendWeight = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Additional Roots", meta = (EditCondition = "bUseAdditionalRoots", ClampMin = "0.0"))
	float AdditionalRootImpulseScale = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Dampened Bones")
	bool bUseDampenedBones = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Dampened Bones", meta = (EditCondition = "bUseDampenedBones"))
	TArray<FName> DampenedBoneNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicalHit|Dampened Bones", meta = (EditCondition = "bUseDampenedBones", ClampMin = "0.0", ClampMax = "1.0"))
	float DampenedBoneBlendWeight = 0.06f;
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
		const FJunAttackDefenseRuleData& DefenseRuleData = FJunAttackDefenseRuleData(),
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
	virtual void BeginMikiriCounterCommandWindow();
	virtual void EndMikiriCounterCommandWindow();

	virtual FVector GetLockOnTargetPoint() const;
	virtual void ApplyPhysicalHitReaction(
		const FVector& WorldDirection,
		const FJunPhysicalHitReactionSettings& Settings,
		float StrengthScale = 1.f,
		FName BoneName = NAME_None);
	virtual void ApplyPhysicalHitReaction(const FVector& WorldDirection, float StrengthScale = 1.f, FName BoneName = NAME_None);

public:
	UFUNCTION(BlueprintCallable)
	bool HasGameplayTag(FGameplayTag Tag) const;

	virtual bool Is_Invincible() const;
	bool Is_Dead() const;

	FGameplayTag GetTeamTag() const;
	bool IsSameTeam(const AJunCharacter* Other) const;
	bool IsEnemyTo(const AJunCharacter* Other) const;
	virtual bool IsMikiriCounterThreatActive() const;
	virtual bool IsJumpCounterThreatActive() const;

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
	void RequestPlayerAttackHitStop(AJunCharacter* AttackerCharacter);
	void RequestDefenseHitStop(AJunCharacter* OtherCharacter);
	void PlayDefenseSound(EJunDefenseSoundType SoundType);
	void SetPendingDefenseSoundType(EJunDefenseSoundType NewSoundType, bool bPlayImmediately = true);
	void UpdatePhysicalHitReaction(float DeltaTime);
	void StopPhysicalHitReaction();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	TObjectPtr<class UPhysicalAnimationComponent> PhysicalAnimationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FGameplayTagContainer OwnedGameplayTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Danger")
	int32 ActiveMikiriCounterThreatCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Danger")
	int32 ActiveJumpCounterThreatCount = 0;

	TMap<EJunWeaponNiagaraComponent, EJunDangerAttackType> ActiveAttackTraceDangerTypes;

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PhysicalHit")
	FJunPhysicalHitReactionSettings LightPhysicalHitReactionSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PhysicalHit")
	FJunPhysicalHitReactionSettings SuperArmorPhysicalHitReactionSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PhysicalHit|Debug")
	bool bDebugPhysicalHitReaction = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	bool bPhysicalHitReactionActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	float PhysicalHitReactionRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	float PhysicalHitReactionDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	FName ActivePhysicalHitRootBoneName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	float ActivePhysicalHitBlendWeight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	TArray<FName> ActivePhysicalHitAdditionalRootBoneNames;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	float ActivePhysicalHitAdditionalRootBlendWeight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	TArray<FName> ActivePhysicalHitDampenedBoneNames;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	float ActivePhysicalHitDampenedBoneBlendWeight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	TEnumAsByte<ECollisionEnabled::Type> SavedPhysicalHitMeshCollisionEnabled = ECollisionEnabled::NoCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicalHit")
	bool bPhysicalHitMeshCollisionOverridden = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	TArray<TObjectPtr<class USoundBase>> PerfectParrySounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	TArray<TObjectPtr<class USoundBase>> NormalParrySounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	TArray<TObjectPtr<class USoundBase>> GuardHitSounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound|Defense")
	EJunDefenseSoundType PendingDefenseSoundType = EJunDefenseSoundType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Defense")
	bool bPlayDefenseSoundImmediately = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop")
	bool bEnablePlayerAttackHitStop = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop", meta = (ClampMin = "0.0"))
	float PlayerAttackHitStopDuration = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float PlayerAttackHitStopTimeScale = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop")
	int32 PlayerAttackHitStopPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|Defense")
	bool bEnableDefenseHitStop = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|Defense", meta = (ClampMin = "0.0"))
	float DefenseHitStopDuration = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|Defense", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float DefenseHitStopTimeScale = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitStop|Defense")
	int32 DefenseHitStopPriority = 1;

	void PlayRandomDefenseSound(const TArray<TObjectPtr<class USoundBase>>& Sounds) const;
	void PlayDefenseSoundByType(EJunDefenseSoundType SoundType) const;


};
