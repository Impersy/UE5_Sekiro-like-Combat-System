#include "Animation/AnimNotify_FireLightningProjectile.h"

#include "Character/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

FString UAnimNotify_FireLightningProjectile::GetNotifyName_Implementation() const
{
	return TEXT("FireLightningProjectile");
}

void UAnimNotify_FireLightningProjectile::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AJunCharacter* OwnerCharacter = MeshComp ? Cast<AJunCharacter>(MeshComp->GetOwner()) : nullptr;
	UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
	if (!OwnerCharacter || !World || !ProjectileClass)
	{
		return;
	}

	if (const AJunPlayer* OwnerPlayer = Cast<AJunPlayer>(OwnerCharacter))
	{
		if (bPlayerRequiresLastIncomingHitReactType &&
			OwnerPlayer->GetLastIncomingHitReactType() != RequiredPlayerLastIncomingHitReactType)
		{
			if (bDebugLog)
			{
				UE_LOG(LogTemp, Warning, TEXT("[FireLightningProjectile] Skipped. Player last incoming hit react=%d Required=%d"),
					static_cast<int32>(OwnerPlayer->GetLastIncomingHitReactType()),
					static_cast<int32>(RequiredPlayerLastIncomingHitReactType));
			}
			return;
		}
	}

	const FTransform SocketTransform = (!SpawnSocketName.IsNone() && MeshComp->DoesSocketExist(SpawnSocketName))
		? MeshComp->GetSocketTransform(SpawnSocketName)
		: OwnerCharacter->GetActorTransform();
	const FVector SpawnLocation = SocketTransform.TransformPosition(SpawnOffset);

	AJunCharacter* Target = FindBestTarget(OwnerCharacter);
	FVector FireDirection = Target
		? (Target->GetActorLocation() - SpawnLocation).GetSafeNormal()
		: OwnerCharacter->GetActorForwardVector();
	if (FireDirection.IsNearlyZero())
	{
		FireDirection = OwnerCharacter->GetActorForwardVector();
	}

	const FRotator SpawnRotation = FireDirection.Rotation();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	AArrowProjectile* Projectile = World->SpawnActor<AArrowProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeArrow(OwnerCharacter, HitReactType, DamageData, DefenseKnockbackData, DefenseRuleData);
	Projectile->Fire(
		FireDirection,
		Speed,
		LifeSeconds,
		bUseHoming ? Target : nullptr,
		bUseHoming ? HomingDuration : 0.f,
		bUseHoming ? HomingInterpSpeed : 0.f,
		HomingTargetHeightOffset);

	if (bDebugLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FireLightningProjectile] Owner=%s Target=%s Projectile=%s HitReact=%d"),
			*OwnerCharacter->GetName(),
			Target ? *Target->GetName() : TEXT("None"),
			*Projectile->GetName(),
			static_cast<int32>(HitReactType));
	}
}

AJunCharacter* UAnimNotify_FireLightningProjectile::FindBestTarget(AJunCharacter* OwnerCharacter) const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const float SearchRadiusSq = TargetSearchRadius > 0.f ? FMath::Square(TargetSearchRadius) : TNumericLimits<float>::Max();
	AJunCharacter* BestTarget = nullptr;
	float BestDistanceSq = SearchRadiusSq;
	for (TActorIterator<AJunCharacter> It(World); It; ++It)
	{
		AJunCharacter* Candidate = *It;
		if (!Candidate || Candidate == OwnerCharacter || !OwnerCharacter->IsEnemyTo(Candidate) || Candidate->Is_Dead())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(OwnerCharacter->GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}
