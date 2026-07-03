#include "Character/Player/JunPlayer.h"

#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunAttackTargetInterface.h"
#include "Interface/JunLockOnTargetInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#pragma region Targeting / Facing
// ============================================================================
// Targeting / Facing
// ============================================================================

AJunCharacter* AJunPlayer::FindBestLockOnTarget()
{
	const FVector Start = GetActorLocation();
	const float Radius = FMath::Max(0.f, LockOnAcquireDistance);
	if (Radius <= 0.f)
	{
		return nullptr;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<AActor*> OutActors;

	const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Start,
		Radius,
		ObjectTypes,
		AJunCharacter::StaticClass(),
		ActorsToIgnore,
		OutActors
	);

	if (!bHit)
	{
		return nullptr;
	}

	AJunCharacter* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	const FVector MyLocation = GetActorLocation();
	const FVector CameraForward = GetCameraForwardOnPlane();

	for (AActor* HitActor : OutActors)
	{
		AJunCharacter* Candidate = Cast<AJunCharacter>(HitActor);
		if (!Candidate || Candidate == this)
		{
			continue;
		}

		if (IsSameTeam(Candidate))
		{
			continue;
		}

		const IJunLockOnTargetInterface* LockOnCandidate = Cast<IJunLockOnTargetInterface>(Candidate);
		if (!LockOnCandidate || !LockOnCandidate->CanBeLockedOnBy(this))
		{
			continue;
		}

		FVector ToTarget = Candidate->GetActorLocation() - MyLocation;
		ToTarget.Z = 0.f;

		const float Distance = ToTarget.Length();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		ToTarget.Normalize();

		const float Dot = FVector::DotProduct(CameraForward, ToTarget);
		if (Dot < 0.2f)
		{
			continue;
		}

		const float Score = Dot * 1000.f - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

AJunCharacter* AJunPlayer::FindBestAttackTarget()
{
	const FVector Start = GetActorLocation();
	const float Radius = FMath::Max(0.f, FreeAttackFacingAcquireDistance);
	if (Radius <= 0.f)
	{
		return nullptr;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<AActor*> OutActors;

	const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Start,
		Radius,
		ObjectTypes,
		AJunCharacter::StaticClass(),
		ActorsToIgnore,
		OutActors
	);

	if (!bHit)
	{
		return nullptr;
	}

	AJunCharacter* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	const FVector MyLocation = GetActorLocation();
	const FVector CameraForward = GetCameraForwardOnPlane();
	const float MinFacingDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(FreeAttackFacingAcquireAngle, 0.f, 180.f)));

	for (AActor* HitActor : OutActors)
	{
		AJunCharacter* Candidate = Cast<AJunCharacter>(HitActor);
		if (!Candidate || Candidate == this)
		{
			continue;
		}

		const IJunAttackTargetInterface* AttackTarget = Cast<IJunAttackTargetInterface>(Candidate);
		if (!AttackTarget || !AttackTarget->CanBeAttackTargetedBy(this))
		{
			continue;
		}

		FVector ToTarget = AttackTarget->GetAttackTargetPoint(this) - MyLocation;
		ToTarget.Z = 0.f;

		const float Distance = ToTarget.Length();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		ToTarget.Normalize();

		const float Dot = FVector::DotProduct(CameraForward, ToTarget);
		if (Dot < MinFacingDot)
		{
			continue;
		}

		const float Score = Dot * 1000.f - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void AJunPlayer::UpdateAttackFacing(float DeltaTime)
{
	if (!bAttackFacingWindowActive)
	{
		return;
	}

	FRotator TargetRotation;
	if (!TryResolveAttackFacingTargetRotation(TargetRotation))
	{
		return;
	}

	const float ResolvedFacingInterpSpeed = AttackFacingWindowInterpSpeed > 0.f ? AttackFacingWindowInterpSpeed : 30.f;
	ApplyPlayerFacingRotation(TargetRotation, DeltaTime, ResolvedFacingInterpSpeed);
}

void AJunPlayer::BeginAttackFacingWindow(float FacingInterpSpeed)
{
	AActor* ExecutionTarget = ExecutionComponent ? ExecutionComponent->GetCurrentExecutionTarget() : nullptr;
	AJunCharacter* ExecutionTargetCharacter = Cast<AJunCharacter>(ExecutionTarget);
	TargetActor = IsExecuting() && ExecutionTargetCharacter
		? ExecutionTargetCharacter
		: (LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget());

	if (!TargetActor)
	{
		bAttackFacingWindowActive = false;
		AttackFacingWindowInterpSpeed = 0.f;
		return;
	}

	bAttackFacingWindowActive = true;
	AttackFacingWindowInterpSpeed = FacingInterpSpeed;
}

void AJunPlayer::EndAttackFacingWindow()
{
	bAttackFacingWindowActive = false;
	AttackFacingWindowInterpSpeed = 0.f;
}

void AJunPlayer::BeginHitReactFacingWindow(float FacingInterpSpeed)
{
	if (!CanUseHitReactFacingWindow() || !HitReactFacingTarget)
	{
		bHitReactFacingWindowActive = false;
		HitReactFacingWindowInterpSpeed = 0.f;
		return;
	}

	bHitReactFacingWindowActive = true;
	HitReactFacingWindowInterpSpeed = FMath::Max(FacingInterpSpeed, 0.f);
}

void AJunPlayer::EndHitReactFacingWindow()
{
	bHitReactFacingWindowActive = false;
	HitReactFacingWindowInterpSpeed = 0.f;
}

void AJunPlayer::UpdateHitReactFacing(float DeltaTime)
{
	if (!bHitReactFacingWindowActive)
	{
		return;
	}

	FRotator TargetRotation;
	if (!TryResolveHitReactFacingTargetRotation(TargetRotation))
	{
		return;
	}

	const float ResolvedInterpSpeed = HitReactFacingWindowInterpSpeed > 0.f ? HitReactFacingWindowInterpSpeed : 30.f;
	ApplyPlayerFacingRotation(TargetRotation, DeltaTime, ResolvedInterpSpeed);
}

bool AJunPlayer::CanUseHitReactFacingWindow() const
{
	return CurrentHitState == EJunPlayerHitState::HitReact ||
		CurrentHitState == EJunPlayerHitState::GuardBreak;
}

#pragma endregion
