

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/HighLightInterface.h"
#include "GameplayTagContainer.h"
#include "JunDefine.h"
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

public:
	virtual void BeginAttackTraceWindow(EHitReactType HitReactType = EHitReactType::LightHit);

	virtual void EndAttackTraceWindow();

	virtual void BeginKickAttackTraceWindow(EHitReactType HitReactType = EHitReactType::LightHit);

	virtual void EndKickAttackTraceWindow();

	virtual FVector GetLockOnTargetPoint() const;

public:
	UFUNCTION(BlueprintCallable)
	bool HasGameplayTag(FGameplayTag Tag) const;

	bool Is_Invincible() const;
	bool Is_Dead() const;

	FGameplayTag GetTeamTag() const;
	bool IsSameTeam(const AJunCharacter* Other) const;
	bool IsEnemyTo(const AJunCharacter* Other) const;

protected:
	UFUNCTION(BlueprintCallable)
	void AddGameplayTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTag(FGameplayTag Tag);

	void SetTeamTag(FGameplayTag NewTeamTag);

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



};
