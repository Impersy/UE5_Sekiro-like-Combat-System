#include "Weapon/ArrowProjectile.h"

#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/JunCombatVFXSubsystem.h"

AArrowProjectile::AArrowProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	ArrowMesh->SetupAttachment(CollisionSphere);
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowMesh->SetGenerateOverlapEvents(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bAutoActivate = false;
}

void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(CollisionRadius);
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AArrowProjectile::OnArrowOverlap);
	}
}

void AArrowProjectile::InitializeArrow(
	AJunCharacter* InOwnerCharacter,
	EHitReactType InHitReactType,
	const FJunAttackDamageData& InDamageData,
	const FJunAttackDefenseKnockbackData& InDefenseKnockbackData,
	const FJunAttackDefenseRuleData& InDefenseRuleData)
{
	OwnerCharacter = InOwnerCharacter;
	HitReactType = InHitReactType;
	DamageData = InDamageData;
	DefenseKnockbackData = InDefenseKnockbackData;
	DefenseRuleData = InDefenseRuleData;
	bHasHit = false;
	CurrentHomingTarget = nullptr;
	HomingRemainTime = 0.f;
}

void AArrowProjectile::Fire(
	const FVector& Direction,
	float Speed,
	float LifeSeconds,
	AActor* HomingTarget,
	float HomingDuration,
	float HomingInterpSpeed,
	float HomingTargetHeightOffset)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	const FVector FireDirection = Direction.IsNearlyZero()
		? GetActorForwardVector()
		: Direction.GetSafeNormal();

	if (ProjectileMovement)
	{
		CurrentSpeed = FMath::Max(0.f, Speed);
		ProjectileMovement->Velocity = FireDirection * CurrentSpeed;
		ProjectileMovement->Activate(true);
	}

	CurrentHomingTarget = HomingTarget;
	HomingRemainTime = FMath::Max(0.f, HomingDuration);
	HomingTurnInterpSpeed = FMath::Max(0.f, HomingInterpSpeed);
	HomingAimHeightOffset = HomingTargetHeightOffset;
	SetActorTickEnabled(HomingRemainTime > 0.f && CurrentHomingTarget.IsValid() && HomingTurnInterpSpeed > 0.f);
	SetLifeSpan(FMath::Max(0.1f, LifeSeconds));
}

void AArrowProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ProjectileMovement || !CurrentHomingTarget.IsValid() || HomingRemainTime <= 0.f || HomingTurnInterpSpeed <= 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}

	HomingRemainTime = FMath::Max(0.f, HomingRemainTime - DeltaTime);

	FVector TargetPoint = CurrentHomingTarget->GetActorLocation();
	TargetPoint.Z += HomingAimHeightOffset;

	const FVector DesiredDirection = (TargetPoint - GetActorLocation()).GetSafeNormal();
	if (!DesiredDirection.IsNearlyZero())
	{
		const FVector CurrentDirection = ProjectileMovement->Velocity.GetSafeNormal();
		const FVector NewDirection = FMath::VInterpNormalRotationTo(
			CurrentDirection.IsNearlyZero() ? GetActorForwardVector() : CurrentDirection,
			DesiredDirection,
			DeltaTime,
			HomingTurnInterpSpeed);
		ProjectileMovement->Velocity = NewDirection * CurrentSpeed;
	}

	if (HomingRemainTime <= 0.f)
	{
		SetActorTickEnabled(false);
	}
}

void AArrowProjectile::OnArrowOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bHasHit || !OtherActor || OtherActor == OwnerCharacter)
	{
		return;
	}

	AJunCharacter* HitCharacter = Cast<AJunCharacter>(OtherActor);
	if (!HitCharacter || !OwnerCharacter || !OwnerCharacter->IsEnemyTo(HitCharacter))
	{
		return;
	}

	bHasHit = true;
	const int32 HpBeforeHit = HitCharacter->GetHp();
	AJunMonster* HitMonster = Cast<AJunMonster>(HitCharacter);
	const bool bHitExecutionReadyMonster = HitMonster && HitMonster->IsExecutionReady();
	const FVector SwingDirection = GetVelocity().GetSafeNormal();
	FJunAttackDefenseRuleData ResolvedDefenseRuleData = DefenseRuleData;
	ResolvedDefenseRuleData.bCanBeDodgedByInvincibility = false;
	if (AJunPlayer* HitPlayer = Cast<AJunPlayer>(HitCharacter))
	{
		HitPlayer->ReceiveHit(HitReactType, DamageData.GetFinalDamage(), OwnerCharacter.Get(), SwingDirection, DefenseKnockbackData, ResolvedDefenseRuleData, false);
	}
	else if (HitMonster)
	{
		HitMonster->ReceiveHit(
			HitReactType,
			DamageData.GetFinalDamage(),
			OwnerCharacter.Get(),
			SwingDirection,
			DefenseKnockbackData,
			ResolvedDefenseRuleData,
			DamageData.PoiseDamage);
	}

	if (HitCharacter->GetHp() < HpBeforeHit || bHitExecutionReadyMonster)
	{
		if (UJunCombatVFXSubsystem* VFXSubsystem = GetWorld()->GetSubsystem<UJunCombatVFXSubsystem>())
		{
			VFXSubsystem->SpawnBloodImpact(
				HitCharacter->GetMesh(),
				SweepResult,
				SwingDirection,
				OwnerCharacter->GetActorLocation(),
				HitMonster && HitMonster->WasLastHitPhysicalReactionOnly());
		}
	}
	Destroy();
}
