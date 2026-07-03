#include "Character/Player/JunPlayer.h"

#include "Camera/CameraComponent.h"
#include "Camera/JunSpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunAttackTargetInterface.h"
#include "Interface/JunExecutionTargetInterface.h"
#include "Interface/JunJumpCounterTargetInterface.h"
#include "Interface/JunLockOnTargetInterface.h"
#include "JunGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#pragma region Camera Modes
// ============================================================================
// Camera Modes
// ============================================================================

void AJunPlayer::UpdateJunCamera(float DeltaTime)
{
	switch (CameraMode)
	{
	case EJunCameraMode::Execution:
		UpdateExecutionCamera(DeltaTime);
		break;
	case EJunCameraMode::Death:
		UpdateDeathCamera(DeltaTime);
		break;
	case EJunCameraMode::LockOn:
		UpdateLockOnCamera(DeltaTime);
		break;
	case EJunCameraMode::Free:
	default:
		UpdateFreeCamera(DeltaTime);
		break;
	}

	FVector TargetSocketOffset = FreeCameraSocketOffset;
	if (CameraMode == EJunCameraMode::Execution)
	{
		TargetSocketOffset = GetCurrentExecutionCameraSocketOffset();
	}
	else if (CameraMode == EJunCameraMode::Death)
	{
		TargetSocketOffset = SpringArm ? SpringArm->SocketOffset : FreeCameraSocketOffset;
	}
	else if (bLockOnActive)
	{
		TargetSocketOffset = LockOnCameraSocketOffset;
	}

	if (!bLockOnActive && HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		TargetSocketOffset += DodgeCameraSocketOffset;
	}

	const FVector NewSocketOffset = FMath::VInterpTo(
		SpringArm->SocketOffset,
		TargetSocketOffset,
		DeltaTime,
		CameraSocketOffsetInterpSpeed
	);

	SpringArm->SocketOffset = NewSocketOffset;

	if (CameraMode != EJunCameraMode::Execution)
	{
		const IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(LockOnTarget.Get());
		const bool bTargetExecutionReady = bLockOnActive && LockOnTargetInterface && LockOnTargetInterface->IsLockOnExecutionReady();
		const float NormalTargetArmLength = CameraMode == EJunCameraMode::Death
			? (SpringArm ? SpringArm->TargetArmLength : DefaultSpringArmLength)
			: (bTargetExecutionReady
				? ExecutionReadyLockOnArmLength
				: GetPitchAdjustedSpringArmLength());
		const float TargetArmLength = bDialogueCameraActive
			? DialogueCameraArmLength
			: NormalTargetArmLength;
		const bool bShouldShortenArm = TargetArmLength < SpringArm->TargetArmLength;
		const float ArmInterpSpeed = bDialogueCameraActive
			? DialogueCameraArmLengthInterpSpeed
			: (bDialogueCameraRestoring
				? DialogueCameraArmLengthRestoreInterpSpeed
				: (bShouldShortenArm
				? PitchArmShortenInterpSpeed
				: GetCurrentExecutionCameraArmLengthRestoreInterpSpeed()));

		SpringArm->TargetArmLength = FMath::FInterpTo(
			SpringArm->TargetArmLength,
			TargetArmLength,
			DeltaTime,
			ArmInterpSpeed
		);

		if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, TargetArmLength, 1.f))
		{
			bExecutionCameraRestoreUsesFinishTuning = false;
			bDialogueCameraRestoring = false;
		}
	}

	UpdateCameraFOV(DeltaTime);
}

void AJunPlayer::UpdateCameraFOV(float DeltaTime)
{
	if (!Camera)
	{
		return;
	}

	float TargetFOV = FreeCameraFOV;
	if (CameraMode == EJunCameraMode::Execution)
	{
		TargetFOV = GetCurrentExecutionCameraFOV();
	}
	else if (bLockOnActive)
	{
		const IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(LockOnTarget.Get());
		TargetFOV = LockOnTargetInterface && LockOnTargetInterface->IsLockOnExecutionReady()
			? ExecutionReadyLockOnFOV
			: LockOnCameraFOV;
	}

	const float NewFOV = FMath::FInterpTo(
		Camera->FieldOfView,
		TargetFOV,
		DeltaTime,
		CameraFOVInterpSpeed
	);

	Camera->SetFieldOfView(NewFOV);
}

