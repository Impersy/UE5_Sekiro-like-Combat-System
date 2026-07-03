#include "Character/Player/JunPlayer.h"

#include "Camera/CameraComponent.h"
#include "Camera/JunSpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerPotionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#pragma region Movement / Camera Foundation
// ============================================================================
// Movement / Camera Foundation
// ============================================================================

bool AJunPlayer::Is_Invincible() const
{
	return bTestInvincible || Super::Is_Invincible();
}

void AJunPlayer::UpdateCameraAnchor(float DeltaTime)
{
	if (!CameraAnchor || !GetCapsuleComponent())
	{
		return;
	}

	const FVector CapsuleLocation = GetCapsuleComponent()->GetComponentLocation();
	FVector TargetAnchorLocation = CapsuleLocation;
	if (bProjectDodgeCameraAnchorToDodgeDirection &&
		bDodgeCameraAnchorProjectionActive &&
		!DodgeCameraAnchorDirection.IsNearlyZero())
	{
		FVector FromDodgeStart = CapsuleLocation - DodgeCameraAnchorStartLocation;
		FromDodgeStart.Z = 0.f;

		const float ProjectedDistance = FVector::DotProduct(FromDodgeStart, DodgeCameraAnchorDirection);
		DodgeCameraAnchorProjectedDistance = FMath::Max(
			DodgeCameraAnchorProjectedDistance,
			ProjectedDistance
		);

		TargetAnchorLocation = DodgeCameraAnchorStartLocation + DodgeCameraAnchorDirection * DodgeCameraAnchorProjectedDistance;
		TargetAnchorLocation.Z = CapsuleLocation.Z;

		if (bShowDodgeCameraDebug)
		{
			const FVector DebugCapsuleLocation = CapsuleLocation + FVector(0.f, 0.f, 40.f);
			const FVector DebugTargetLocation = TargetAnchorLocation + FVector(0.f, 0.f, 40.f);
			DrawDebugSphere(GetWorld(), DebugCapsuleLocation, 10.f, 12, FColor::Red, false, 0.f);
			DrawDebugSphere(GetWorld(), DebugTargetLocation, 10.f, 12, FColor::Green, false, 0.f);
			DrawDebugLine(GetWorld(), DebugCapsuleLocation, DebugTargetLocation, FColor::Yellow, false, 0.f, 0, 1.5f);
			DrawDebugLine(
				GetWorld(),
				DodgeCameraAnchorStartLocation + FVector(0.f, 0.f, 55.f),
				TargetAnchorLocation + FVector(0.f, 0.f, 55.f),
				FColor::Green,
				false,
				0.f,
				0,
				2.f
			);
		}
	}

	const float InterpSpeed = HasGameplayTag(JunGameplayTags::State_Action_Dodge)
		? CameraAnchorDodgeInterpSpeed
		: CameraAnchorInterpSpeed;

	const FVector NewAnchorLocation = FMath::VInterpTo(
		CameraAnchor->GetComponentLocation(),
		TargetAnchorLocation,
		DeltaTime,
		InterpSpeed
	);

	CameraAnchor->SetWorldLocation(NewAnchorLocation);
}

void AJunPlayer::UpdateMovementBraking(float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (KnockbackBrakingOverrideRemainTime > 0.f)
	{
		KnockbackBrakingOverrideRemainTime = FMath::Max(0.f, KnockbackBrakingOverrideRemainTime - DeltaTime);
		MovementComponent->BrakingDecelerationWalking = KnockbackBrakingDecelerationOverride;
		MovementComponent->GroundFriction = KnockbackGroundFrictionOverride;
		MovementComponent->BrakingFrictionFactor = KnockbackBrakingFrictionFactorOverride;
		return;
	}

	const bool bHasMoveInput =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);

	const bool bCanUseFreeRunSlide =
		!bLockOnActive &&
		GetDefenseState() == EJunDefenseState::None &&
		!GetPlayerIsFalling() &&
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		!HasGameplayTag(JunGameplayTags::State_Block_Move);

	if (!bCanUseFreeRunSlide || bHasMoveInput)
	{
		bFreeRunSlideActive = false;
		FreeRunSlideSpeed = 0.f;
		RestoreDefaultMovementBrakingSettings();
		return;
	}

	RestoreDefaultMovementBrakingSettings();

	if (!bFreeRunSlideActive || FreeRunSlideDirection.IsNearlyZero())
	{
		return;
	}

	const float CurrentVerticalSpeed = MovementComponent->Velocity.Z;
	FreeRunSlideSpeed = FMath::Max(0.f, FreeRunSlideSpeed - (FreeRunStopBrakingDeceleration * DeltaTime));

	if (FreeRunSlideSpeed <= 0.f)
	{
		bFreeRunSlideActive = false;
		return;
	}

	MovementComponent->Velocity = FVector(
		FreeRunSlideDirection.X * FreeRunSlideSpeed,
		FreeRunSlideDirection.Y * FreeRunSlideSpeed,
		CurrentVerticalSpeed
	);
}

