#include "Character/Monster/Boss/JunBoss.h"

#include "Character/Player/JunPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const TCHAR* GetBossCodeMoveStopReasonDebugText(EBossCodeMoveStopReason StopReason)
	{
		switch (StopReason)
		{
		case EBossCodeMoveStopReason::Started:
			return TEXT("Started");
		case EBossCodeMoveStopReason::TooClose:
			return TEXT("TooClose");
		case EBossCodeMoveStopReason::MaxDistance:
			return TEXT("MaxDistance");
		case EBossCodeMoveStopReason::DurationEnd:
			return TEXT("DurationEnd");
		case EBossCodeMoveStopReason::LostTarget:
			return TEXT("LostTarget");
		case EBossCodeMoveStopReason::InvalidMovement:
			return TEXT("InvalidMovement");
		case EBossCodeMoveStopReason::Interrupted:
			return TEXT("Interrupted");
		case EBossCodeMoveStopReason::None:
		default:
			return TEXT("None");
		}
	}
}

void AJunBoss::OnAttackTick(float DeltaTime)
{
	Super::OnAttackTick(DeltaTime);

	if (CurrentAttackRuntime.ActionType == EBossActionType::ComboAttack)
	{
		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* ComboMontage = GetPlannedComboMontage();
		if (AnimInstance && ComboMontage && !AnimInstance->Montage_IsPlaying(ComboMontage))
		{
			FinishAttack();
			return;
		}
	}

	if (CodeMoveState.bPending)
	{
		CodeMoveState.DelayRemainTime = FMath::Max(0.f, CodeMoveState.DelayRemainTime - DeltaTime);
		if (CodeMoveState.DelayRemainTime <= 0.f)
		{
			StartCodeDrivenAttackMove(CodeMoveState.AttackType);
		}
	}

	UpdateCodeDrivenAttackMove(DeltaTime);
	UpdateLightningSwordAirMove(DeltaTime);

	if (bAttackGroundMotionOverrideActive)
	{
		if (AttackGroundMotionOverrideRemainTime > 0.f)
		{
			AttackGroundMotionOverrideRemainTime = FMath::Max(0.f, AttackGroundMotionOverrideRemainTime - DeltaTime);
		}

		if (!GetCharacterMovement()->IsMovingOnGround() || AttackGroundMotionOverrideRemainTime == 0.f)
		{
			StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::DurationEnd);
		}
	}
}

void AJunBoss::QueueCodeDrivenAttackMove(EBossNormalAttackType AttackType, float Delay)
{
	const FBossNormalAttackData* AttackData = GetNormalAttackData(AttackType);
	if (!AttackData || AttackData->MoveSpeed <= 0.f)
	{
		CacheCodeDrivenAttackMoveDebug(EBossCodeMoveStopReason::InvalidMovement);
		return;
	}

	CodeMoveState.bPending = true;
	CodeMoveState.bActive = false;
	CodeMoveState.AttackType = AttackType;
	CodeMoveState.DelayRemainTime = FMath::Max(0.f, Delay);
	CacheCodeDrivenAttackMoveDebug(EBossCodeMoveStopReason::Started);
	if (CodeMoveState.DelayRemainTime <= 0.f)
	{
		StartCodeDrivenAttackMove(AttackType);
	}
}

void AJunBoss::BeginCodeDrivenAttackMoveWindow(EBossNormalAttackType AttackType)
{
	if (AttackType == EBossNormalAttackType::None)
	{
		AttackType = CurrentAttackRuntime.NormalAttackType;
	}

	CodeMoveState.bPending = false;
	CodeMoveState.DelayRemainTime = 0.f;
	StartCodeDrivenAttackMove(AttackType);
}

void AJunBoss::EndCodeDrivenAttackMoveWindow()
{
	StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::DurationEnd);
}