float AJunPlayer::GetPitchAdjustedSpringArmLength() const
{
	const float PitchAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(PitchArmShortenStartPitch, PitchArmShortenEndPitch),
		FVector2D(0.f, 1.f),
		FMath::Max(TargetCameraPitch, 0.f)
	);

	return FMath::Lerp(DefaultSpringArmLength, PitchShortenedSpringArmLength, PitchAlpha);
}

void AJunPlayer::UpdateFreeCamera(float DeltaTime)
{
	bFreeCameraYawInputActiveThisFrame = false;

	if (!Controller)
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FVector2D Input = PendingCameraLookInput;
	bFreeCameraYawInputActiveThisFrame = !FMath::IsNearlyZero(Input.X);

	if (GetVelocity().SizeSquared2D() > 0.f)
	{
		Input *= MovingLookInputScale;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Attack))
	{
		Input *= AttackLookInputScale;
	}

	TargetCameraYaw += Input.X * FreeCameraYawSpeed * DeltaTime;
	TargetCameraPitch += Input.Y * FreeCameraPitchSpeed * DeltaTime;
	TargetCameraPitch = FMath::Clamp(TargetCameraPitch, MinCameraPitch, MaxCameraPitch);

	const FRotator CurrentRot = Controller->GetControlRotation();
	const FRotator TargetRot(TargetCameraPitch, TargetCameraYaw, 0.f);

	const FRotator NewRot = FMath::RInterpTo(
		CurrentRot,
		TargetRot,
		DeltaTime,
		FreeCameraRotationInterpSpeed
	);

	Controller->SetControlRotation(NewRot);
	PendingCameraLookInput = FVector2D::ZeroVector;
}

FVector AJunPlayer::GetCameraForwardOnPlane() const
{
	if (!Controller)
	{
		return GetActorForwardVector();
	}

	FRotator ControlRot = Controller->GetControlRotation();
	ControlRot.Pitch = 0.f;
	ControlRot.Roll = 0.f;

	return ControlRot.Vector().GetSafeNormal();
}

void AJunPlayer::StartLockOn(AJunCharacter* NewTarget)
{
	if (!NewTarget)
	{
		return;
	}

	bLockOnActive = true;
	LockOnTarget = NewTarget;
	CameraMode = EJunCameraMode::LockOn;
	CachedLockOnTargetPoint = FVector::ZeroVector;
	CachedLockOnRangeAlpha = -1.f;
	SmoothedLockOnDistance2D = -1.f;
	SmoothedLockOnPitchOffset = LockOnPitchOffset;
	bLockOnClosePitchModeActive = false;
	bLockOnFarPitchModeActive = false;
	SmoothedLockOnOcclusionLateralOffset = 0.f;
	CachedLockOnAimDirection2D = FVector::ZeroVector;
	bLockOnCloseAimStabilizationActive = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(NewTarget))
	{
		LockOnTargetInterface->OnLockedOnBy(this);
	}
}

void AJunPlayer::EndLockOn()
{
	bLockOnActive = false;
	LockOnTarget = nullptr;
	CameraMode = EJunCameraMode::Free;
	CachedLockOnTargetPoint = FVector::ZeroVector;
	CachedLockOnRangeAlpha = -1.f;
	SmoothedLockOnDistance2D = -1.f;
	SmoothedLockOnPitchOffset = LockOnPitchOffset;
	bLockOnClosePitchModeActive = false;
	bLockOnFarPitchModeActive = false;
	SmoothedLockOnOcclusionLateralOffset = 0.f;
	CachedLockOnAimDirection2D = FVector::ZeroVector;
	bLockOnCloseAimStabilizationActive = false;
	CancelLockOnTurn(0.05f);

	GetCharacterMovement()->bOrientRotationToMovement = true;
}

bool AJunPlayer::IsLockOnTargetValid() const
{
	if (!LockOnTarget)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), LockOnTarget->GetActorLocation());
	return DistSq <= FMath::Square(LockOnBreakDistance);
}

