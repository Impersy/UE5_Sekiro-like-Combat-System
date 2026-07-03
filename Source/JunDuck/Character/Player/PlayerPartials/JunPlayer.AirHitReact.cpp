#include "Character/Player/JunPlayer.h"

#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"

#pragma region Air Hit React Runtime
// ============================================================================
// Air Hit React Runtime
// ============================================================================

void AJunPlayer::StartAirGuardBreakLandMontage()
{
	bAirGuardBreakLandMontagePending = false;
	RestoreAirHeavyHitFallTuning();

	CurrentHitReactMontage = AirGuardBreakMontage;
	const float LandDuration = AirGuardBreakMontage
		? AirGuardBreakMontage->GetPlayLength()
		: GuardBreakDuration;
	PlayerHitStateRemainTime = LandDuration;
	PlayerHitControlLockRemainTime = FMath::Max(PlayerHitControlLockRemainTime, LandDuration);
	GuardBreakVulnerableRemainTime = FMath::Max(GuardBreakVulnerableRemainTime, LandDuration);

	KnockbackBrakingDecelerationOverride = AirGuardBreakLandingBrakingDeceleration;
	KnockbackGroundFrictionOverride = AirGuardBreakLandingGroundFriction;
	KnockbackBrakingFrictionFactorOverride = AirGuardBreakLandingBrakingFrictionFactor;
	KnockbackBrakingOverrideRemainTime = AirGuardBreakLandingSlideDuration;

	if (AirGuardBreakMontage)
	{
		PlayHitReactMontageWithBlend(AirGuardBreakMontage, false);
	}
}

void AJunPlayer::ApplyAirGuardBreakTuning()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (!bAirHeavyHitFallTuningActive)
	{
		DefaultAirHeavyHitGravityScale = MovementComponent->GravityScale;
		DefaultAirHeavyHitFallingLateralFriction = MovementComponent->FallingLateralFriction;
		bAirHeavyHitFallTuningActive = true;
	}

	MovementComponent->GravityScale = AirGuardBreakGravityScale;
	MovementComponent->FallingLateralFriction = AirGuardBreakFallingLateralFriction;

	FVector KnockbackDirection = FVector::ZeroVector;
	if (HitReactFacingTarget)
	{
		KnockbackDirection = GetActorLocation() - HitReactFacingTarget->GetActorLocation();
	}

	KnockbackDirection.Z = 0.f;
	KnockbackDirection = KnockbackDirection.GetSafeNormal();
	if (KnockbackDirection.IsNearlyZero())
	{
		switch (LastIncomingKnockbackDirection)
		{
		case ECharacterKnockbackDirection::Forward:
			KnockbackDirection = GetActorForwardVector();
			break;
		case ECharacterKnockbackDirection::Left:
			KnockbackDirection = -GetActorRightVector();
			break;
		case ECharacterKnockbackDirection::Right:
			KnockbackDirection = GetActorRightVector();
			break;
		case ECharacterKnockbackDirection::Backward:
		default:
			KnockbackDirection = -GetActorForwardVector();
			break;
		}

		KnockbackDirection.Z = 0.f;
		KnockbackDirection = KnockbackDirection.GetSafeNormal();
	}

	if (!KnockbackDirection.IsNearlyZero() && AirGuardBreakKnockbackStrength > 0.f)
	{
		MovementComponent->AddImpulse(KnockbackDirection * AirGuardBreakKnockbackStrength, true);
	}

	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.Z = FMath::Min(NewVelocity.Z, AirGuardBreakDownwardVelocity);
	MovementComponent->Velocity = NewVelocity;
}


void AJunPlayer::UpdateAirHeavyHitReact(float DeltaTime)
{
	if (CurrentHitState != EJunPlayerHitState::HitReact ||
		AirHeavyHitStage == EJunAirHeavyHitStage::None ||
		AirHeavyHitStage == EJunAirHeavyHitStage::Land)
	{
		return;
	}

	if (AirHeavyHitStageRemainTime > 0.f)
	{
		AirHeavyHitStageRemainTime = FMath::Max(0.f, AirHeavyHitStageRemainTime - DeltaTime);
	}

	if (AirHeavyHitStage == EJunAirHeavyHitStage::Launch &&
		AirHeavyHitStageRemainTime <= 0.f)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
			MovementComponent && MovementComponent->IsFalling())
		{
			if (bAirDeathSequenceActive || AirHitFHeavyDownMontage)
			{
				StartAirHeavyHitDownStage();
			}
		}
		else
		{
			StartAirHeavyHitLandStage();
		}
	}
}