void AJunBoss::BeginLightningSwordAirMoveWindow(
	EBossLightningSwordAirMoveMode MoveMode,
	float TargetHeightOffset,
	float RiseSpeed,
	float DiveSpeed,
	float DiveGravityScale,
	bool bLockHorizontalVelocity)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (!bLightningSwordAirMoveActive)
	{
		LightningSwordAirMovePreviousMovementMode = MovementComponent->MovementMode;
		LightningSwordAirMovePreviousCustomMovementMode = MovementComponent->CustomMovementMode;
		LightningSwordAirMovePreviousGravityScale = MovementComponent->GravityScale;
	}

	bLightningSwordAirMoveActive = true;
	LightningSwordAirMoveMode = MoveMode;
	LightningSwordAirMoveTargetZ = (MoveMode == EBossLightningSwordAirMoveMode::Hover)
		? GetActorLocation().Z
		: GetActorLocation().Z + FMath::Max(0.f, TargetHeightOffset);
	LightningSwordAirMoveRiseSpeed = FMath::Max(0.f, RiseSpeed);
	LightningSwordAirMoveDiveSpeed = FMath::Max(0.f, DiveSpeed);
	LightningSwordAirMoveDiveGravityScale = FMath::Max(0.f, DiveGravityScale);
	bLightningSwordAirMoveLockHorizontalVelocity = bLockHorizontalVelocity;

	FVector NewVelocity = MovementComponent->Velocity;
	if (bLightningSwordAirMoveLockHorizontalVelocity)
	{
		NewVelocity.X = 0.f;
		NewVelocity.Y = 0.f;
	}

	if (MoveMode == EBossLightningSwordAirMoveMode::Dive)
	{
		MovementComponent->GravityScale = LightningSwordAirMoveDiveGravityScale;
		MovementComponent->SetMovementMode(MOVE_Falling);
		NewVelocity.Z = -LightningSwordAirMoveDiveSpeed;
	}
	else
	{
		MovementComponent->GravityScale = 0.f;
		MovementComponent->SetMovementMode(MOVE_Flying);
		NewVelocity.Z = 0.f;
	}

	MovementComponent->Velocity = NewVelocity;
}

void AJunBoss::EndLightningSwordAirMoveWindow(EBossLightningSwordAirMoveMode MoveMode)
{
	if (!bLightningSwordAirMoveActive || LightningSwordAirMoveMode != MoveMode)
	{
		return;
	}

	StopLightningSwordAirMove(true);
}

void AJunBoss::StartCodeDrivenAttackMove(EBossNormalAttackType AttackType)
{
	const FBossNormalAttackData* AttackData = GetNormalAttackData(AttackType);
	if (!CurrentTarget || !AttackData || AttackData->MoveSpeed <= 0.f)
	{
		StopCodeDrivenAttackMove(true, !CurrentTarget ? EBossCodeMoveStopReason::LostTarget : EBossCodeMoveStopReason::InvalidMovement);
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::InvalidMovement);
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::InvalidMovement);
		return;
	}

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	const float DistanceToTarget = ToTarget.Size();
	const float StopDistance = FMath::Max(AttackData->MoveStandOffDistance, AttackData->CodeMoveStopDistance);
	if (DistanceToTarget <= StopDistance)
	{
		StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::TooClose);
		return;
	}

	const float GroundMotionOverrideDuration = AttackData->bUseAnimNotifyTiming
		? -1.f
		: AttackData->GroundMotionOverrideDuration;
	ApplyAttackGroundMotionOverride(GroundMotionOverrideDuration);

	CodeMoveState.bPending = false;
	CodeMoveState.bActive = true;
	CodeMoveState.AttackType = AttackType;
	CodeMoveState.DelayRemainTime = 0.f;
	CodeMoveState.StartLocation = GetActorLocation();
	CacheCodeDrivenAttackMoveDebug(EBossCodeMoveStopReason::Started);

	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.X = MoveDirection.X * AttackData->MoveSpeed;
	NewVelocity.Y = MoveDirection.Y * AttackData->MoveSpeed;
	MovementComponent->Velocity = NewVelocity;
}