bool AJunPlayer::ShouldUseJumpCounterLockOnCameraPitch() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		return false;
	}

	const AActor* JumpCounterTarget = JumpCounterStompTarget.Get();
	if (!JumpCounterTarget)
	{
		JumpCounterTarget = LockOnTarget.Get();
	}

	if (!JumpCounterTarget)
	{
		return false;
	}

	const float MaxPitchDistance = FMath::Max(0.f, JumpCounterLockOnPitchMaxDistance);
	if (FVector::DistSquared2D(GetActorLocation(), JumpCounterTarget->GetActorLocation()) > FMath::Square(MaxPitchDistance))
	{
		return false;
	}

	if (bJumpCounterStompFollowUpActive || bJumpCounterStompFollowUpAvailable)
	{
		return true;
	}

	const IJunJumpCounterTargetInterface* JumpCounterTargetInterface = Cast<IJunJumpCounterTargetInterface>(LockOnTarget.Get());
	return JumpCounterTargetInterface && JumpCounterTargetInterface->IsJumpCounterStompThreatActive();
}

void AJunPlayer::UpdateLockOnCamera(float DeltaTime)
{
	if (!Controller || !LockOnTarget)
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FLockOnCameraDesiredState DesiredState;
	if (EvaluateLockOnCameraDesiredState(DeltaTime, DesiredState))
	{
		ApplyLockOnCameraDesiredState(DesiredState, DeltaTime);
	}

	PendingCameraLookInput = FVector2D::ZeroVector;
}