void AJunPlayer::OnMoveInputReleased()
{
	const bool bCanStartFreeRunSlide =
		!bLockOnActive &&
		GetDefenseState() == EJunDefenseState::None &&
		!GetPlayerIsFalling() &&
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		!HasGameplayTag(JunGameplayTags::State_Block_Move);

	if (!bCanStartFreeRunSlide)
	{
		bFreeRunSlideActive = false;
		FreeRunSlideSpeed = 0.f;
		return;
	}

	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.f;

	const float HorizontalSpeed = HorizontalVelocity.Size();
	if (HorizontalSpeed < FreeRunSlideMinStartSpeed)
	{
		bFreeRunSlideActive = false;
		FreeRunSlideSpeed = 0.f;
		return;
	}

	bFreeRunSlideActive = true;
	FreeRunSlideDirection = HorizontalVelocity.GetSafeNormal();
	FreeRunSlideSpeed = HorizontalSpeed;
}

void AJunPlayer::CacheDefaultMovementBrakingSettings()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		DefaultWalkingBrakingDeceleration = MovementComponent->BrakingDecelerationWalking;
		DefaultGroundFriction = MovementComponent->GroundFriction;
		DefaultBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;
	}
}

void AJunPlayer::RestoreDefaultMovementBrakingSettings()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->BrakingDecelerationWalking = DefaultWalkingBrakingDeceleration;
		MovementComponent->GroundFriction = DefaultGroundFriction;
		MovementComponent->BrakingFrictionFactor = DefaultBrakingFrictionFactor;
	}
}

void AJunPlayer::TryInitializeCameraRotationFromController()
{
	if (bCameraRotationInitialized || !Controller)
	{
		return;
	}

	ResetCameraToSpawnView(GetActorRotation(), SpawnCameraPitch);
	StartCameraHardSnap();
}

void AJunPlayer::ResetCameraToSpawnView(const FRotator& SpawnRotation, const float CameraPitch)
{
	if (!Controller)
	{
		bCameraRotationInitialized = false;
		return;
	}

	TargetCameraYaw = SpawnRotation.Yaw;
	TargetCameraPitch = FMath::Clamp(CameraPitch, MinCameraPitch, MaxCameraPitch);
	Controller->SetControlRotation(FRotator(TargetCameraPitch, TargetCameraYaw, 0.f));
	bCameraRotationInitialized = true;
}

void AJunPlayer::SnapCameraToCurrentFreeView()
{
	if (CameraAnchor && GetCapsuleComponent())
	{
		CameraAnchor->SetWorldLocation(GetCapsuleComponent()->GetComponentLocation());
	}

	if (SpringArm)
	{
		SpringArm->SocketOffset = FreeCameraSocketOffset;
		SpringArm->TargetArmLength = GetPitchAdjustedSpringArmLength();
		if (UJunSpringArmComponent* JunSpringArm = Cast<UJunSpringArmComponent>(SpringArm))
		{
			JunSpringArm->ResetCameraSmoothing();
		}
	}

	if (Camera)
	{
		Camera->SetFieldOfView(FreeCameraFOV);
	}
}

void AJunPlayer::StartCameraHardSnap(int32 FrameCount)
{
	CameraHardSnapRemainFrames = FMath::Max(CameraHardSnapRemainFrames, FrameCount);
	if (SpringArm)
	{
		bSavedCameraLagEnabledForHardSnap = SpringArm->bEnableCameraLag;
		SpringArm->bEnableCameraLag = false;
	}
	SnapCameraToCurrentFreeView();
}

void AJunPlayer::UpdateCameraHardSnap()
{
	if (CameraHardSnapRemainFrames <= 0)
	{
		return;
	}

	SnapCameraToCurrentFreeView();
	--CameraHardSnapRemainFrames;

	if (CameraHardSnapRemainFrames <= 0 && SpringArm)
	{
		SpringArm->bEnableCameraLag = bSavedCameraLagEnabledForHardSnap;
	}
}