void AJunBoss::UpdateCodeDrivenAttackMove(float DeltaTime)
{
	if (!CodeMoveState.bActive)
	{
		return;
	}

	const FBossNormalAttackData* AttackData = GetNormalAttackData(CodeMoveState.AttackType);
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!CurrentTarget || !AttackData || !MovementComponent)
	{
		StopCodeDrivenAttackMove(true, !CurrentTarget ? EBossCodeMoveStopReason::LostTarget : EBossCodeMoveStopReason::InvalidMovement);
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	const float DistanceToTarget = ToTarget.Size();
	const float StopDistance = FMath::Max(AttackData->MoveStandOffDistance, AttackData->CodeMoveStopDistance);
	if (DistanceToTarget <= StopDistance)
	{
		StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::TooClose);
		return;
	}

	const float MaxDistance = AttackData->CodeMoveMaxDistance;
	if (MaxDistance > 0.f &&
		FVector::Dist2D(CodeMoveState.StartLocation, GetActorLocation()) >= MaxDistance)
	{
		StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::MaxDistance);
		return;
	}

	if (ToTarget.IsNearlyZero())
	{
		StopCodeDrivenAttackMove(true, EBossCodeMoveStopReason::InvalidMovement);
		return;
	}

	FVector Velocity = MovementComponent->Velocity;
	FVector MoveDirection = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
	if (MoveDirection.IsNearlyZero() || AttackData->bTrackTargetDuringCodeMove)
	{
		MoveDirection = ToTarget.GetSafeNormal();
	}

	if (AttackData->bTrackTargetDuringCodeMove && AttackData->AirTrackInterpSpeed > 0.f)
	{
		const FVector CurrentHorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
		const FVector TargetHorizontalVelocity = ToTarget.GetSafeNormal() * AttackData->MoveSpeed;
		const FVector NewHorizontalVelocity = FMath::VInterpTo(
			CurrentHorizontalVelocity,
			TargetHorizontalVelocity,
			DeltaTime,
			AttackData->AirTrackInterpSpeed
		);
		Velocity.X = NewHorizontalVelocity.X;
		Velocity.Y = NewHorizontalVelocity.Y;
	}
	else
	{
		Velocity.X = MoveDirection.X * AttackData->MoveSpeed;
		Velocity.Y = MoveDirection.Y * AttackData->MoveSpeed;
	}

	MovementComponent->Velocity = Velocity;
}

void AJunBoss::StopCodeDrivenAttackMove(bool bClearVelocity, EBossCodeMoveStopReason StopReason)
{
	CacheCodeDrivenAttackMoveDebug(StopReason);
	CodeMoveState.ResetMotion();

	if (bClearVelocity)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			FVector NewVelocity = MovementComponent->Velocity;
			NewVelocity.X = 0.f;
			NewVelocity.Y = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}

	RestoreAttackGroundMotionOverride();
}

void AJunBoss::CacheCodeDrivenAttackMoveDebug(EBossCodeMoveStopReason StopReason)
{
	CodeMoveState.LastStopReason = StopReason;

	const FBossNormalAttackData* AttackData = GetNormalAttackData(CodeMoveState.AttackType);
	CodeMoveState.LastStopDistance = AttackData
		? FMath::Max(AttackData->MoveStandOffDistance, AttackData->CodeMoveStopDistance)
		: 0.f;
	CodeMoveState.LastMaxDistance = AttackData ? AttackData->CodeMoveMaxDistance : 0.f;
	CodeMoveState.LastTravelDistance = CodeMoveState.StartLocation.IsNearlyZero()
		? 0.f
		: FVector::Dist2D(CodeMoveState.StartLocation, GetActorLocation());

	if (CurrentTarget)
	{
		CodeMoveState.LastTargetDistance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	}
	else
	{
		CodeMoveState.LastTargetDistance = 0.f;
	}
}