bool AJunPlayer::EvaluateLockOnCameraDesiredState(
	float DeltaTime,
	FLockOnCameraDesiredState& OutState)
{
	if (!Controller || !LockOnTarget)
	{
		return false;
	}

	FVector CameraBasePoint = CameraAnchor
		? CameraAnchor->GetComponentLocation()
		: GetActorLocation();
	CameraBasePoint.Z += 50.f;

	const FVector TargetPoint = GetFilteredLockOnTargetPoint();
	const float PlayerTargetDistance2D = FVector::Dist2D(GetActorLocation(), LockOnTarget->GetActorLocation());
	if (SmoothedLockOnDistance2D < 0.f)
	{
		SmoothedLockOnDistance2D = PlayerTargetDistance2D;
	}
	else
	{
		SmoothedLockOnDistance2D = FMath::FInterpTo(
			SmoothedLockOnDistance2D,
			PlayerTargetDistance2D,
			DeltaTime,
			LockOnDistanceInterpSpeed);
	}

	const float FarPitchEnterDistance = FMath::Max(0.f, LockOnFarPitchEnterDistance);
	const float FarPitchExitDistance = FMath::Clamp(LockOnFarPitchExitDistance, 0.f, FarPitchEnterDistance);
	if (!bLockOnFarPitchModeActive && SmoothedLockOnDistance2D >= FarPitchEnterDistance)
	{
		bLockOnFarPitchModeActive = true;
	}
	else if (bLockOnFarPitchModeActive && SmoothedLockOnDistance2D <= FarPitchExitDistance)
	{
		bLockOnFarPitchModeActive = false;
	}

	const float RawCloseRangeAlpha = bLockOnFarPitchModeActive
		? 1.f
		: FMath::Clamp(
			(SmoothedLockOnDistance2D - LockOnCloseDistance) / FMath::Max(LockOnFarDistance - LockOnCloseDistance, 1.f),
			0.f,
			1.f
		);

	if (CachedLockOnRangeAlpha < 0.f)
	{
		CachedLockOnRangeAlpha = RawCloseRangeAlpha;
	}
	else
	{
		CachedLockOnRangeAlpha = FMath::FInterpTo(
			CachedLockOnRangeAlpha,
			RawCloseRangeAlpha,
			DeltaTime,
			LockOnRangeAlphaInterpSpeed
		);
	}

	const float CloseRangeAlpha = CachedLockOnRangeAlpha;
	const float TargetBlend = FMath::Lerp(LockOnCloseTargetBlend, 1.f, CloseRangeAlpha);

	FVector PlayerFocusPoint = CameraBasePoint;
	const FVector EffectiveTargetPoint = FMath::Lerp(PlayerFocusPoint, TargetPoint, TargetBlend);
	FVector ToTarget = EffectiveTargetPoint - CameraBasePoint;
	const FVector DebugSpherePoint = GetLockOnDebugSpherePoint();

	//DrawDebugSphere(GetWorld(), DebugSpherePoint, 6.f, 12, FColor::Red, false, 0.f);

	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	FVector DesiredAimDirection2D(ToTarget.X, ToTarget.Y, 0.f);
	if (!DesiredAimDirection2D.IsNearlyZero())
	{
		DesiredAimDirection2D.Normalize();
		CachedLockOnAimDirection2D = DesiredAimDirection2D;
		bLockOnCloseAimStabilizationActive = false;
	}

	const float Distance2D = FVector(ToTarget.X, ToTarget.Y, 0.f).Length();
	const float DeltaZ = ToTarget.Z;

	FRotator DesiredRot = ToTarget.Rotation();
	DesiredRot.Roll = 0.f;

	const float TargetPitchDeg = FMath::RadiansToDegrees(
		FMath::Atan2(DeltaZ, FMath::Max(Distance2D, 1.f))
	);

	const float LockOnDistanceAlpha = CloseRangeAlpha;
	const bool bUseJumpCounterPitch = ShouldUseJumpCounterLockOnCameraPitch();
	const float ClosePitchEnterDistance = FMath::Max(0.f, LockOnClosePitchEnterDistance);
	const float ClosePitchExitDistance = FMath::Max(ClosePitchEnterDistance, LockOnClosePitchExitDistance);
	if (!bLockOnClosePitchModeActive && SmoothedLockOnDistance2D <= ClosePitchEnterDistance)
	{
		bLockOnClosePitchModeActive = true;
	}
	else if (bLockOnClosePitchModeActive && SmoothedLockOnDistance2D >= ClosePitchExitDistance)
	{
		bLockOnClosePitchModeActive = false;
	}

	const IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(LockOnTarget.Get());
	const bool bTargetExecutionReady = LockOnTargetInterface && LockOnTargetInterface->IsLockOnExecutionReady();
	const float TargetLockOnPitchOffset = bTargetExecutionReady
		? ExecutionReadyLockOnPitchOffset
		: (bUseJumpCounterPitch
		? JumpCounterLockOnPitchOffset
		: (bLockOnFarPitchModeActive
			? LockOnPitchOffset
			: (bLockOnClosePitchModeActive
			? LockOnClosePitchOffset
			: FMath::Lerp(
				LockOnClosePitchOffset,
				LockOnPitchOffset,
				LockOnDistanceAlpha
				))));
	if (!FMath::IsFinite(SmoothedLockOnPitchOffset))
	{
		SmoothedLockOnPitchOffset = TargetLockOnPitchOffset;
	}
	else
	{
		SmoothedLockOnPitchOffset = FMath::FInterpTo(
			SmoothedLockOnPitchOffset,
			TargetLockOnPitchOffset,
			DeltaTime,
			LockOnPitchOffsetInterpSpeed);
	}

	DesiredRot.Pitch = SmoothedLockOnPitchOffset + TargetPitchDeg;

	const FRotator CurrentRot = Controller->GetControlRotation();

	if (GetCharacterMovement()->IsFalling())
	{
		DesiredRot.Pitch = FMath::Lerp(CurrentRot.Pitch, DesiredRot.Pitch, 0.35f);
	}

	const float ResolvedMinPitch = bUseJumpCounterPitch
		? JumpCounterMinLockOnCameraPitch
		: MinLockOnCameraPitch;
	DesiredRot.Pitch = FMath::Clamp(DesiredRot.Pitch, ResolvedMinPitch, MaxLockOnCameraPitch);

	SmoothedLockOnOcclusionLateralOffset = 0.f;

	const float DynamicLockOnRotationInterpSpeed = FMath::Lerp(
		LockOnCloseRotationInterpSpeed,
		LockOnRotationInterpSpeed,
		CloseRangeAlpha
	);

	const float DynamicLockOnPitchRotationInterpSpeed = FMath::Lerp(
		LockOnClosePitchRotationInterpSpeed,
		LockOnPitchRotationInterpSpeed,
		CloseRangeAlpha
	);

	OutState.FocusPoint = EffectiveTargetPoint;
	OutState.DesiredRotation = DesiredRot;
	OutState.DistanceAlpha = CloseRangeAlpha;
	OutState.YawInterpSpeed = DynamicLockOnRotationInterpSpeed;
	OutState.PitchInterpSpeed = DynamicLockOnPitchRotationInterpSpeed;
	return true;
}