void AJunPlayer::StartAirHeavyHitDownStage()
{
	AirHeavyHitStage = EJunAirHeavyHitStage::Down;
	AirHeavyHitStageRemainTime = AirHitFHeavyDownMontage
		? AirHitFHeavyDownMontage->GetPlayLength()
		: 0.f;

	CurrentHitReactMontage = AirHitFHeavyDownMontage;
	if (AirHitFHeavyDownMontage)
	{
		PlayHitReactMontageWithBlend(AirHitFHeavyDownMontage, false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		FVector NewVelocity = MovementComponent->Velocity;
		NewVelocity.Z = FMath::Min(NewVelocity.Z, AirHeavyHitDownwardVelocity);
		MovementComponent->Velocity = NewVelocity;
	}
}

void AJunPlayer::StartAirHeavyHitLandStage()
{
	RestoreAirHeavyHitFallTuning();
	const bool bUseAirDeathLandMontage = bAirDeathSequenceActive;
	AirHeavyHitStage = EJunAirHeavyHitStage::Land;
	AirHeavyHitStageRemainTime = 0.f;

	UAnimMontage* LandMontage = bUseAirDeathLandMontage
		? GetAirDeathLandMontage()
		: AirHitFHeavyLandMontage.Get();
	bAirDeathSequenceActive = false;

	CurrentHitReactMontage = LandMontage;
	const float LandDuration = LandMontage
		? LandMontage->GetPlayLength()
		: AirHeavyHitReactDuration;
	PlayerHitStateRemainTime = LandDuration;
	PlayerHitControlLockRemainTime = FMath::Max(PlayerHitControlLockRemainTime, LandDuration);

	if (LandMontage)
	{
		PlayHitReactMontageWithBlend(LandMontage, false);
	}
	else if (bUseAirDeathLandMontage)
	{
		TriggerPendingDeathPresentation();
	}
}

void AJunPlayer::ResetAirHeavyHitReactState()
{
	RestoreAirHeavyHitFallTuning();
	AirHeavyHitStage = EJunAirHeavyHitStage::None;
	AirHeavyHitStageRemainTime = 0.f;
}

void AJunPlayer::ApplyAirLightHitFallTuning()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (!bAirHeavyHitFallTuningActive)
	{
		DefaultAirHeavyHitGravityScale = MovementComponent->GravityScale;
		DefaultAirHeavyHitFallingLateralFriction = MovementComponent->FallingLateralFriction;
		bAirHeavyHitFallTuningActive = true;
	}

	MovementComponent->GravityScale = AirLightHitGravityScale;
	MovementComponent->FallingLateralFriction = AirLightHitFallingLateralFriction;

	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.Z = FMath::Min(NewVelocity.Z, AirLightHitInitialDownwardVelocity);
	MovementComponent->Velocity = NewVelocity;
}

void AJunPlayer::ApplyAirHeavyHitFallTuning()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (!bAirHeavyHitFallTuningActive)
	{
		DefaultAirHeavyHitGravityScale = MovementComponent->GravityScale;
		DefaultAirHeavyHitFallingLateralFriction = MovementComponent->FallingLateralFriction;
		bAirHeavyHitFallTuningActive = true;
	}

	MovementComponent->GravityScale = AirHeavyHitGravityScale;
	MovementComponent->FallingLateralFriction = AirHeavyHitFallingLateralFriction;

	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.Z = FMath::Min(NewVelocity.Z, AirHeavyHitInitialDownwardVelocity);
	MovementComponent->Velocity = NewVelocity;
}

void AJunPlayer::RestoreAirHeavyHitFallTuning()
{
	if (!bAirHeavyHitFallTuningActive)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->GravityScale = DefaultAirHeavyHitGravityScale;
		MovementComponent->FallingLateralFriction = DefaultAirHeavyHitFallingLateralFriction;
	}

	bAirHeavyHitFallTuningActive = false;
}

void AJunPlayer::ApplyAirHitKnockback(EJunAirHitReactType AirHitReactType)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const float KnockbackStrength = AirHitReactType == EJunAirHitReactType::Heavy
		? AirHeavyHitKnockbackStrength
		: AirLightHitKnockbackStrength;
	if (!MovementComponent || KnockbackStrength <= 0.f)
	{
		return;
	}

	FVector KnockbackDirection = FVector::ZeroVector;
	if (HitReactFacingTarget)
	{
		KnockbackDirection = GetActorLocation() - HitReactFacingTarget->GetActorLocation();
	}

	KnockbackDirection.Z = 0.f;
	KnockbackDirection = KnockbackDirection.GetSafeNormal();
	if (KnockbackDirection.IsNearlyZero())
	{
		switch (LastIncomingKnockbackDirection)
		{
		case ECharacterKnockbackDirection::Forward:
			KnockbackDirection = GetActorForwardVector();
			break;
		case ECharacterKnockbackDirection::Left:
			KnockbackDirection = -GetActorRightVector();
			break;
		case ECharacterKnockbackDirection::Right:
			KnockbackDirection = GetActorRightVector();
			break;
		case ECharacterKnockbackDirection::Backward:
		default:
			KnockbackDirection = -GetActorForwardVector();
			break;
		}

		KnockbackDirection.Z = 0.f;
		KnockbackDirection = KnockbackDirection.GetSafeNormal();
	}

	if (!KnockbackDirection.IsNearlyZero())
	{
		MovementComponent->AddImpulse(KnockbackDirection * KnockbackStrength, true);
	}
}

#pragma endregion