void AJunPlayer::UpdateMovementSpeed(float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	UpdateMovementBraking(DeltaTime);
	UpdateFreeRunRotationRate();

	TargetMaxWalkSpeed = GetDesiredMaxWalkSpeed();
	MovementComponent->MaxWalkSpeed = FMath::FInterpTo(
		MovementComponent->MaxWalkSpeed,
		TargetMaxWalkSpeed,
		DeltaTime,
		MoveSpeedInterpRate
	);
}

void AJunPlayer::UpdateFreeRunRotationRate()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->RotationRate = FRotator(0.f, EvaluateFreeRunRotationRate(), 0.f);
}

void AJunPlayer::ValidateLockOnTarget()
{
	if (!bLockOnActive || IsLockOnTargetValid())
	{
		return;
	}

	EndLockOn();
}

void AJunPlayer::UpdateCharacterRotationForCurrentCameraMode(float DeltaTime)
{
	if (bAttackFacingWindowActive)
	{
		UpdateAttackFacing(DeltaTime);
		return;
	}

	if (bLockOnActive)
	{
		if (bIsBasicAttacking || bIsHeavyAttacking)
		{
			return;
		}

		TryStartLockOnTurn();
		UpdateLockOnCharacterRotation(DeltaTime);
	}
}

void AJunPlayer::UpdateJumpStartAnimTrigger(float DeltaTime)
{
	if (JumpStartAnimTriggerRemainTime > 0.f)
	{
		JumpStartAnimTriggerRemainTime = FMath::Max(0.f, JumpStartAnimTriggerRemainTime - DeltaTime);
	}
}

void AJunPlayer::TryStartLockOnTurn()
{
	UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance();
	UAnimMontage* TurnMontage = EvaluateLockOnTurnMontage();
	if (!PlayerAnimInstance || !TurnMontage)
	{
		return;
	}

	PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnLockOnTurnMontageEnded);
	PlayerAnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnLockOnTurnMontageEnded);

	if (PlayerAnimInstance->Montage_Play(TurnMontage) <= 0.f)
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnLockOnTurnMontageEnded);
		return;
	}

	CurrentLockOnTurnMontage = TurnMontage;
	bLockOnTurnInProgress = true;
}

void AJunPlayer::OnLockOnTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentLockOnTurnMontage)
	{
		return;
	}

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnLockOnTurnMontageEnded);
	}

	bLockOnTurnInProgress = false;
	CurrentLockOnTurnMontage = nullptr;
}


void AJunPlayer::CancelLockOnTurn(float BlendOutTime)
{
	if (!IsLockOnTurnPlaying())
	{
		return;
	}

	const float ResolvedBlendOutTime =
		BlendOutTime >= 0.f ? BlendOutTime : LockOnTurnCancelBlendOutTime;
	UAnimMontage* PlayingTurnMontage = CurrentLockOnTurnMontage.Get();
	bLockOnTurnInProgress = false;
	CurrentLockOnTurnMontage = nullptr;

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnLockOnTurnMontageEnded);

		if (PlayingTurnMontage)
		{
			PlayerAnimInstance->Montage_Stop(ResolvedBlendOutTime, PlayingTurnMontage);
		}
	}
}

bool AJunPlayer::CanStartLockOnTurn() const
{
	if (!bLockOnActive || !LockOnTarget || bLockOnTurnInProgress)
	{
		return false;
	}

	if (GetDefenseState() != EJunDefenseState::None)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Attack) ||
		(GetCharacterMovement() && GetCharacterMovement()->IsFalling()))
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	return GetVelocity().Size2D() <= LockOnTurnMaxGroundSpeed;
}

bool AJunPlayer::IsHeavyAttacking() const
{
	return bIsHeavyAttacking;
}

bool AJunPlayer::IsJigenAttacking() const
{
	return bIsJigenAttacking;
}

bool AJunPlayer::IsJumpAttacking() const
{
	return bIsJumpAttacking;
}

bool AJunPlayer::IsDodgeAttacking() const
{
	return bIsDodgeAttacking;
}

bool AJunPlayer::IsDrinkingPotion() const
{
	return PotionComponent && PotionComponent->IsDrinking();
}

bool AJunPlayer::IsExecuting() const
{
	return ExecutionComponent && ExecutionComponent->IsExecuting();
}

bool AJunPlayer::IsInDeathSequence() const
{
	return DeathState != EJunPlayerDeathState::Alive;
}

bool AJunPlayer::IsWaitingForFakeDeathChoice() const
{
	return DeathState == EJunPlayerDeathState::FakeDeath && bDeathPresentationStarted;
}