void AJunPlayer::ApplyLockOnCameraDesiredState(
	const FLockOnCameraDesiredState& State,
	float DeltaTime)
{
	if (!Controller)
	{
		return;
	}

	const FRotator CurrentRot = Controller->GetControlRotation();

	const FRotator NewYawRot = FMath::RInterpTo(
		FRotator(0.f, CurrentRot.Yaw, 0.f),
		FRotator(0.f, State.DesiredRotation.Yaw, 0.f),
		DeltaTime,
		State.YawInterpSpeed);
	const float NewPitch = FMath::FInterpTo(
		CurrentRot.Pitch,
		State.DesiredRotation.Pitch,
		DeltaTime,
		State.PitchInterpSpeed);
	const FRotator NewRot(NewPitch, NewYawRot.Yaw, 0.f);

	Controller->SetControlRotation(NewRot);
	TargetCameraYaw = NewRot.Yaw;
	TargetCameraPitch = NewRot.Pitch;
}

void AJunPlayer::StartExecutionCamera(AActor* ExecutionTargetActor)
{
	if (!ExecutionTargetActor)
	{
		return;
	}

	const IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionTargetActor);
	if (!ExecutionTarget)
	{
		return;
	}

	CameraModeBeforeExecution = CameraMode;
	ExecutionCameraTarget = ExecutionTargetActor;
	bCurrentExecutionIsFinish = ExecutionTarget->IsFinalExecution();
	bExecutionCameraRestoreUsesFinishTuning = false;
	bExecutionCameraSecondStage = false;
	bExecutionCameraFinishPullInStage = false;
	CameraMode = EJunCameraMode::Execution;
	CancelLockOnTurn(0.05f);
	PendingCameraLookInput = FVector2D::ZeroVector;
}

void AJunPlayer::EndExecutionCamera()
{
	if (CameraMode != EJunCameraMode::Execution)
	{
		ExecutionCameraTarget = nullptr;
		bExecutionCameraSecondStage = false;
		bExecutionCameraFinishPullInStage = false;
		bCurrentExecutionIsFinish = false;
		return;
	}

	CameraMode = bLockOnActive && LockOnTarget ? EJunCameraMode::LockOn : EJunCameraMode::Free;
	ExecutionCameraTarget = nullptr;
	bExecutionCameraSecondStage = false;
	bExecutionCameraFinishPullInStage = false;
	bExecutionCameraRestoreUsesFinishTuning = bCurrentExecutionIsFinish;
	bCurrentExecutionIsFinish = false;
}

void AJunPlayer::AdvanceExecutionCameraStage()
{
	if (CameraMode == EJunCameraMode::Execution)
	{
		bExecutionCameraSecondStage = true;
	}
}

void AJunPlayer::AdvanceExecutionFinishCameraPullInStage()
{
	if (CameraMode == EJunCameraMode::Execution && bCurrentExecutionIsFinish)
	{
		bExecutionCameraSecondStage = true;
		bExecutionCameraFinishPullInStage = true;
	}
}