void AJunBoss::UpdateLightningSwordAirMove(float DeltaTime)
{
	if (!bLightningSwordAirMoveActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		StopLightningSwordAirMove(false);
		return;
	}

	FVector NewVelocity = MovementComponent->Velocity;
	if (bLightningSwordAirMoveLockHorizontalVelocity)
	{
		NewVelocity.X = 0.f;
		NewVelocity.Y = 0.f;
	}

	switch (LightningSwordAirMoveMode)
	{
	case EBossLightningSwordAirMoveMode::LiftAndHover:
	{
		if (MovementComponent->MovementMode != MOVE_Flying)
		{
			MovementComponent->SetMovementMode(MOVE_Flying);
		}

		const float ZDelta = LightningSwordAirMoveTargetZ - GetActorLocation().Z;
		if (ZDelta > 1.f && DeltaTime > KINDA_SMALL_NUMBER)
		{
			NewVelocity.Z = FMath::Min(LightningSwordAirMoveRiseSpeed, ZDelta / DeltaTime);
		}
		else
		{
			NewVelocity.Z = 0.f;
		}
		break;
	}
	case EBossLightningSwordAirMoveMode::Hover:
	{
		if (MovementComponent->MovementMode != MOVE_Flying)
		{
			MovementComponent->SetMovementMode(MOVE_Flying);
		}

		NewVelocity.Z = 0.f;
		break;
	}
	case EBossLightningSwordAirMoveMode::Dive:
	{
		if (MovementComponent->IsMovingOnGround())
		{
			StopLightningSwordAirMove(true);
			return;
		}

		if (MovementComponent->MovementMode != MOVE_Falling)
		{
			MovementComponent->SetMovementMode(MOVE_Falling);
		}

		NewVelocity.Z = FMath::Min(NewVelocity.Z, -LightningSwordAirMoveDiveSpeed);
		break;
	}
	default:
		break;
	}

	MovementComponent->Velocity = NewVelocity;
}

void AJunBoss::StopLightningSwordAirMove(bool bClearVelocity)
{
	if (!bLightningSwordAirMoveActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	bLightningSwordAirMoveActive = false;
	LightningSwordAirMoveMode = EBossLightningSwordAirMoveMode::LiftAndHover;
	LightningSwordAirMoveTargetZ = 0.f;

	if (MovementComponent)
	{
		MovementComponent->GravityScale = LightningSwordAirMovePreviousGravityScale;

		if (MovementComponent->IsMovingOnGround())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
		else
		{
			MovementComponent->SetMovementMode(
				LightningSwordAirMovePreviousMovementMode,
				LightningSwordAirMovePreviousCustomMovementMode);
		}

		if (bClearVelocity)
		{
			FVector NewVelocity = MovementComponent->Velocity;
			if (bLightningSwordAirMoveLockHorizontalVelocity)
			{
				NewVelocity.X = 0.f;
				NewVelocity.Y = 0.f;
			}
			NewVelocity.Z = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}

	LightningSwordAirMovePreviousMovementMode = MOVE_Walking;
	LightningSwordAirMovePreviousCustomMovementMode = 0;
	LightningSwordAirMovePreviousGravityScale = 1.f;
}

void AJunBoss::ApplyAttackGroundMotionOverride(float Duration)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || bAttackGroundMotionOverrideActive)
	{
		return;
	}

	DefaultGroundFriction = MovementComponent->GroundFriction;
	DefaultBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
	DefaultBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;

	MovementComponent->GroundFriction = 0.f;
	MovementComponent->BrakingDecelerationWalking = 0.f;
	MovementComponent->BrakingFrictionFactor = 0.f;
	bAttackGroundMotionOverrideActive = true;
	AttackGroundMotionOverrideRemainTime = Duration;
}

void AJunBoss::RestoreAttackGroundMotionOverride()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !bAttackGroundMotionOverrideActive)
	{
		return;
	}

	MovementComponent->GroundFriction = DefaultGroundFriction;
	MovementComponent->BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	MovementComponent->BrakingFrictionFactor = DefaultBrakingFrictionFactor;
	bAttackGroundMotionOverrideActive = false;
	AttackGroundMotionOverrideRemainTime = 0.f;
}