bool AJunPlayer::IsLockOnTurnPlaying() const
{
	return bLockOnTurnInProgress && CurrentLockOnTurnMontage != nullptr;
}

float AJunPlayer::GetLockOnTargetYawDelta() const
{
	if (!LockOnTarget)
	{
		return 0.f;
	}

	FVector ToTarget = LockOnTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return 0.f;
	}

	const FRotator TargetRotation = ToTarget.Rotation();
	return FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetRotation.Yaw);
}

UAnimMontage* AJunPlayer::ChooseLockOnTurnMontage(float YawDelta) const
{
	const float AbsYawDelta = FMath::Abs(YawDelta);
	if (AbsYawDelta < LockOnTurnStartAngle)
	{
		return nullptr;
	}

	const bool bUse180 = AbsYawDelta >= LockOnTurn180Threshold;
	const bool bTurnRight = YawDelta > 0.f;

	if (bUse180)
	{
		return bTurnRight ? LockOnTurnRight180Montage.Get() : LockOnTurnLeft180Montage.Get();
	}

	return bTurnRight ? LockOnTurnRight90Montage.Get() : LockOnTurnLeft90Montage.Get();
}

float AJunPlayer::GetCurrentRunMoveSpeed() const
{
	return bLockOnActive ? LockOnRunMoveSpeed * GetLockOnMoveSpeedMultiplier() : FreeRunMoveSpeed;
}

float AJunPlayer::GetLockOnMoveSpeedMultiplier() const
{
	FVector2D MoveInput(DesiredMoveForward, DesiredMoveRight);
	if (MoveInput.IsNearlyZero())
	{
		MoveInput = FVector2D(PendingMoveForward, PendingMoveRight);
	}

	const float ForwardAmount = FMath::Abs(MoveInput.X);
	const float SideAmount = FMath::Abs(MoveInput.Y);
	const float TotalAmount = ForwardAmount + SideAmount;
	if (TotalAmount <= KINDA_SMALL_NUMBER)
	{
		return LockOnForwardMoveSpeedMultiplier;
	}

	const float ForwardMultiplier = MoveInput.X < 0.f
		? LockOnBackwardMoveSpeedMultiplier
		: LockOnForwardMoveSpeedMultiplier;

	return (ForwardAmount * ForwardMultiplier + SideAmount * LockOnSideMoveSpeedMultiplier) / TotalAmount;
}

FRotator AJunPlayer::GetJumpLaunchBasisRotation() const
{
	if (bLockOnActive)
	{
		return GetActorRotation();
	}

	if (Controller)
	{
		FRotator ControlRotation = Controller->GetControlRotation();
		ControlRotation.Pitch = 0.f;
		ControlRotation.Roll = 0.f;
		return ControlRotation;
	}

	return GetActorRotation();
}

FVector AJunPlayer::GetJumpLaunchDirection(const FVector2D& MoveInput) const
{
	if (MoveInput.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FRotator BasisRotation = GetJumpLaunchBasisRotation();
	const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(BasisRotation);
	const FVector RightDirection = UKismetMathLibrary::GetRightVector(BasisRotation);

	FVector LaunchDirection =
		(ForwardDirection * MoveInput.X) +
		(RightDirection * MoveInput.Y);

	LaunchDirection.Z = 0.f;
	return LaunchDirection.GetSafeNormal();
}

FVector AJunPlayer::BuildJumpLaunchVelocity(const FVector2D& MoveInput) const
{
	FVector LaunchVelocity(0.f, 0.f, GetCharacterMovement() ? GetCharacterMovement()->JumpZVelocity : 0.f);
	const FVector LaunchDirection = GetJumpLaunchDirection(MoveInput);

	if (LaunchDirection.IsNearlyZero())
	{
		return LaunchVelocity;
	}

	FVector CurrentHorizontalVelocity = GetVelocity();
	CurrentHorizontalVelocity.Z = 0.f;

	float CurrentDirectionalHorizontalSpeed =
		FMath::Max(0.f, FVector::DotProduct(CurrentHorizontalVelocity, LaunchDirection));

	if (bLockOnActive)
	{
		CurrentDirectionalHorizontalSpeed = FMath::Max(
			CurrentDirectionalHorizontalSpeed,
			LockOnJumpMinimumHorizontalSpeed
		);
	}

	LaunchVelocity.X = LaunchDirection.X * CurrentDirectionalHorizontalSpeed;
	LaunchVelocity.Y = LaunchDirection.Y * CurrentDirectionalHorizontalSpeed;
	return LaunchVelocity;
}

#pragma endregion