void AJunPlayer::StartDeathCamera()
{
	if (bLockOnActive)
	{
		EndLockOn();
	}

	CameraMode = EJunCameraMode::Death;
	CancelLockOnTurn(0.05f);
	PendingCameraLookInput = FVector2D::ZeroVector;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void AJunPlayer::EndDeathCamera()
{
	if (CameraMode == EJunCameraMode::Death)
	{
		CameraMode = EJunCameraMode::Free;
	}
}

void AJunPlayer::UpdateDeathCamera(float DeltaTime)
{
	if (!Controller)
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FVector CameraBasePoint = CameraAnchor
		? CameraAnchor->GetComponentLocation()
		: GetActorLocation();
	CameraBasePoint.Z += 50.f;

	FVector TargetPoint = GetActorLocation();
	TargetPoint.Z += DeathCameraTargetHeight;

	const FVector ToTarget = TargetPoint - CameraBasePoint;
	if (ToTarget.IsNearlyZero())
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FRotator DesiredRot = ToTarget.Rotation();
	DesiredRot.Pitch += DeathCameraPitchOffset;
	DesiredRot.Roll = 0.f;
	DesiredRot.Pitch = FMath::Clamp(DesiredRot.Pitch, MinCameraPitch, MaxCameraPitch);

	const FRotator NewRot = FMath::RInterpTo(
		Controller->GetControlRotation(),
		DesiredRot,
		DeltaTime,
		DeathCameraRotationInterpSpeed
	);

	Controller->SetControlRotation(NewRot);
	TargetCameraYaw = NewRot.Yaw;
	TargetCameraPitch = NewRot.Pitch;
	PendingCameraLookInput = FVector2D::ZeroVector;
}

void AJunPlayer::UpdateExecutionCamera(float DeltaTime)
{
	const IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(ExecutionCameraTarget.Get());
	if (!Controller || !ExecutionCameraTarget || !ExecutionTarget)
	{
		EndExecutionCamera();
		return;
	}

	FVector CameraBasePoint = CameraAnchor
		? CameraAnchor->GetComponentLocation()
		: GetActorLocation();
	CameraBasePoint.Z += 50.f;

	const FVector TargetPoint = ExecutionTarget->GetExecutionCameraTargetPoint();
	const FVector ToTarget = TargetPoint - CameraBasePoint;
	if (ToTarget.IsNearlyZero())
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FRotator DesiredRot = ToTarget.Rotation();
	DesiredRot.Roll = 0.f;
	DesiredRot.Pitch = GetCurrentExecutionCameraPitchOffset();
	DesiredRot.Pitch = FMath::Clamp(DesiredRot.Pitch, MinCameraPitch, MaxCameraPitch);

	const FRotator NewRot = FMath::RInterpTo(
		Controller->GetControlRotation(),
		DesiredRot,
		DeltaTime,
		GetCurrentExecutionCameraRotationInterpSpeed()
	);

	Controller->SetControlRotation(NewRot);
	TargetCameraYaw = NewRot.Yaw;
	TargetCameraPitch = NewRot.Pitch;
	PendingCameraLookInput = FVector2D::ZeroVector;

	if (SpringArm)
	{
		float TargetArmLength = GetCurrentExecutionCameraArmLength();
		if (bExecutionCameraFinishPullInStage)
		{
			TargetArmLength = ExecutionFinishCameraPullInArmLength;
		}
		else if (bExecutionCameraSecondStage)
		{
			TargetArmLength = GetCurrentExecutionCameraApplyArmLength();
		}

		SpringArm->TargetArmLength = FMath::FInterpTo(
			SpringArm->TargetArmLength,
			TargetArmLength,
			DeltaTime,
			GetCurrentExecutionCameraArmLengthInterpSpeed()
		);
	}
}

FVector AJunPlayer::GetCurrentExecutionCameraSocketOffset() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraSocketOffset : ExecutionCameraSocketOffset;
}

float AJunPlayer::GetCurrentExecutionCameraFOV() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraFOV : ExecutionCameraFOV;
}

float AJunPlayer::GetCurrentExecutionCameraArmLength() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraArmLength : ExecutionCameraArmLength;
}

float AJunPlayer::GetCurrentExecutionCameraApplyArmLength() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraApplyArmLength : ExecutionCameraApplyArmLength;
}

float AJunPlayer::GetCurrentExecutionCameraRotationInterpSpeed() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraRotationInterpSpeed : ExecutionCameraRotationInterpSpeed;
}

float AJunPlayer::GetCurrentExecutionCameraArmLengthInterpSpeed() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraArmLengthInterpSpeed : ExecutionCameraArmLengthInterpSpeed;
}

float AJunPlayer::GetCurrentExecutionCameraArmLengthRestoreInterpSpeed() const
{
	return bExecutionCameraRestoreUsesFinishTuning
		? ExecutionFinishCameraArmLengthRestoreInterpSpeed
		: ExecutionCameraArmLengthRestoreInterpSpeed;
}

float AJunPlayer::GetCurrentExecutionCameraPitchOffset() const
{
	return bCurrentExecutionIsFinish ? ExecutionFinishCameraPitchOffset : ExecutionCameraPitchOffset;
}

void AJunPlayer::UpdateLockOnCharacterRotation(float DeltaTime)
{
	if (!bLockOnActive || !LockOnTarget || IsLockOnTurnPlaying())
	{
		return;
	}

	FVector ToTarget = LockOnTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRot = GetActorRotation();
	const FRotator DesiredRot = ToTarget.Rotation();
	const bool bUseSideDashRotationSpeed =
		HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		(IsSideDashMontage(CurrentDodgeMontage) ||
			CurrentDodgeMontage == LockOnDashForwardLeftMontage ||
			CurrentDodgeMontage == LockOnDashForwardRightMontage);
	const float RotationInterpSpeed = bUseSideDashRotationSpeed
		? LockOnSideDashRotationInterpSpeed
		: LockOnCharacterRotationInterpSpeed;

	const FRotator NewRot = FMath::RInterpTo(
		CurrentRot,
		DesiredRot,
		DeltaTime,
		RotationInterpSpeed
	);

	SetActorRotation(NewRot);
}

FVector AJunPlayer::GetRawLockOnBonePoint() const
{
	if (!LockOnTarget)
	{
		return FVector::ZeroVector;
	}

	USkeletalMeshComponent* TargetMesh = LockOnTarget->GetMesh();
	if (!TargetMesh)
	{
		return LockOnTarget->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	}

	static const FName LockOnBoneName(TEXT("spine_03"));

	if (TargetMesh->GetBoneIndex(LockOnBoneName) != INDEX_NONE)
	{
		// 이 본 위치는 락온 UI 그릴때 사용
		return TargetMesh->GetBoneLocation(LockOnBoneName);
	}

	return LockOnTarget->GetActorLocation() + FVector(0.f, 0.f, 40.f);
}

FVector AJunPlayer::GetLockOnDebugSpherePoint()
{
	if (!LockOnTarget)
	{
		return FVector::ZeroVector;
	}

	FVector CameraBasePoint = CameraAnchor
		? CameraAnchor->GetComponentLocation()
		: GetActorLocation();
	CameraBasePoint.Z += 50.f;

	const FVector TargetPoint = GetFilteredLockOnTargetPoint();
	const FVector ToTarget = TargetPoint - CameraBasePoint;
	if (ToTarget.IsNearlyZero())
	{
		return GetRawLockOnBonePoint();
	}

	return GetRawLockOnBonePoint() - (ToTarget.GetSafeNormal() * 20.f);
}

FVector AJunPlayer::GetRawLockOnCapsulePoint() const
{
	if (!LockOnTarget)
	{
		return FVector::ZeroVector;
	}

	return LockOnTarget->GetActorLocation() + FVector(0.f, 0.f, 40.f);
}

FVector AJunPlayer::GetFilteredLockOnTargetPoint()
{
	FVector RawPoint = GetRawLockOnCapsulePoint();
	if (LockOnTarget)
	{
		const IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(LockOnTarget.Get());
		const FVector TargetLockOnPoint = LockOnTargetInterface
			? LockOnTargetInterface->GetLockOnCameraTargetPoint()
			: LockOnTarget->GetLockOnTargetPoint();
		RawPoint.Z = TargetLockOnPoint.Z;
	}
	
	if (CachedLockOnTargetPoint.IsNearlyZero())
	{
		CachedLockOnTargetPoint = RawPoint;
		return CachedLockOnTargetPoint;
	}

	const float ZDelta = FMath::Abs(RawPoint.Z - CachedLockOnTargetPoint.Z);
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;

	if (LockOnTargetXYInterpSpeed <= 0.f)
	{
		CachedLockOnTargetPoint.X = RawPoint.X;
		CachedLockOnTargetPoint.Y = RawPoint.Y;
	}
	else
	{
		const FVector InterpedPoint = FMath::VInterpTo(
			CachedLockOnTargetPoint,
			FVector(RawPoint.X, RawPoint.Y, CachedLockOnTargetPoint.Z),
			DeltaTime,
			LockOnTargetXYInterpSpeed);
		CachedLockOnTargetPoint.X = InterpedPoint.X;
		CachedLockOnTargetPoint.Y = InterpedPoint.Y;
	}

	if (ZDelta >= LockOnTargetZIgnoreThreshold)
	{
		const float ZInterpSpeed = RawPoint.Z > CachedLockOnTargetPoint.Z
			? LockOnTargetZRiseInterpSpeed
			: LockOnTargetZFallInterpSpeed;

		CachedLockOnTargetPoint.Z = FMath::FInterpTo(
			CachedLockOnTargetPoint.Z,
			RawPoint.Z,
			DeltaTime,
			ZInterpSpeed
		);
	}

	return CachedLockOnTargetPoint;
}

#pragma endregion
