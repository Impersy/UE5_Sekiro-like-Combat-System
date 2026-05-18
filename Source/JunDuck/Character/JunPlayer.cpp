#include "Character/JunPlayer.h"
#include "AlphaBlend.h"
#include "Animation/AnimMontage.h"
#include "Camera/JunSpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/JunMonster.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Equipment/HatActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/JunPlayerController.h"
#include "Weapon/WeaponActor.h"
#include "JunGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "System/JunBGMManager.h"
#include "UObject/UnrealType.h"

namespace
{
	bool DoesMontageUseRootMotion(const UAnimMontage* Montage)
	{
		return Montage && Montage->HasRootMotion();
	}
}

AJunPlayer::AJunPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetupMeshAndCollision();
	SetupCameraComponents();
	SetupMovementDefaults();

	FakeDeathAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FakeDeathAudioComponent"));
	FakeDeathAudioComponent->SetupAttachment(RootComponent);
	FakeDeathAudioComponent->bAutoActivate = false;

	RealDeathAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("RealDeathAudioComponent"));
	RealDeathAudioComponent->SetupAttachment(RootComponent);
	RealDeathAudioComponent->bAutoActivate = false;
}

void AJunPlayer::SetupMeshAndCollision()
{
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("PlayerCapsule"));
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
}

void AJunPlayer::SetupCameraComponents()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CameraAnchor"));
	CameraAnchor->SetupAttachment(GetCapsuleComponent());
	CameraAnchor->SetUsingAbsoluteLocation(true);
	CameraAnchor->SetRelativeLocation(FVector::ZeroVector);

	SpringArm = CreateDefaultSubobject<UJunSpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(CameraAnchor);
	SpringArm->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 60.f), FRotator(-20.0f, 0, 0));
	SpringArm->TargetArmLength = DefaultSpringArmLength;
	SpringArm->bUsePawnControlRotation = true;

	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;

	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = false;
	SpringArm->bDoCollisionTest = true;
	SpringArm->CameraLagSpeed = 15.f;
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetFieldOfView(FreeCameraFOV);
}

void AJunPlayer::SetupMovementDefaults()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->RotationRate = FRotator(0.f, 700.f, 0.f);
	MovementComponent->JumpZVelocity = 900.f;
	MovementComponent->GravityScale = 2.4f;
	MovementComponent->AirControl = 0.25f;
	TargetMaxWalkSpeed = FreeRunMoveSpeed;
	MovementComponent->MaxWalkSpeed = FreeRunMoveSpeed;
}

void AJunPlayer::BeginPlay()
{
	Super::BeginPlay();

	SetTeamTag(JunGameplayTags::Team_Player);
	CurrentReviveCount = FMath::Max(0, MaxReviveCount);
	SpawnAndAttachWeapon();
	SpawnAndAttachScabbard();
	SpawnAndAttachDefaultHat();
	GetPlayerAnimInstance();
	CacheDefenseEffectComponents();
	CacheDefaultMovementBrakingSettings();
	ResetCameraToSpawnView(GetActorRotation());
	StartCameraHardSnap();

	if (CameraAnchor)
	{
		CameraAnchor->SetWorldLocation(GetCapsuleComponent()->GetComponentLocation());
	}
}

void AJunPlayer::CacheDefenseEffectComponents()
{
	CachedParryParticleComponent = FindNiagaraComponentByName(ParryParticleComponentName);
	CachedDeflectParticleComponent = FindNiagaraComponentByName(DeflectParticleComponentName);
	CachedGuardBreakParticleComponent = FindNiagaraComponentByName(GuardBreakParticleComponentName);
}

UNiagaraComponent* AJunPlayer::FindNiagaraComponentByName(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent && NiagaraComponent->GetFName() == ComponentName)
		{
			return NiagaraComponent;
		}
	}

	return nullptr;
}

void AJunPlayer::PlayDefenseEffect(TObjectPtr<UNiagaraComponent>& CachedComponent, FName ComponentName)
{
	if (!CachedComponent)
	{
		CachedComponent = FindNiagaraComponentByName(ComponentName);
	}

	if (CachedComponent)
	{
		CachedComponent->Activate(true);
		if (bAutoDeactivateDefenseParticles && GetWorld())
		{
			TWeakObjectPtr<UNiagaraComponent> WeakComponent = CachedComponent;
			FTimerHandle DeactivateTimerHandle;
			GetWorldTimerManager().SetTimer(
				DeactivateTimerHandle,
				[WeakComponent]()
				{
					if (UNiagaraComponent* NiagaraComponent = WeakComponent.Get())
					{
						NiagaraComponent->Deactivate();
					}
				},
				DefenseParticleAutoDeactivateDelay,
				false
			);
		}
	}
}

void AJunPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TryInitializeCameraRotationFromController();
	UpdateCameraHardSnap();
	UpdateCameraAnchor(DeltaTime);
	UpdateJunCamera(DeltaTime);

	if (DeathState != EJunPlayerDeathState::Alive)
	{
		UpdateDeathState(DeltaTime);
		return;
	}

	ValidateLockOnTarget();

	UpdateCharacterRotationForCurrentCameraMode(DeltaTime);

	UpdateMovementSpeed(DeltaTime);

	ApplyRunningStateToAnimInstance(ShouldUseRunningAnim());

	UpdateBasicAttackJumpAndMoveCancelState(DeltaTime);

	UpdateBasicAttackRecoveryBuffer(DeltaTime);
	UpdateHeavyAttackInput(DeltaTime);
	UpdateJumpAttackState(DeltaTime);
	if (bIsDodgeAttacking)
	{
		DodgeAttackElapsedTime += DeltaTime * GetCurrentDodgeAttackTimelineRate();
	}

	UpdateDodgeState(DeltaTime);

	UpdateDefenseInput(DeltaTime);

	UpdateGuardBreakVulnerability(DeltaTime);
	UpdatePlayerHitState(DeltaTime);
	UpdatePostureRecovery(DeltaTime);

	UpdateJumpStartAnimTrigger(DeltaTime);
}

void AJunPlayer::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	if (Damage <= 0 || DeathState != EJunPlayerDeathState::Alive)
	{
		return;
	}

	Hp = FMath::Clamp(Hp - Damage, 0, MaxHp);
	if (Hp > 0)
	{
		return;
	}

	if (CurrentReviveCount > 0)
	{
		EnterFakeDeath();
		return;
	}

	EnterRealDeath();
}

void AJunPlayer::UpdateCameraAnchor(float DeltaTime)
{
	if (!CameraAnchor || !GetCapsuleComponent())
	{
		return;
	}

	const FVector TargetAnchorLocation = GetCapsuleComponent()->GetComponentLocation();
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
		DefenseState == EJunDefenseState::None &&
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
		DefenseState == EJunDefenseState::None &&
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

	ResetCameraToSpawnView(GetActorRotation());
	StartCameraHardSnap();
}

void AJunPlayer::ResetCameraToSpawnView(const FRotator& SpawnRotation)
{
	if (!Controller)
	{
		bCameraRotationInitialized = false;
		return;
	}

	TargetCameraYaw = SpawnRotation.Yaw;
	TargetCameraPitch = FMath::Clamp(SpawnCameraPitch, MinCameraPitch, MaxCameraPitch);
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

	TargetMaxWalkSpeed = GetDesiredMaxWalkSpeed();
	MovementComponent->MaxWalkSpeed = FMath::FInterpTo(
		MovementComponent->MaxWalkSpeed,
		TargetMaxWalkSpeed,
		DeltaTime,
		MoveSpeedInterpRate
	);
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
	if (!CanStartLockOnTurn())
	{
		return;
	}

	UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance();
	if (!PlayerAnimInstance)
	{
		return;
	}

	const float YawDelta = GetLockOnTargetYawDelta();
	UAnimMontage* TurnMontage = ChooseLockOnTurnMontage(YawDelta);
	if (!TurnMontage)
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

	if (DefenseState != EJunDefenseState::None)
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

bool AJunPlayer::IsJumpAttacking() const
{
	return bIsJumpAttacking;
}

bool AJunPlayer::IsDodgeAttacking() const
{
	return bIsDodgeAttacking;
}

bool AJunPlayer::IsExecuting() const
{
	return bIsExecuting;
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
	return bLockOnActive ? LockOnRunMoveSpeed : FreeRunMoveSpeed;
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

void AJunPlayer::Jump()
{
	if (bIsDodgeAttacking)
	{
		return;
	}

	if (bIsHeavyAttacking)
	{
		if (!CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Jump))
		{
			BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction::Jump);
			return;
		}

		CancelHeavyAttackForRecoveryTransition(HeavyAttackJumpCancelBlendOutTime);
	}

	if (!CanJump())
	{
		return;
	}

	CancelLockOnTurn();
	JumpStartAnimTriggerRemainTime = JumpStartAnimTriggerDuration;

	const FVector2D MoveInput(DesiredMoveForward, DesiredMoveRight);
	LaunchCharacter(BuildJumpLaunchVelocity(MoveInput), true, true);
}

void AJunPlayer::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (bIsJumpAttacking)
	{
		RequestJumpAttackEnd();
		return;
	}
}

void AJunPlayer::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	Super::HandleGameplayEventNotify(EventTag);

	if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_BasicAttack_ComboWindow))
	{
		OnBasicAttackComboWindowBegin();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_HeavyAttack_ComboWindow))
	{
		OnHeavyAttackComboWindowBegin();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Dodge_Start))
	{
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Defense_ParryWindowBegin))
	{
		// Code-driven parry timing.
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Defense_ParryWindowEnd))
	{
		// Code-driven parry timing.
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Defense_EndBaseRelease))
	{
		// GuardEnd base release is now code-driven.
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Execution_Apply))
	{
		AdvanceExecutionCameraStage();
		if (bCurrentExecutionIsFinish && bLockOnActive)
		{
			EndLockOn();
			if (bIsExecuting && ExecutionCameraTarget)
			{
				CameraMode = EJunCameraMode::Execution;
			}
		}
		if (bIsExecuting && CurrentExecutionTarget)
		{
			CurrentExecutionTarget->TriggerPendingExecutionMontage(!bCurrentExecutionIsFinish);
		}
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Execution_FinishCameraPullIn))
	{
		AdvanceExecutionFinishCameraPullInStage();
		if (bCurrentExecutionIsFinish && bIsExecuting && CurrentExecutionTarget)
		{
			CurrentExecutionTarget->ApplyPendingExecutionResult();
		}
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Execution_CameraRestore))
	{
		EndExecutionCamera();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Death_Present))
	{
		TriggerPendingDeathPresentation();
	}
}

void AJunPlayer::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, EJunWeaponNiagaraComponent NiagaraComponent)
{
	if (!EquippedWeapon)
	{
		return;
	}

	EquippedWeapon->SetAttackHitReactType(HitReactType);
	EquippedWeapon->SetAttackDamageData(DamageData);
	EquippedWeapon->SetAttackDefenseKnockbackData(DefenseKnockbackData);
	EquippedWeapon->StartAttackTrace(NiagaraComponent);
}

void AJunPlayer::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
	if (!EquippedWeapon)
	{
		return;
	}

	EquippedWeapon->EndAttackTrace(NiagaraComponent);
}

void AJunPlayer::BeginWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->ActivateWeaponNiagara(ComponentType);
	}
}

void AJunPlayer::EndWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->DeactivateWeaponNiagara(ComponentType);
	}
}

void AJunPlayer::BasicAttack()
{
	if (bIsDodgeAttacking)
	{
		TryCancelDodgeAttackIntoBasicAttack();
		return;
	}

	if (bIsHeavyAttacking)
	{
		TryCancelHeavyAttackIntoBasicAttack();
		return;
	}

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		StartJumpAttack();
		return;
	}

	if (!BasicAttackMontage || !AnimInstance)
	{
		return;
	}

	if (!bIsBasicAttacking)
	{
		CancelLockOnTurn();
		StartBasicAttack();
		return;
	}

	if (bBasicAttackComboAdvanceStateActive)
	{
		bBufferedBasicAttackComboInput = true;
		TryAdvanceBasicAttackCombo();
		return;
	}

	if (bCanBufferBasicAttackComboInput)
	{
		bBufferedBasicAttackComboInput = true;
		return;
	}

	if (bCanRestartBasicAttackAfterComboAdvance)
	{
		CancelBasicAttackForRecoveryTransition(BasicAttackRestartBlendOutTime);
		StartBasicAttack();
		return;
	}

	bBufferedBasicAttackInput = true;
}

void AJunPlayer::OnHeavyAttackStarted()
{
	if (bIsDodgeAttacking)
	{
		if (!TryCancelDodgeAttackIntoHeavyAttack())
		{
			return;
		}

		StartHeavyAttackTap();
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (!TryCancelDodgeIntoHeavyAttack())
		{
			return;
		}
	}

	if (bIsJumpAttacking)
	{
		if (!TryCancelJumpAttackEndIntoHeavyAttack())
		{
			return;
		}

		StartHeavyAttackTap();
		return;
	}

	if (bIsBasicAttacking)
	{
		if (!TryCancelBasicAttackIntoHeavyAttack())
		{
			return;
		}

		StartHeavyAttackTap();
		return;
	}

	if (bIsHeavyAttacking)
	{
		if (HeavyAttackState == EJunHeavyAttackState::Tap)
		{
			if (bHeavyAttackComboAdvanceStateActive)
			{
				bBufferedHeavyAttackComboInput = true;
				TryAdvanceHeavyAttackCombo();
				return;
			}

			if (bCanBufferHeavyAttackComboInput)
			{
				bBufferedHeavyAttackComboInput = true;
				return;
			}

			if (bCanRestartHeavyAttackAfterComboAdvance)
			{
				CancelHeavyAttackForRecoveryTransition(HeavyAttackRestartBlendOutTime);
				StartHeavyAttackTap();
				return;
			}

			bBufferedHeavyAttackInput = true;
			return;
		}

		if (TryCancelHeavyAttackIntoHeavyAttack())
		{
			StartHeavyAttackTap();
		}
		return;
	}

	if (bHeavyAttackInputHeld)
	{
		return;
	}

	bHeavyAttackInputHeld = true;
	HeavyAttackInputHoldTime = 0.f;
	HeavyAttackChargeLoopElapsedTime = 0.f;
}

void AJunPlayer::OnHeavyAttackReleased()
{
	if (!bHeavyAttackInputHeld && !bIsHeavyAttacking)
	{
		return;
	}

	bHeavyAttackInputHeld = false;

	if (bIsHeavyAttacking)
	{
		if (HeavyAttackState == EJunHeavyAttackState::ChargeStart)
		{
			bHeavyAttackChargeEndRequested = true;
		}
		else if (HeavyAttackState == EJunHeavyAttackState::ChargeLoop)
		{
			StartHeavyAttackChargeEnd();
		}
		return;
	}

	if (!CanStartHeavyAttack())
	{
		ResetHeavyAttackChargeInput();
		return;
	}

	if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
	{
		StartHeavyAttackChargeEnd();
		return;
	}

	StartHeavyAttackTap();
}

void AJunPlayer::StartDodge()
{
	StartDodgeInternal(false);
}

void AJunPlayer::StartDodgeInternal(bool bIgnoreDodgeBlockAndReleaseGate)
{
	if (bIsDodgeAttacking)
	{
		TryCancelDodgeAttackIntoDodge();
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (bIsHeavyAttacking)
	{
		if (!CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Dodge))
		{
			BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction::Dodge);
			return;
		}

		CancelHeavyAttackForRecoveryTransition(HeavyAttackDodgeCancelBlendOutTime);
	}

	CancelLockOnTurn();

	// 援щⅤ湲곕뒗 ?낅젰 異⑸룎????븘???곹깭 ?쒓렇瑜?癒쇱? ?좉렐 ??紐쏀?二쇰? ?쒖옉?쒕떎.
	// ?먯쑀 移대찓?쇰뒗 ?낅젰 諛⑺뼢?쇰줈 罹먮┃?곕? 癒쇱? 留욎텛怨?
	// ?쎌삩? StartDodge ?덉뿉???좏깮??諛⑺뼢蹂?紐쏀?二쇨? ?뚰뵾 諛⑺뼢??梨낆엫吏꾨떎.
	UAnimMontage* DodgeMontageToPlay = GetDodgeMontageToPlay();
	if (!DodgeMontageToPlay)
	{
		return;
	}

	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (bIgnoreDodgeBlockAndReleaseGate)
		{
			return;
		}

		TryStartDodgeChain();
		return;
	}

	if (!bIgnoreDodgeBlockAndReleaseGate &&
		(HasGameplayTag(JunGameplayTags::State_Block_Dodge) || !bDodgeInputReleasedSinceLastDodge))
	{
		return;
	}

	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (!bLockOnActive)
	{
		AlignActorToDesiredMoveDirectionForDodge();
	}

	AddGameplayTag(JunGameplayTags::State_Action_Dodge);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	DodgeElapsedTime = 0.f;
	bDodgeInputReleasedSinceLastDodge = false;
	if (DodgeMontageToPlay == BackDashMontage)
	{
		DodgeInternalCooldownRemainTime = BackDashInternalCooldownDuration;
		DodgeFinishRemainTime = BackDashFinishTime;
		DodgeInvincibleRemainTime = BackDashInvincibleDuration;
	}
	else if (DodgeMontageToPlay == LockOnDodgeBackMontage)
	{
		DodgeInternalCooldownRemainTime = BackwardDodgeInternalCooldownDuration;
		DodgeFinishRemainTime = BackwardDodgeFinishTime;
		DodgeInvincibleRemainTime = BackwardDodgeInvincibleDuration;
	}
	else if (DodgeMontageToPlay == LockOnDodgeLeftMontage || DodgeMontageToPlay == LockOnDodgeRightMontage)
	{
		DodgeInternalCooldownRemainTime = SideDodgeInternalCooldownDuration;
		DodgeFinishRemainTime = SideDodgeFinishTime;
		DodgeInvincibleRemainTime = SideDodgeInvincibleDuration;
	}
	else
	{
		DodgeInternalCooldownRemainTime = ForwardDodgeInternalCooldownDuration;
		DodgeFinishRemainTime = ForwardDodgeFinishTime;
		DodgeInvincibleRemainTime = ForwardDodgeInvincibleDuration;
	}

	if (DodgeInvincibleRemainTime > 0.f)
	{
		AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnDodgeMontageEnded);

	CurrentDodgeMontage = DodgeMontageToPlay;
	CurrentDodgePlayRate = GetDodgePlayRateForMontage(DodgeMontageToPlay);
	AnimInstance->Montage_Play(DodgeMontageToPlay, CurrentDodgePlayRate);
}

void AJunPlayer::OnDefenseStarted()
{
	if (bIsDodgeAttacking)
	{
		TryCancelDodgeAttackIntoDefense();
		return;
	}

	if (bIsJumpAttacking && JumpAttackState == EJunJumpAttackState::End)
	{
		if (!TryCancelJumpAttackEndIntoDefense())
		{
			return;
		}
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		if (ChainParryWindowRemainTime > 0.f)
		{
			bDefenseButtonHeld = true;
			bHoldRequestedForCurrentDeflect = true;
			bParryWindowOpen = true;
			ParryWindowRemainTime = DefaultParryWindowDuration;
			return;
		}

		if (ParrySuccessElapsedTime >= ParrySuccessDefenseCancelOpenTime)
		{
			bDefenseButtonHeld = true;
			bHoldRequestedForCurrentDeflect = true;
			CancelParrySuccessForCancelTransition(ParrySuccessDefenseCancelBlendOutTime);
			StartDefenseSequence();
		}
		return;
	}

	if (bIsHeavyAttacking)
	{
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = true;
		BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction::Defense);
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	CancelLockOnTurn();

	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (!TryCancelDodgeIntoDefense())
		{
			bDefenseButtonHeld = true;
		}
		return;
	}

	// Defense is split into:
	// None -> Starting(deflect attempt) -> Guarding(block) -> Ending -> None.
	// Repeated taps restart the deflect attempt immediately, while hold transitions into Guarding
	// only after GuardStart finishes and the button is still held.
	bDefenseButtonHeld = true;
	bHoldRequestedForCurrentDeflect = true;
	if (bIsBasicAttacking || CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Defense))
	{
		if (CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Defense))
		{
			BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction::Defense);
		}
		return;
	}

	if (DefenseState == EJunDefenseState::Guarding)
	{
		return;
	}

	if (DefenseState == EJunDefenseState::Starting || DefenseState == EJunDefenseState::Ending)
	{
		StartDefenseSequence();
		return;
	}

	StartDefenseSequence();
}

void AJunPlayer::OnDefenseReleased()
{
	// Release only changes intent during the deflect attempt.
	// BlockGuard transitions into Ending immediately on release.
	bDefenseButtonHeld = false;
	bHoldRequestedForCurrentDeflect = false;

	if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		if (AnimInstance && GuardBlockMontage)
		{
			AnimInstance->Montage_Stop(GuardBlockReleaseBlendOutTime, GuardBlockMontage);
		}

		CurrentHitState = EJunPlayerHitState::None;
		PlayerHitStateRemainTime = 0.f;
		CurrentHitReactType = EHitReactType::None;
		CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;

		if (GetCharacterMovement())
		{
			RestoreDefaultMovementBrakingSettings();
		}

		RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
		RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
		RemoveGameplayTag(JunGameplayTags::State_Block_Move);
		RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
		RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
		RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

		GetWorldTimerManager().ClearTimer(GuardBlockReleaseIntoGuardEndTimerHandle);
		GetWorldTimerManager().SetTimer(
			GuardBlockReleaseIntoGuardEndTimerHandle,
			this,
			&AJunPlayer::CompleteGuardBlockReleaseIntoGuardEnd,
			GuardBlockReleaseBlendOutTime,
			false
		);
		return;
	}

	if (DefenseState == EJunDefenseState::Guarding)
	{
		BeginGuardEnd();
	}
}

void AJunPlayer::AddCameraLookInput(const FVector2D& Input)
{
	PendingCameraLookInput = Input;
}

void AJunPlayer::ToggleLockOn()
{
	if (bLockOnActive)
	{
		EndLockOn();
		return;
	}

	AJunCharacter* NewTarget = FindBestLockOnTarget();
	if (!NewTarget)
	{
		return;
	}

	StartLockOn(NewTarget);
}

UAnimInstance* AJunPlayer::GetPlayerAnimInstance()
{
	if (!AnimInstance)
	{
		AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	}

	return AnimInstance;
}

void AJunPlayer::SpawnAndAttachWeapon()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (DefaultWeaponClass)
	{
		EquippedWeapon = World->SpawnActor<AWeaponActor>(
			DefaultWeaponClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn weapon."));
		}
		else
		{
			EquippedWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				WeaponSocketName
			);
		}
	}
}

void AJunPlayer::SpawnAndAttachScabbard()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (DefaultScabbardClass)
	{
		EquippedScabbard = World->SpawnActor<AWeaponActor>(
			DefaultScabbardClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedScabbard)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn scabbard."));
		}
		else
		{
			EquippedScabbard->StopAttackTrace();
			EquippedScabbard->SetActorTickEnabled(false);
			EquippedScabbard->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				ScabbardSocketName
			);
		}
	}
}

void AJunPlayer::SpawnAndAttachDefaultHat()
{
	if (DefaultHatClass)
	{
		EquipHat(DefaultHatClass);
	}
}

void AJunPlayer::EquipHat(TSubclassOf<AHatActor> NewHatClass)
{
	UnequipHat();

	UWorld* World = GetWorld();
	if (!World || !NewHatClass || !GetMesh() || HatSocketName.IsNone())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedHat = World->SpawnActor<AHatActor>(
		NewHatClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!EquippedHat)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn hat."));
		return;
	}

	EquippedHat->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		HatSocketName
	);
}

void AJunPlayer::UnequipHat()
{
	if (EquippedHat)
	{
		EquippedHat->Destroy();
		EquippedHat = nullptr;
	}
}

bool AJunPlayer::GetPlayerIsFalling()
{
	return GetMovementComponent()->IsFalling();
}

bool AJunPlayer::IsLockOn()
{
	return bLockOnActive;
}

FVector AJunPlayer::GetLockOnMarkerWorldPoint()
{
	return GetRawLockOnBonePoint();
}

bool AJunPlayer::IsGuardOn()
{
	return DefenseState == EJunDefenseState::Guarding;
}

bool AJunPlayer::IsGuardPoseActive()
{
	return DefenseState != EJunDefenseState::None;
}

bool AJunPlayer::IsBasicAttacking() const
{
	return bIsBasicAttacking;
}

bool AJunPlayer::IsWalking() const
{
	return GetMoveState() != EJunMoveState::Run;
}

bool AJunPlayer::IsSprinting() const
{
	return false;
}

bool AJunPlayer::IsInParrySuccess() const
{
	return CurrentHitState == EJunPlayerHitState::ParrySuccess;
}

bool AJunPlayer::IsJumpStartAnimTriggered() const
{
	return JumpStartAnimTriggerRemainTime > 0.f;
}

EJunMoveState AJunPlayer::GetMoveState() const
{
	if (DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Guarding)
	{
		return EJunMoveState::Guard;
	}

	const bool bHasMoveInput =
		!FMath::IsNearlyZero(DesiredMoveForward) ||
		!FMath::IsNearlyZero(DesiredMoveRight) ||
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);

	if (bWalkRequested && bHasMoveInput)
	{
		return EJunMoveState::Walk;
	}

	return EJunMoveState::Run;
}

bool AJunPlayer::CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction Action) const
{
	if (!bIsBasicAttacking)
	{
		switch (Action)
		{
		case EJunBufferedRecoveryAction::Dodge:
		case EJunBufferedRecoveryAction::Jump:
			return PostBasicAttackJumpDodgeBufferRemainTime > 0.f;
		case EJunBufferedRecoveryAction::Defense:
			return PostBasicAttackDefenseBufferRemainTime > 0.f;
		default:
			return false;
		}
	}

	const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
	const float DodgeCancelOpenTime = BasicAttackDodgeCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackDodgeCancelOpenTimes[ComboArrayIndex]
		: 0.18f;
	const float JumpCancelOpenTime = BasicAttackJumpCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackJumpCancelOpenTimes[ComboArrayIndex]
		: DodgeCancelOpenTime;
	const float DefenseCancelOpenTime = BasicAttackDefenseCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackDefenseCancelOpenTimes[ComboArrayIndex]
		: JumpCancelOpenTime;

	switch (Action)
	{
	case EJunBufferedRecoveryAction::Dodge:
		return BasicAttackSectionElapsedTime >= DodgeCancelOpenTime;
	case EJunBufferedRecoveryAction::Jump:
		return BasicAttackSectionElapsedTime >= JumpCancelOpenTime;
	case EJunBufferedRecoveryAction::Defense:
		return BasicAttackSectionElapsedTime >= DefenseCancelOpenTime;
	default:
		return false;
	}
}

bool AJunPlayer::CanCancelBasicAttackIntoHeavyAttack() const
{
	if (!bIsBasicAttacking)
	{
		return false;
	}

	const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
	const float HeavyAttackCancelOpenTime = BasicAttackHeavyAttackCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackHeavyAttackCancelOpenTimes[ComboArrayIndex]
		: 0.18f;

	return BasicAttackSectionElapsedTime >= HeavyAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelBasicAttackIntoHeavyAttack()
{
	if (!CanCancelBasicAttackIntoHeavyAttack())
	{
		return false;
	}

	CancelBasicAttackForRecoveryTransition(BasicAttackHeavyAttackCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanBufferDefenseTransitionCancel() const
{
	return DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Ending ||
		PostDefenseTransitionCancelBufferRemainTime > 0.f;
}

bool AJunPlayer::CanCancelDefenseTransitionIntoMove() const
{
	return DefenseState == EJunDefenseState::Ending &&
		DefenseTransitionElapsedTime >= DefenseMoveCancelOpenTime;
}

bool AJunPlayer::CanBufferParrySuccessCancel(EJunBufferedParrySuccessCancelAction Action) const
{
	if (CurrentHitState != EJunPlayerHitState::ParrySuccess)
	{
		return false;
	}

	switch (Action)
	{
	case EJunBufferedParrySuccessCancelAction::BasicAttack:
		return ParrySuccessElapsedTime >= ParrySuccessBasicAttackCancelOpenTime;
	case EJunBufferedParrySuccessCancelAction::HeavyAttack:
		return ParrySuccessElapsedTime >= ParrySuccessHeavyAttackCancelOpenTime;
	case EJunBufferedParrySuccessCancelAction::Jump:
		return ParrySuccessElapsedTime >= ParrySuccessJumpCancelOpenTime;
	case EJunBufferedParrySuccessCancelAction::Dodge:
		return ParrySuccessElapsedTime >= ParrySuccessDodgeCancelOpenTime;
	case EJunBufferedParrySuccessCancelAction::Move:
		return ParrySuccessElapsedTime >= ParrySuccessMoveCancelOpenTime;
	default:
		return false;
	}
}

bool AJunPlayer::ShouldUseGuardBase() const
{
	return DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Guarding ||
		(DefenseState == EJunDefenseState::Ending && bKeepGuardBaseWhileEnding);
}

EJunDefenseState AJunPlayer::GetDefenseState() const
{
	return DefenseState;
}

bool AJunPlayer::IsParryWindowOpen()
{
	return bParryWindowOpen;
}

float AJunPlayer::GetDesiredMoveForward() const
{
	return DesiredMoveForward;
}

float AJunPlayer::GetDesiredMoveRight() const
{
	return DesiredMoveRight;
}

float AJunPlayer::GetGuardMoveBlendRemainTime() const
{
	return GuardMoveBlendRemainTime;
}

bool AJunPlayer::ShouldDeferGuardMoveInput() const
{
	return bDeferGuardMoveInput;
}

float AJunPlayer::GetDesiredMaxWalkSpeed() const
{
	switch (GetMoveState())
	{
	case EJunMoveState::Walk:
		return WalkMoveSpeed;
	case EJunMoveState::Guard:
		return GuardMoveSpeed;
	case EJunMoveState::Sprint:
		return SprintMoveSpeed;
	case EJunMoveState::Run:
	default:
		return GetCurrentRunMoveSpeed();
	}
}

bool AJunPlayer::ShouldUseRunningAnim() const
{
	if (bWalkRequested)
	{
		return false;
	}

	return GetMoveState() == EJunMoveState::Run;
}

void AJunPlayer::ToggleWalkingState()
{
	SetWalkRequested(!bWalkRequested);
}

void AJunPlayer::SetWalkRequested(bool bNewWalkRequested)
{
	bWalkRequested = bNewWalkRequested;
	TargetMaxWalkSpeed = GetDesiredMaxWalkSpeed();
	ApplyRunningStateToAnimInstance(ShouldUseRunningAnim());
}

void AJunPlayer::SetDesiredMoveAxes(float NewForward, float NewRight)
{
	PendingMoveForward = FMath::Clamp(NewForward, -1.f, 1.f);
	PendingMoveRight = FMath::Clamp(NewRight, -1.f, 1.f);
	const bool bHasMoveInput =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);

	if (bHasMoveInput && IsLockOnTurnPlaying())
	{
		CancelLockOnTurn();
	}

	if (bIsJumpAttacking && JumpAttackState == EJunJumpAttackState::End)
	{
		if (bHasMoveInput && !TryCancelJumpAttackEndIntoMove())
		{
			return;
		}
	}

	if (bIsDodgeAttacking)
	{
		if (bHasMoveInput && !TryCancelDodgeAttackIntoMove())
		{
			return;
		}
	}

	if (bHasMoveInput &&
		bIsHeavyAttacking &&
		TryCancelHeavyAttackIntoMove())
	{
		DesiredMoveForward = PendingMoveForward;
		DesiredMoveRight = PendingMoveRight;
		return;
	}

	if (bDeferGuardMoveInput)
	{
		return;
	}

	if (bHasMoveInput && TryCancelHitReactIntoMove())
	{
		DesiredMoveForward = PendingMoveForward;
		DesiredMoveRight = PendingMoveRight;
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Block_Move))
	{
		DesiredMoveForward = 0.f;
		DesiredMoveRight = 0.f;
		return;
	}

	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;
}

void AJunPlayer::BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction Action)
{
	if (!CanCancelBasicAttackIntoRecoveryAction(Action))
	{
		return;
	}

	BufferedBasicAttackRecoveryAction = Action;
	TryExecuteBufferedBasicAttackRecoveryAction();
}

void AJunPlayer::BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction Action)
{
	const bool bCanBufferFromParryState =
		DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Ending ||
		PostDefenseTransitionCancelBufferRemainTime > 0.f;

	if (!bCanBufferFromParryState)
	{
		return;
	}

	BufferedDefenseTransitionCancelAction = Action;
	TryExecuteBufferedDefenseTransitionCancelAction();
}

void AJunPlayer::BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction Action)
{
	if (CurrentHitState != EJunPlayerHitState::ParrySuccess)
	{
		return;
	}

	BufferedParrySuccessCancelAction = Action;
	TryExecuteBufferedParrySuccessCancelAction();
}

void AJunPlayer::ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (Is_Dead() || bIsExecuting || IsInDeathSequence())
	{
		return;
	}

	LastIncomingSwingDirection = SwingDirection;
	LastIncomingKnockbackDirection = DetermineKnockbackDirectionFromDamageCauser(DamageCauser);
	LastIncomingDefenseKnockbackData = DefenseKnockbackData;

	const EJunPlayerHitResolveResult ResolveResult = ResolveIncomingHitResult(HitType);
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);

	switch (ResolveResult)
	{
	case EJunPlayerHitResolveResult::Ignored:
		return;
	case EJunPlayerHitResolveResult::PerfectParrySuccess:
		SetPendingDefenseSoundType(EJunDefenseSoundType::PerfectParry, bPlayDefenseSoundImmediately);
		AddPosture(PerfectParryPostureGain);
		if (bCanBuildAttackerPostureOnParry)
		{
			if (AJunMonster* AttackingMonster = Cast<AJunMonster>(DamageCauser))
			{
				AttackingMonster->NotifyAttackParriedBy(this, 1.f);
			}
		}
		StartParrySuccess();
		return;
	case EJunPlayerHitResolveResult::NormalParrySuccess:
		SetPendingDefenseSoundType(EJunDefenseSoundType::NormalParry, bPlayDefenseSoundImmediately);
		if (IsPostureFull())
		{
			StartGuardBreak();
			return;
		}
		if (bCanBuildAttackerPostureOnParry)
		{
			if (AJunMonster* AttackingMonster = Cast<AJunMonster>(DamageCauser))
			{
				AttackingMonster->NotifyAttackParriedBy(this, 0.5f);
			}
		}
		AddPosture(NormalParryPostureGain);
		StartParrySuccess();
		return;
	case EJunPlayerHitResolveResult::GuardBlock:
		SetPendingDefenseSoundType(EJunDefenseSoundType::GuardHit, bPlayDefenseSoundImmediately);
		if (IsPostureFull())
		{
			StartGuardBreak();
			return;
		}
		AddPosture(GuardBlockPostureGain);
		StartGuardBlock();
		return;
	case EJunPlayerHitResolveResult::HitReact:
	default:
		const float DamageMultiplier = GuardBreakVulnerableRemainTime > 0.f
			? GuardBreakDamageMultiplier
			: 1.f;
		OnDamaged(FMath::RoundToInt(DamageAmount * DamageMultiplier), AttackerCharacter);
		if (Is_Dead() || IsInDeathSequence())
		{
			return;
		}
		if (AddPosture(HitReactPostureGain))
		{
			StartGuardBreak();
			return;
		}
		StartHitReact(HitType, DetermineHitReactDirection(DamageCauser, SwingDirection));
		return;
	}
}

void AJunPlayer::OnBasicAttackComboWindowBegin()
{
	bCanBufferBasicAttackComboInput = true;

	if (bBufferedBasicAttackInput)
	{
		bBufferedBasicAttackComboInput = true;
		bBufferedBasicAttackInput = false;
	}
}

void AJunPlayer::OnAttackComboAdvanceStateBegin(EJunAttackComboType ComboType)
{
	if (ComboType == EJunAttackComboType::HeavyAttack)
	{
		OnHeavyAttackComboAdvanceStateBegin();
		return;
	}

	OnBasicAttackComboAdvanceStateBegin();
}

void AJunPlayer::OnAttackComboAdvanceStateEnd(EJunAttackComboType ComboType)
{
	if (ComboType == EJunAttackComboType::HeavyAttack)
	{
		OnHeavyAttackComboAdvanceStateEnd();
		return;
	}

	OnBasicAttackComboAdvanceStateEnd();
}

void AJunPlayer::OnBasicAttackComboAdvanceStateBegin()
{
	bBasicAttackComboAdvanceStateActive = true;
	bCanBufferBasicAttackComboInput = false;
	bCanRestartBasicAttackAfterComboAdvance = false;

	if (bBufferedBasicAttackComboInput)
	{
		TryAdvanceBasicAttackCombo();
	}
}

void AJunPlayer::OnBasicAttackComboAdvanceStateEnd()
{
	const bool bWasComboAdvanceStateActive = bBasicAttackComboAdvanceStateActive;
	bBasicAttackComboAdvanceStateActive = false;
	bBufferedBasicAttackComboInput = false;
	bCanRestartBasicAttackAfterComboAdvance = bWasComboAdvanceStateActive && bIsBasicAttacking;
}

void AJunPlayer::OnHeavyAttackComboWindowBegin()
{
	if (HeavyAttackState != EJunHeavyAttackState::Tap)
	{
		return;
	}

	bCanBufferHeavyAttackComboInput = true;

	if (bBufferedHeavyAttackInput)
	{
		bBufferedHeavyAttackComboInput = true;
		bBufferedHeavyAttackInput = false;
	}
}

void AJunPlayer::OnHeavyAttackComboAdvanceStateBegin()
{
	if (HeavyAttackState != EJunHeavyAttackState::Tap)
	{
		return;
	}

	bHeavyAttackComboAdvanceStateActive = true;
	bCanBufferHeavyAttackComboInput = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;

	if (bBufferedHeavyAttackComboInput)
	{
		TryAdvanceHeavyAttackCombo();
	}
}

void AJunPlayer::OnHeavyAttackComboAdvanceStateEnd()
{
	const bool bWasComboAdvanceStateActive = bHeavyAttackComboAdvanceStateActive;
	bHeavyAttackComboAdvanceStateActive = false;
	bBufferedHeavyAttackComboInput = false;
	bCanRestartHeavyAttackAfterComboAdvance =
		bWasComboAdvanceStateActive &&
		bIsHeavyAttacking &&
		HeavyAttackState == EJunHeavyAttackState::Tap;
}

void AJunPlayer::UpdateDodgeState(float DeltaTime)
{
	const float DodgeTimelineDeltaTime = DeltaTime * GetCurrentDodgeTimelineRate();

	if (DodgeInvincibleRemainTime > 0.f)
	{
		DodgeInvincibleRemainTime = FMath::Max(0.f, DodgeInvincibleRemainTime - DodgeTimelineDeltaTime);

		if (DodgeInvincibleRemainTime <= 0.f)
		{
			RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
		}
	}

	if (DodgeInternalCooldownRemainTime > 0.f)
	{
		DodgeInternalCooldownRemainTime = FMath::Max(0.f, DodgeInternalCooldownRemainTime - DodgeTimelineDeltaTime);

		if (DodgeInternalCooldownRemainTime <= 0.f && !HasGameplayTag(JunGameplayTags::State_Action_Dodge))
		{
			RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
			CurrentDodgePlayRate = 1.f;
		}
	}

	if (!HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return;
	}

	DodgeElapsedTime += DodgeTimelineDeltaTime;
	if (DodgeElapsedTime >= GetDodgeMoveCancelOpenTimeForMontage(CurrentDodgeMontage))
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	}

	if (DodgeFinishRemainTime <= 0.f)
	{
		return;
	}

	DodgeFinishRemainTime = FMath::Max(0.f, DodgeFinishRemainTime - DodgeTimelineDeltaTime);

	if (DodgeFinishRemainTime <= 0.f)
	{
		FinishDodgeState();
	}
}

void AJunPlayer::OnDodgeInputReleased()
{
	bDodgeInputReleasedSinceLastDodge = true;
}

bool AJunPlayer::TryStartDodgeChain()
{
	if (!bDodgeChainWindowActive ||
		!AnimInstance ||
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		!CurrentDodgeMontage)
	{
		return false;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
	AnimInstance->Montage_Stop(DodgeChainBlendOutTime, DodgeMontageToStop);

	FinishDodgeState();
	DodgeInternalCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	CurrentDodgePlayRate = 1.f;

	StartDodgeInternal(true);
	return HasGameplayTag(JunGameplayTags::State_Action_Dodge);
}

bool AJunPlayer::CanCancelDodgeIntoRecoveryAction() const
{
	return DodgeElapsedTime >= GetDodgeRecoveryCancelOpenTimeForMontage(CurrentDodgeMontage) &&
		HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		CurrentDodgeMontage != nullptr;
}

void AJunPlayer::CancelDodgeForRecoveryTransition(float BlendOutTime)
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	if (AnimInstance && DodgeMontageToStop)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, DodgeMontageToStop);
	}

	FinishDodgeState();
	DodgeInternalCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	CurrentDodgePlayRate = 1.f;
	DodgeElapsedTime = 0.f;
}

bool AJunPlayer::TryCancelDodgeIntoBasicAttack()
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(DodgeRecoveryCancelBlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoHeavyAttack()
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(DodgeRecoveryCancelBlendOutTime);
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoJump()
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(DodgeRecoveryCancelBlendOutTime);
	Jump();
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoDefense()
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(DodgeRecoveryCancelBlendOutTime);
	BeginDefenseFromBufferedInput();
	return true;
}

bool AJunPlayer::TryStartDodgeAttack()
{
	if (!AnimInstance ||
		!DodgeAttackMontage ||
		!bDodgeAttackWindowActive ||
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		bIsDodgeAttacking)
	{
		return false;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	if (DodgeMontageToStop)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
		AnimInstance->Montage_Stop(DodgeAttackBlendOutTime, DodgeMontageToStop);
	}

	FinishDodgeState();
	bDodgeAttackWindowActive = false;

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsDodgeAttacking = true;
	DodgeAttackElapsedTime = 0.f;
	CurrentDodgeAttackMontage = DodgeAttackMontage;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
	AnimInstance->Montage_Play(DodgeAttackMontage, FMath::Max(DodgeAttackPlayRate, KINDA_SMALL_NUMBER));
	return true;
}

void AJunPlayer::BeginDodgeAttackWindow()
{
	bDodgeAttackWindowActive = true;
}

void AJunPlayer::EndDodgeAttackWindow()
{
	bDodgeAttackWindowActive = false;
}

void AJunPlayer::BeginDodgeChainWindow()
{
	bDodgeChainWindowActive = true;
}

void AJunPlayer::EndDodgeChainWindow()
{
	bDodgeChainWindowActive = false;
}

void AJunPlayer::TryExecuteBufferedBasicAttackRecoveryAction()
{
	if (BufferedBasicAttackRecoveryAction == EJunBufferedRecoveryAction::None ||
		!CanCancelBasicAttackIntoRecoveryAction(BufferedBasicAttackRecoveryAction))
	{
		return;
	}

	const EJunBufferedRecoveryAction PendingAction = BufferedBasicAttackRecoveryAction;
	BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	const bool bShouldCancelCurrentAttack = bIsBasicAttacking && CanCancelBasicAttackIntoRecoveryAction(PendingAction);
	PostBasicAttackJumpDodgeBufferRemainTime = 0.f;
	PostBasicAttackDefenseBufferRemainTime = 0.f;

	switch (PendingAction)
	{
	case EJunBufferedRecoveryAction::Dodge:
		if (bShouldCancelCurrentAttack)
		{
			CancelBasicAttackForRecoveryTransition(BasicAttackDodgeCancelBlendOutTime);
		}
		StartDodge();
		break;
	case EJunBufferedRecoveryAction::Jump:
		if (bShouldCancelCurrentAttack)
		{
			CancelBasicAttackForRecoveryTransition(BasicAttackJumpCancelBlendOutTime);
		}
		Jump();
		break;
	case EJunBufferedRecoveryAction::Defense:
		if (bShouldCancelCurrentAttack)
		{
			CancelBasicAttackForRecoveryTransition(BasicAttackDefenseCancelBlendOutTime);
		}
		BeginDefenseFromBufferedInput();
		break;
	default:
		break;
	}
}

void AJunPlayer::UpdateHeavyAttackInput(float DeltaTime)
{
	if (!bHeavyAttackInputHeld && !bIsHeavyAttacking)
	{
		return;
	}

	const float HeavyAttackTimelineDeltaTime = DeltaTime * GetCurrentHeavyAttackTimelineRate();

	if (!bIsHeavyAttacking)
	{
		if (!CanStartHeavyAttack())
		{
			ResetHeavyAttackChargeInput();
			return;
		}

		HeavyAttackInputHoldTime += DeltaTime;
		if (!bHeavyAttackChargeStartPlayed && HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
		{
			StartHeavyAttackCharge();
		}

		return;
	}

	if (HeavyAttackState == EJunHeavyAttackState::ChargeLoop)
	{
		HeavyAttackChargeLoopElapsedTime += HeavyAttackTimelineDeltaTime;
		HeavyAttackSectionElapsedTime += HeavyAttackTimelineDeltaTime;
		if (bHeavyAttackChargeEndRequested &&
			AnimInstance &&
			AnimInstance->Montage_GetCurrentSection(HeavyAttackChargeMontage) == HeavyAttackChargeEndSectionName)
		{
			HeavyAttackState = EJunHeavyAttackState::ChargeEnd;
			HeavyAttackSectionElapsedTime = 0.f;
			ExecuteHeavyAttackChargeEndDash();
			ResetHeavyAttackChargeInput();
			return;
		}

		if (!bHeavyAttackChargeEndRequested &&
			HeavyAttackChargeLoopElapsedTime >= HeavyAttackChargeLoopMaxDuration)
		{
			bHeavyAttackInputHeld = false;
			bHeavyAttackChargeEndRequested = true;

			if (AnimInstance && HeavyAttackChargeMontage)
			{
				AnimInstance->Montage_SetNextSection(
					HeavyAttackChargeLoopSectionName,
					HeavyAttackChargeEndSectionName,
					HeavyAttackChargeMontage
				);
			}
		}
	}
	else if (HeavyAttackState == EJunHeavyAttackState::ChargeStart)
	{
		HeavyAttackSectionElapsedTime += HeavyAttackTimelineDeltaTime;
		HeavyAttackChargeStartRemainTime = FMath::Max(0.f, HeavyAttackChargeStartRemainTime - HeavyAttackTimelineDeltaTime);
		if (HeavyAttackChargeStartRemainTime <= 0.f)
		{
			if (bHeavyAttackInputHeld && !bHeavyAttackChargeEndRequested)
			{
				HeavyAttackState = EJunHeavyAttackState::ChargeLoop;
				HeavyAttackChargeStartRemainTime = 0.f;
				HeavyAttackChargeLoopElapsedTime = 0.f;
				HeavyAttackSectionElapsedTime = 0.f;
			}
			else
			{
				StartHeavyAttackChargeEnd();
			}
		}
	}
	else if (HeavyAttackState == EJunHeavyAttackState::Tap ||
		HeavyAttackState == EJunHeavyAttackState::ChargeEnd)
	{
		HeavyAttackSectionElapsedTime += HeavyAttackTimelineDeltaTime;
	}

	TryExecuteBufferedHeavyAttackRecoveryAction();
}

bool AJunPlayer::CanStartHeavyAttack() const
{
	if (!AnimInstance)
	{
		return false;
	}

	if ((GetCharacterMovement() && GetCharacterMovement()->IsFalling()) ||
		HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		HasGameplayTag(JunGameplayTags::State_Block_Attack) ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	if (bIsBasicAttacking || bIsHeavyAttacking)
	{
		return false;
	}

	return DefenseState == EJunDefenseState::None;
}

void AJunPlayer::ResetHeavyAttackChargeInput()
{
	bHeavyAttackInputHeld = false;
	bHeavyAttackChargeStartPlayed = false;
	bHeavyAttackChargeEndRequested = false;
	HeavyAttackInputHoldTime = 0.f;
	HeavyAttackChargeStartRemainTime = 0.f;
}

void AJunPlayer::StartHeavyAttackTap()
{
	if (!AnimInstance || !HeavyAttackTapMontage)
	{
		ResetHeavyAttackChargeInput();
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsHeavyAttacking = true;
	HeavyAttackState = EJunHeavyAttackState::Tap;
	CurrentHeavyAttackMontage = HeavyAttackTapMontage;
	CurrentHeavyAttackPlayRate = FMath::Max(HeavyAttackTapPlayRate, KINDA_SMALL_NUMBER);
	HeavyAttackChargeLoopElapsedTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	BufferedHeavyAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	HeavyAttackComboIndex = 1;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->Montage_Play(HeavyAttackTapMontage, CurrentHeavyAttackPlayRate);
	AnimInstance->Montage_JumpToSection(HeavyAttackTapFirstSectionName, HeavyAttackTapMontage);

	ResetHeavyAttackChargeInput();
}

void AJunPlayer::StartHeavyAttackCharge()
{
	if (bHeavyAttackChargeStartPlayed)
	{
		return;
	}

	if (!AnimInstance || !HeavyAttackChargeMontage)
	{
		StartHeavyAttackTap();
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsHeavyAttacking = true;
	bHeavyAttackChargeStartPlayed = true;
	bHeavyAttackChargeEndRequested = false;
	HeavyAttackState = EJunHeavyAttackState::ChargeStart;
	CurrentHeavyAttackMontage = HeavyAttackChargeMontage;
	CurrentHeavyAttackPlayRate = FMath::Max(HeavyAttackChargePlayRate, KINDA_SMALL_NUMBER);
	const int32 StartSectionIndex = HeavyAttackChargeMontage->GetSectionIndex(HeavyAttackChargeStartSectionName);
	HeavyAttackChargeStartRemainTime = StartSectionIndex != INDEX_NONE
		? HeavyAttackChargeMontage->GetSectionLength(StartSectionIndex)
		: HeavyAttackChargeMontage->GetPlayLength();
	HeavyAttackChargeLoopElapsedTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	bHeavyAttackChargeEndDashExecuted = false;
	BufferedHeavyAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	HeavyAttackComboIndex = 0;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->Montage_Play(HeavyAttackChargeMontage, CurrentHeavyAttackPlayRate);
	AnimInstance->Montage_SetNextSection(
		HeavyAttackChargeLoopSectionName,
		HeavyAttackChargeLoopSectionName,
		HeavyAttackChargeMontage
	);
	AnimInstance->Montage_JumpToSection(HeavyAttackChargeStartSectionName, HeavyAttackChargeMontage);
}

void AJunPlayer::StartHeavyAttackChargeEnd()
{
	if (!AnimInstance || !HeavyAttackChargeMontage)
	{
		FinishHeavyAttack();
		return;
	}

	if (!bIsHeavyAttacking)
	{
		CancelLockOnTurn();
		AddGameplayTag(JunGameplayTags::State_Action_Attack);
		AddGameplayTag(JunGameplayTags::State_Block_Move);
		AddGameplayTag(JunGameplayTags::State_Block_Jump);
		AddGameplayTag(JunGameplayTags::State_Block_Attack);
		AddGameplayTag(JunGameplayTags::State_Block_Dodge);
		bIsHeavyAttacking = true;
	}

	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	HeavyAttackState = EJunHeavyAttackState::ChargeEnd;
	CurrentHeavyAttackMontage = HeavyAttackChargeMontage;
	CurrentHeavyAttackPlayRate = FMath::Max(HeavyAttackChargePlayRate, KINDA_SMALL_NUMBER);
	HeavyAttackChargeStartRemainTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	HeavyAttackComboIndex = 0;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
	if (!AnimInstance->Montage_IsPlaying(HeavyAttackChargeMontage))
	{
		AnimInstance->Montage_Play(HeavyAttackChargeMontage, CurrentHeavyAttackPlayRate);
	}
	AnimInstance->Montage_JumpToSection(HeavyAttackChargeEndSectionName, HeavyAttackChargeMontage);
	ExecuteHeavyAttackChargeEndDash();
	ResetHeavyAttackChargeInput();
}

void AJunPlayer::ExecuteHeavyAttackChargeEndDash()
{
	if (bHeavyAttackChargeEndDashExecuted)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	bHeavyAttackChargeEndDashExecuted = true;

	const float FullDashThreshold =
		HeavyAttackChargeLoopMaxDuration * FMath::Clamp(HeavyAttackChargeFullDashThresholdRatio, 0.f, 1.f);
	const bool bUseFullDash = HeavyAttackChargeLoopElapsedTime >= FullDashThreshold;
	const float DashSpeed = bUseFullDash
		? HeavyAttackChargeEndDashMaxSpeed
		: HeavyAttackChargeEndDashMaxSpeed * HeavyAttackChargeEndDashMinSpeedRatio;

	FVector DashDirection = GetActorForwardVector();
	DashDirection.Z = 0.f;
	DashDirection = DashDirection.GetSafeNormal();
	if (DashDirection.IsNearlyZero())
	{
		return;
	}

	const float CurrentVerticalSpeed = MovementComponent->Velocity.Z;
	MovementComponent->Velocity = FVector(
		DashDirection.X * DashSpeed,
		DashDirection.Y * DashSpeed,
		CurrentVerticalSpeed
	);

	KnockbackBrakingOverrideRemainTime = HeavyAttackChargeEndDashGroundMotionDuration;
	KnockbackBrakingDecelerationOverride = 0.f;
	KnockbackGroundFrictionOverride = 0.f;
	KnockbackBrakingFrictionFactorOverride = 0.f;
}

void AJunPlayer::FinishHeavyAttack()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsHeavyAttacking = false;
	HeavyAttackState = EJunHeavyAttackState::None;
	CurrentHeavyAttackMontage = nullptr;
	CurrentHeavyAttackPlayRate = 1.f;
	HeavyAttackChargeStartRemainTime = 0.f;
	HeavyAttackChargeLoopElapsedTime = 0.f;
	HeavyAttackSectionElapsedTime = 0.f;
	bHeavyAttackChargeEndDashExecuted = false;
	BufferedHeavyAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	HeavyAttackComboIndex = 0;
	bCanBufferHeavyAttackComboInput = false;
	bBufferedHeavyAttackComboInput = false;
	bBufferedHeavyAttackInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	ResetHeavyAttackChargeInput();
}

float AJunPlayer::GetCurrentHeavyAttackTimelineRate() const
{
	return FMath::Max(CurrentHeavyAttackPlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction Action) const
{
	if (!bIsHeavyAttacking)
	{
		return false;
	}

	if (HeavyAttackState != EJunHeavyAttackState::Tap &&
		HeavyAttackState != EJunHeavyAttackState::ChargeEnd)
	{
		return false;
	}

	float CancelOpenTime = 0.f;
	switch (Action)
	{
	case EJunBufferedRecoveryAction::Dodge:
		CancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
			? HeavyAttackTapDodgeCancelOpenTime
			: HeavyAttackChargeEndDodgeCancelOpenTime;
		break;
	case EJunBufferedRecoveryAction::Jump:
		CancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
			? HeavyAttackTapJumpCancelOpenTime
			: HeavyAttackChargeEndJumpCancelOpenTime;
		break;
	case EJunBufferedRecoveryAction::Defense:
		CancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
			? HeavyAttackTapDefenseCancelOpenTime
			: HeavyAttackChargeEndDefenseCancelOpenTime;
		break;
	default:
		return false;
	}

	return HeavyAttackSectionElapsedTime >= CancelOpenTime;
}

void AJunPlayer::BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction Action)
{
	BufferedHeavyAttackRecoveryAction = Action;
	TryExecuteBufferedHeavyAttackRecoveryAction();
}

void AJunPlayer::TryExecuteBufferedHeavyAttackRecoveryAction()
{
	if (BufferedHeavyAttackRecoveryAction == EJunBufferedRecoveryAction::None ||
		!CanCancelHeavyAttackIntoRecoveryAction(BufferedHeavyAttackRecoveryAction))
	{
		return;
	}

	const EJunBufferedRecoveryAction PendingAction = BufferedHeavyAttackRecoveryAction;
	BufferedHeavyAttackRecoveryAction = EJunBufferedRecoveryAction::None;

	switch (PendingAction)
	{
	case EJunBufferedRecoveryAction::Dodge:
		CancelHeavyAttackForRecoveryTransition(HeavyAttackDodgeCancelBlendOutTime);
		StartDodge();
		break;
	case EJunBufferedRecoveryAction::Jump:
		CancelHeavyAttackForRecoveryTransition(HeavyAttackJumpCancelBlendOutTime);
		Jump();
		break;
	case EJunBufferedRecoveryAction::Defense:
		CancelHeavyAttackForRecoveryTransition(HeavyAttackDefenseCancelBlendOutTime);
		BeginDefenseFromBufferedInput();
		break;
	default:
		break;
	}
}

void AJunPlayer::CancelHeavyAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsHeavyAttacking)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	if (AnimInstance && CurrentHeavyAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, CurrentHeavyAttackMontage);
	}

	FinishHeavyAttack();
}

bool AJunPlayer::CanCancelHeavyAttackIntoMove() const
{
	if (!bIsHeavyAttacking)
	{
		return false;
	}

	const float MoveCancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
		? HeavyAttackTapMoveCancelOpenTime
		: HeavyAttackChargeEndMoveCancelOpenTime;

	return (HeavyAttackState == EJunHeavyAttackState::Tap ||
		HeavyAttackState == EJunHeavyAttackState::ChargeEnd) &&
		HeavyAttackSectionElapsedTime >= MoveCancelOpenTime;
}

bool AJunPlayer::TryCancelHeavyAttackIntoMove()
{
	if (!CanCancelHeavyAttackIntoMove())
	{
		return false;
	}

	CancelHeavyAttackForRecoveryTransition(HeavyAttackMoveCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelHeavyAttackIntoBasicAttack() const
{
	if (!bIsHeavyAttacking)
	{
		return false;
	}

	if (HeavyAttackState != EJunHeavyAttackState::Tap &&
		HeavyAttackState != EJunHeavyAttackState::ChargeEnd)
	{
		return false;
	}

	const float BasicAttackCancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
		? HeavyAttackTapBasicAttackCancelOpenTime
		: HeavyAttackChargeEndBasicAttackCancelOpenTime;

	return HeavyAttackSectionElapsedTime >= BasicAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelHeavyAttackIntoBasicAttack()
{
	if (!CanCancelHeavyAttackIntoBasicAttack())
	{
		return false;
	}

	CancelHeavyAttackForRecoveryTransition(HeavyAttackBasicAttackCancelBlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::CanCancelHeavyAttackIntoHeavyAttack() const
{
	if (!bIsHeavyAttacking)
	{
		return false;
	}

	if (HeavyAttackState != EJunHeavyAttackState::Tap &&
		HeavyAttackState != EJunHeavyAttackState::ChargeEnd)
	{
		return false;
	}

	const float HeavyAttackCancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
		? HeavyAttackTapBasicAttackCancelOpenTime
		: HeavyAttackChargeEndBasicAttackCancelOpenTime;

	return HeavyAttackSectionElapsedTime >= HeavyAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelHeavyAttackIntoHeavyAttack()
{
	if (!CanCancelHeavyAttackIntoHeavyAttack())
	{
		return false;
	}

	CancelHeavyAttackForRecoveryTransition(HeavyAttackBasicAttackCancelBlendOutTime);
	return true;
}

void AJunPlayer::UpdateJumpAttackState(float DeltaTime)
{
	if (!bIsJumpAttacking)
	{
		return;
	}

	JumpAttackSectionElapsedTime += DeltaTime * GetCurrentJumpAttackTimelineRate();

	if (bJumpAttackEndRequested &&
		JumpAttackState != EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackMinStartTime)
	{
		EnterJumpAttackEnd();
	}
}

bool AJunPlayer::CanStartJumpAttack() const
{
	return AnimInstance &&
		JumpAttackMontage &&
		GetCharacterMovement() &&
		GetCharacterMovement()->IsFalling() &&
		HasEnoughAirTimeForJumpAttack() &&
		!bIsJumpAttacking &&
		!bIsBasicAttacking &&
		!bIsHeavyAttacking &&
		DefenseState == EJunDefenseState::None &&
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		!HasGameplayTag(JunGameplayTags::State_Block_Attack) &&
		!HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
}

bool AJunPlayer::HasEnoughAirTimeForJumpAttack() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return false;
	}

	if (MovementComponent->Velocity.Z < JumpAttackMinStartVerticalVelocity)
	{
		return false;
	}

	if (MovementComponent->Velocity.Z >= 0.f)
	{
		return true;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return false;
	}

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float TraceDistance = CapsuleHalfHeight + JumpAttackMinGroundDistance + 20.f;
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JumpAttackGroundCheck), false, this);
	FHitResult Hit;
	const bool bHitGround = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (!bHitGround)
	{
		return true;
	}

	const float DistanceFromCapsuleBottomToGround = FMath::Max(0.f, Hit.Distance - CapsuleHalfHeight);
	return DistanceFromCapsuleBottomToGround >= JumpAttackMinGroundDistance;
}

void AJunPlayer::StartJumpAttack()
{
	if (!CanStartJumpAttack())
	{
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	const FVector HorizontalVelocity = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
	const bool bHasMoveInput = !FMath::IsNearlyZero(DesiredMoveForward) ||
		!FMath::IsNearlyZero(DesiredMoveRight) ||
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);
	const bool bShouldApplyStationaryJumpAttackImpulse =
		!bHasMoveInput &&
		HorizontalVelocity.SizeSquared() <= FMath::Square(JumpAttackStationaryImpulseMaxHorizontalSpeed);

	if (bShouldApplyStationaryJumpAttackImpulse && JumpAttackStartForwardImpulseStrength > 0.f)
	{
		FVector ForwardImpulseDirection = FVector::ZeroVector;
		if (TargetActor)
		{
			ForwardImpulseDirection = TargetActor->GetActorLocation() - GetActorLocation();
			ForwardImpulseDirection.Z = 0.f;
		}

		if (ForwardImpulseDirection.IsNearlyZero())
		{
			ForwardImpulseDirection = GetActorForwardVector();
			ForwardImpulseDirection.Z = 0.f;
		}

		if (!ForwardImpulseDirection.IsNearlyZero())
		{
			ForwardImpulseDirection.Normalize();
			GetCharacterMovement()->AddImpulse(ForwardImpulseDirection * JumpAttackStartForwardImpulseStrength, true);
		}
	}

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJumpAttacking = true;
	bJumpAttackEndRequested = false;
	JumpAttackState = EJunJumpAttackState::Start;
	JumpAttackSectionElapsedTime = 0.f;
	CurrentJumpAttackPlayRate = FMath::Max(JumpAttackPlayRate, KINDA_SMALL_NUMBER);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
	AnimInstance->Montage_Play(JumpAttackMontage, CurrentJumpAttackPlayRate);
	AnimInstance->Montage_JumpToSection(JumpAttackStartSectionName, JumpAttackMontage);
}

void AJunPlayer::RequestJumpAttackEnd()
{
	if (!bIsJumpAttacking)
	{
		return;
	}

	bJumpAttackEndRequested = true;
	if (JumpAttackSectionElapsedTime >= JumpAttackMinStartTime)
	{
		EnterJumpAttackEnd();
	}
}

void AJunPlayer::EnterJumpAttackEnd()
{
	if (!bIsJumpAttacking || JumpAttackState == EJunJumpAttackState::End)
	{
		return;
	}

	JumpAttackState = EJunJumpAttackState::End;
	JumpAttackSectionElapsedTime = 0.f;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	if (AnimInstance && JumpAttackMontage)
	{
		AnimInstance->Montage_JumpToSection(JumpAttackEndSectionName, JumpAttackMontage);
	}
}

void AJunPlayer::FinishJumpAttack()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJumpAttacking = false;
	bJumpAttackEndRequested = false;
	JumpAttackState = EJunJumpAttackState::None;
	JumpAttackSectionElapsedTime = 0.f;
	CurrentJumpAttackPlayRate = 1.f;
}

float AJunPlayer::GetCurrentJumpAttackTimelineRate() const
{
	return FMath::Max(CurrentJumpAttackPlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::CanCancelJumpAttackEndIntoMove() const
{
	return bIsJumpAttacking &&
		JumpAttackState == EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackEndMoveCancelOpenTime;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoMove()
{
	if (!CanCancelJumpAttackEndIntoMove())
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(JumpAttackEndMoveCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelJumpAttackEndIntoDodge() const
{
	return bIsJumpAttacking &&
		JumpAttackState == EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackEndDodgeCancelOpenTime;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoDodge()
{
	if (!CanCancelJumpAttackEndIntoDodge())
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(JumpAttackEndDodgeCancelBlendOutTime);
	StartDodge();
	return true;
}

bool AJunPlayer::CanCancelJumpAttackEndIntoDefense() const
{
	return bIsJumpAttacking &&
		JumpAttackState == EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackEndDefenseCancelOpenTime;
}

bool AJunPlayer::CanCancelJumpAttackEndIntoBasicAttack() const
{
	return bIsJumpAttacking &&
		JumpAttackState == EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackEndBasicAttackCancelOpenTime;
}

bool AJunPlayer::CanCancelJumpAttackEndIntoHeavyAttack() const
{
	return bIsJumpAttacking &&
		JumpAttackState == EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackEndHeavyAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoDefense()
{
	if (!CanCancelJumpAttackEndIntoDefense())
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(JumpAttackEndDefenseCancelBlendOutTime);
	BeginDefenseFromBufferedInput();
	return true;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoBasicAttack()
{
	if (!CanCancelJumpAttackEndIntoBasicAttack())
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(JumpAttackEndBasicAttackCancelBlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoHeavyAttack()
{
	if (!CanCancelJumpAttackEndIntoHeavyAttack())
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(JumpAttackEndHeavyAttackCancelBlendOutTime);
	return true;
}

void AJunPlayer::CancelJumpAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsJumpAttacking)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	if (AnimInstance && JumpAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, JumpAttackMontage);
	}

	FinishJumpAttack();
}

void AJunPlayer::UpdateBasicAttackJumpAndMoveCancelState(float DeltaTime)
{
	if (!bIsBasicAttacking)
	{
		return;
	}

	BasicAttackSectionElapsedTime += DeltaTime * FMath::Max(BasicAttackPlayRate, KINDA_SMALL_NUMBER);

	const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
	const float JumpCancelOpenTime = BasicAttackJumpCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackJumpCancelOpenTimes[ComboArrayIndex]
		: 0.18f;
	const float MoveCancelOpenTime = BasicAttackMoveCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackMoveCancelOpenTimes[ComboArrayIndex]
		: JumpCancelOpenTime;

	if (BasicAttackSectionElapsedTime >= JumpCancelOpenTime)
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	}

	if (BasicAttackSectionElapsedTime >= MoveCancelOpenTime)
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	}
}

bool AJunPlayer::CanCancelBasicAttackIntoMove() const
{
	if (!bIsBasicAttacking)
	{
		return false;
	}

	const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
	const float MoveCancelOpenTime = BasicAttackMoveCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackMoveCancelOpenTimes[ComboArrayIndex]
		: 0.18f;

	return BasicAttackSectionElapsedTime >= MoveCancelOpenTime;
}

bool AJunPlayer::TryCancelBasicAttackIntoMove()
{
	if (!CanCancelBasicAttackIntoMove())
	{
		return false;
	}

	CancelBasicAttackForRecoveryTransition(BasicAttackMoveCancelBlendOutTime);
	return true;
}

void AJunPlayer::UpdateBasicAttackRecoveryBuffer(float DeltaTime)
{
	if (bIsBasicAttacking)
	{
		return;
	}

	if (PostBasicAttackJumpDodgeBufferRemainTime > 0.f)
	{
		PostBasicAttackJumpDodgeBufferRemainTime = FMath::Max(0.f, PostBasicAttackJumpDodgeBufferRemainTime - DeltaTime);
	}

	if (PostBasicAttackDefenseBufferRemainTime > 0.f)
	{
		PostBasicAttackDefenseBufferRemainTime = FMath::Max(0.f, PostBasicAttackDefenseBufferRemainTime - DeltaTime);
	}
}

void AJunPlayer::CompleteGuardBlockReleaseIntoGuardEnd()
{
	GetWorldTimerManager().ClearTimer(GuardBlockReleaseIntoGuardEndTimerHandle);

	if (DefenseState == EJunDefenseState::Guarding && !bDefenseButtonHeld)
	{
		BeginGuardEnd();
	}
}

void AJunPlayer::TryExecuteBufferedDefenseTransitionCancelAction()
{
	const bool bCanExecute =
		(DefenseState == EJunDefenseState::Starting ||
		 DefenseState == EJunDefenseState::Ending) ||
		PostDefenseTransitionCancelBufferRemainTime > 0.f;

	if (!bCanExecute)
	{
		return;
	}

	ExecuteBufferedDefenseTransitionCancelAction();
}

void AJunPlayer::ExecuteBufferedDefenseTransitionCancelAction()
{
	if (BufferedDefenseTransitionCancelAction == EJunBufferedDefenseCancelAction::None)
	{
		return;
	}

	const EJunBufferedDefenseCancelAction PendingAction = BufferedDefenseTransitionCancelAction;
	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;

	switch (PendingAction)
	{
	case EJunBufferedDefenseCancelAction::BasicAttack:
		CancelDefenseForCancelTransition();
		BasicAttack();
		break;
	case EJunBufferedDefenseCancelAction::Jump:
		CancelDefenseForCancelTransition();
		Jump();
		break;
	case EJunBufferedDefenseCancelAction::Dodge:
		CancelDefenseForCancelTransition();
		StartDodge();
		break;
	case EJunBufferedDefenseCancelAction::Move:
		if (!CanCancelDefenseTransitionIntoMove())
		{
			BufferedDefenseTransitionCancelAction = PendingAction;
			return;
		}

		CancelDefenseForCancelTransition(DefenseMoveCancelBlendOutTime);
		break;
	default:
		break;
	}
}

void AJunPlayer::TryExecuteBufferedParrySuccessCancelAction()
{
	if (BufferedParrySuccessCancelAction == EJunBufferedParrySuccessCancelAction::None)
	{
		return;
	}

	if (!CanBufferParrySuccessCancel(BufferedParrySuccessCancelAction))
	{
		return;
	}

	ExecuteBufferedParrySuccessCancelAction();
}

void AJunPlayer::CancelParrySuccessForCancelTransition(float BlendOutTime)
{
	if (AnimInstance && CurrentParrySuccessMontage)
	{
		AnimInstance->Montage_Stop(BlendOutTime, CurrentParrySuccessMontage);
	}

	FinishPlayerHitState();
}

void AJunPlayer::ExecuteBufferedParrySuccessCancelAction()
{
	if (BufferedParrySuccessCancelAction == EJunBufferedParrySuccessCancelAction::None)
	{
		return;
	}

	const EJunBufferedParrySuccessCancelAction PendingAction = BufferedParrySuccessCancelAction;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;

	switch (PendingAction)
	{
	case EJunBufferedParrySuccessCancelAction::BasicAttack:
		CancelParrySuccessForCancelTransition(ParrySuccessBasicAttackCancelBlendOutTime);
		BasicAttack();
		break;
	case EJunBufferedParrySuccessCancelAction::HeavyAttack:
		CancelParrySuccessForCancelTransition(ParrySuccessHeavyAttackCancelBlendOutTime);
		StartHeavyAttackTap();
		break;
	case EJunBufferedParrySuccessCancelAction::Jump:
		CancelParrySuccessForCancelTransition(ParrySuccessJumpCancelBlendOutTime);
		Jump();
		break;
	case EJunBufferedParrySuccessCancelAction::Dodge:
		CancelParrySuccessForCancelTransition(ParrySuccessDodgeCancelBlendOutTime);
		StartDodge();
		break;
	case EJunBufferedParrySuccessCancelAction::Move:
		CancelParrySuccessForCancelTransition(ParrySuccessMoveCancelBlendOutTime);
		break;
	default:
		break;
	}
}

void AJunPlayer::CancelDefenseForCancelTransition(float BlendOutTime)
{
	if (DefenseState == EJunDefenseState::None || DefenseState == EJunDefenseState::Guarding)
	{
		return;
	}

	const float ResolvedBlendOutTime =
		BlendOutTime >= 0.f ? BlendOutTime : DefenseCancelBlendOutTime;

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);

		if (GuardStartMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, GuardStartMontage);
		}

		if (GuardEndMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, GuardEndMontage);
		}
	}

	FinishDefenseForCancel();
}

void AJunPlayer::FinishDodgeState()
{
	if (!HasGameplayTag(JunGameplayTags::State_Action_Dodge) && !CurrentDodgeMontage)
	{
		return;
	}

	RemoveGameplayTag(JunGameplayTags::State_Action_Dodge);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	DodgeFinishRemainTime = 0.f;
	DodgeInvincibleRemainTime = 0.f;
	DodgeElapsedTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	if (DodgeInternalCooldownRemainTime <= 0.f)
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
		CurrentDodgePlayRate = 1.f;
	}
	GetCharacterMovement()->bOrientRotationToMovement = !bLockOnActive;
	CurrentDodgeMontage = nullptr;
	bDodgeAttackWindowActive = false;
	bDodgeChainWindowActive = false;
}

void AJunPlayer::FinishDodgeAttack()
{
	EndAttackFacingWindow();

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsDodgeAttacking = false;
	DodgeAttackElapsedTime = 0.f;
	CurrentDodgeAttackMontage = nullptr;
}

float AJunPlayer::GetCurrentDodgeAttackTimelineRate() const
{
	return FMath::Max(DodgeAttackPlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::CanCancelDodgeAttackIntoMove() const
{
	return bIsDodgeAttacking && DodgeAttackElapsedTime >= DodgeAttackMoveCancelOpenTime;
}

bool AJunPlayer::TryCancelDodgeAttackIntoMove()
{
	if (!CanCancelDodgeAttackIntoMove())
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(DodgeAttackMoveCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoDodge() const
{
	return bIsDodgeAttacking && DodgeAttackElapsedTime >= DodgeAttackDodgeCancelOpenTime;
}

bool AJunPlayer::TryCancelDodgeAttackIntoDodge()
{
	if (!CanCancelDodgeAttackIntoDodge())
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(DodgeAttackDodgeCancelBlendOutTime);
	StartDodge();
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoBasicAttack() const
{
	return bIsDodgeAttacking && DodgeAttackElapsedTime >= DodgeAttackBasicAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelDodgeAttackIntoBasicAttack()
{
	if (!CanCancelDodgeAttackIntoBasicAttack())
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(DodgeAttackBasicAttackCancelBlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoHeavyAttack() const
{
	return bIsDodgeAttacking && DodgeAttackElapsedTime >= DodgeAttackHeavyAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelDodgeAttackIntoHeavyAttack()
{
	if (!CanCancelDodgeAttackIntoHeavyAttack())
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(DodgeAttackHeavyAttackCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoDefense() const
{
	return bIsDodgeAttacking && DodgeAttackElapsedTime >= DodgeAttackDefenseCancelOpenTime;
}

bool AJunPlayer::TryCancelDodgeAttackIntoDefense()
{
	if (!CanCancelDodgeAttackIntoDefense())
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(DodgeAttackDefenseCancelBlendOutTime);
	BeginDefenseFromBufferedInput();
	return true;
}

void AJunPlayer::CancelDodgeAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsDodgeAttacking)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	if (AnimInstance && CurrentDodgeAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, CurrentDodgeAttackMontage);
	}

	FinishDodgeAttack();
}

bool AJunPlayer::TryStartExecution()
{
	if (bIsExecuting)
	{
		return false;
	}

	AJunMonster* ExecutionTarget = Cast<AJunMonster>(LockOnTarget.Get());
	if (!CanStartExecution(ExecutionTarget))
	{
		ExecutionTarget = Cast<AJunMonster>(FindBestAttackTarget());
	}

	if (!CanStartExecution(ExecutionTarget))
	{
		return false;
	}

	StartExecution(ExecutionTarget);
	return true;
}

bool AJunPlayer::CanStartExecution(const AJunMonster* Monster) const
{
	return Monster
		&& GetExecutionMontageForTarget(Monster)
		&& AnimInstance
		&& Monster->CanBeExecutedBy(this);
}

void AJunPlayer::StartExecution(AJunMonster* Monster)
{
	if (!CanStartExecution(Monster))
	{
		return;
	}

	if (!Monster->TryBeginExecutionBy(this))
	{
		return;
	}

	InterruptActionsForHitReaction();
	FinishPlayerHitState();

	bIsExecuting = true;
	CurrentExecutionTarget = Monster;
	bCurrentExecutionIsFinish = Monster->IsFinalExecution();
	CurrentExecutionMontage = GetExecutionMontageForTarget(Monster);
	TargetActor = Monster;
	SetExecutionCapsuleCollisionIgnore(Monster, true);
	ReduceExecutionCapsuleRadius(Monster);
	StartExecutionCamera(Monster);

	const FVector ToTarget = Monster->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.ToOrientationRotator());
	}

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnExecutionMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnExecutionMontageEnded);
	PlayAnimMontage(CurrentExecutionMontage);
}

void AJunPlayer::TriggerExecutionTargetMontage()
{
	if (!bIsExecuting || !CurrentExecutionTarget)
	{
		return;
	}

	CurrentExecutionTarget->TriggerPendingExecutionMontage(!bCurrentExecutionIsFinish);
}

void AJunPlayer::FinishExecution()
{
	if (!bIsExecuting)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnExecutionMontageEnded);
	}

	bIsExecuting = false;
	EndAttackFacingWindow();
	EndExecutionCamera();
	bCurrentExecutionIsFinish = false;
	if (CurrentExecutionTarget && !CurrentExecutionTarget->HasExecutionResultApplied())
	{
		CurrentExecutionTarget->CancelPendingExecution();
	}
	SetExecutionCapsuleCollisionIgnore(CurrentExecutionTarget.Get(), false);
	RestoreExecutionCapsuleRadius(CurrentExecutionTarget.Get());
	CurrentExecutionTarget = nullptr;
	CurrentExecutionMontage = nullptr;

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentExecutionMontage)
	{
		return;
	}

	FinishExecution();
}

UAnimMontage* AJunPlayer::GetExecutionMontageForTarget(const AJunMonster* Monster) const
{
	if (Monster && Monster->IsFinalExecution())
	{
		return ExecutionFinishMontage ? ExecutionFinishMontage.Get() : ExecutionMontage.Get();
	}

	return ExecutionMontage.Get();
}

void AJunPlayer::SetExecutionCapsuleCollisionIgnore(AJunMonster* Monster, bool bIgnore)
{
	if (!Monster)
	{
		return;
	}

	if (bIgnore && !Monster->ShouldIgnoreExecutorCapsuleCollisionDuringExecution())
	{
		return;
	}

	if (UCapsuleComponent* PlayerCapsule = GetCapsuleComponent())
	{
		PlayerCapsule->IgnoreActorWhenMoving(Monster, bIgnore);
	}

	if (UCapsuleComponent* MonsterCapsule = Monster->GetCapsuleComponent())
	{
		MonsterCapsule->IgnoreActorWhenMoving(this, bIgnore);
	}
}

void AJunPlayer::ReduceExecutionCapsuleRadius(AJunMonster* Monster)
{
	if (UCapsuleComponent* PlayerCapsule = GetCapsuleComponent())
	{
		if (!bExecutionCapsuleRadiusReduced)
		{
			SavedExecutionCapsuleRadius = PlayerCapsule->GetUnscaledCapsuleRadius();
			bExecutionCapsuleRadiusReduced = true;
		}

		PlayerCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius / 3.f, true);
	}

	if (!Monster)
	{
		return;
	}

	if (UCapsuleComponent* MonsterCapsule = Monster->GetCapsuleComponent())
	{
		if (!Monster->bExecutionCapsuleRadiusReduced)
		{
			Monster->SavedExecutionCapsuleRadius = MonsterCapsule->GetUnscaledCapsuleRadius();
			Monster->bExecutionCapsuleRadiusReduced = true;
		}

		MonsterCapsule->SetCapsuleRadius(Monster->SavedExecutionCapsuleRadius / 3.f, true);
	}
}

void AJunPlayer::RestoreExecutionCapsuleRadius(AJunMonster* Monster)
{
	if (UCapsuleComponent* PlayerCapsule = GetCapsuleComponent())
	{
		if (bExecutionCapsuleRadiusReduced && SavedExecutionCapsuleRadius > 0.f)
		{
			PlayerCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius, true);
		}
	}

	SavedExecutionCapsuleRadius = 0.f;
	bExecutionCapsuleRadiusReduced = false;

	if (Monster)
	{
		Monster->RestoreExecutionCapsuleRadius();
	}
}

void AJunPlayer::FinishDefenseForCancel()
{
	DefenseState = EJunDefenseState::None;
	CurrentDefensePlayRate = 1.f;
	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	bKeepGuardBaseWhileEnding = false;
	bDeferGuardMoveInput = false;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	GuardExitMoveReleaseRemainTime = 0.f;
	GuardMoveBlendRemainTime = 0.f;
	ClearDefenseBlockTags();
	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;
}

void AJunPlayer::CancelBasicAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsBasicAttacking)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	if (AnimInstance && BasicAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnBasicAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, BasicAttackMontage);
	}

	FinishBasicAttack();
}

void AJunPlayer::ResetBasicAttackCombo()
{
	FinishBasicAttack();
	bBufferedBasicAttackInput = false;
}

void AJunPlayer::FinishBasicAttack()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);

	bIsBasicAttacking = false;
	bCanBufferBasicAttackComboInput = false;
	bBufferedBasicAttackComboInput = false;
	bBasicAttackComboAdvanceStateActive = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	PostBasicAttackJumpDodgeBufferRemainTime = PostBasicAttackJumpDodgeBufferDuration;
	PostBasicAttackDefenseBufferRemainTime = PostBasicAttackDefenseBufferDuration;
	BasicAttackSectionElapsedTime = 0.f;
	BasicAttackComboIndex = 0;
}

void AJunPlayer::StartBasicAttack()
{
	if (!AnimInstance || !BasicAttackMontage)
	{
		return;
	}

	// 怨듦꺽 ?쒖옉 ???寃??뚯쟾 蹂댁“/?대룞 李⑤떒/肄ㅻ낫 ?곹깭瑜???踰덉뿉 ?명똿?쒕떎.
	bBufferedBasicAttackInput = false;

	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);

	bIsBasicAttacking = true;
	bCanBufferBasicAttackComboInput = false;
	bBufferedBasicAttackComboInput = false;
	bBasicAttackComboAdvanceStateActive = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	PostBasicAttackJumpDodgeBufferRemainTime = 0.f;
	PostBasicAttackDefenseBufferRemainTime = 0.f;
	BasicAttackSectionElapsedTime = 0.f;
	BasicAttackComboIndex = 1;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnBasicAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnBasicAttackMontageEnded);

	AnimInstance->Montage_Play(BasicAttackMontage, FMath::Max(BasicAttackPlayRate, KINDA_SMALL_NUMBER));
	AnimInstance->Montage_JumpToSection(FName("1"), BasicAttackMontage);
}

void AJunPlayer::TryAdvanceBasicAttackCombo()
{
	if (!bIsBasicAttacking || !bBufferedBasicAttackComboInput || !BasicAttackMontage)
	{
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	FName NextSectionName = NAME_None;

	switch (BasicAttackComboIndex)
	{
	case 1:
		NextSectionName = FName("2");
		break;
	case 2:
		NextSectionName = FName("3");
		break;
	case 3:
		NextSectionName = FName("4");
		break;
	case 4:
		bBufferedBasicAttackComboInput = false;
		bCanBufferBasicAttackComboInput = false;
		bBasicAttackComboAdvanceStateActive = false;
		bCanRestartBasicAttackAfterComboAdvance = false;
		CancelBasicAttackForRecoveryTransition(BasicAttackRestartBlendOutTime);
		StartBasicAttack();
		return;
	default:
		return;
	}

	bBufferedBasicAttackComboInput = false;
	bCanBufferBasicAttackComboInput = false;
	bBasicAttackComboAdvanceStateActive = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	BasicAttackComboIndex = BasicAttackComboIndex >= 4 ? 1 : BasicAttackComboIndex + 1;
	BasicAttackSectionElapsedTime = 0.f;

	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AnimInstance->Montage_JumpToSection(NextSectionName, BasicAttackMontage);
}

void AJunPlayer::TryAdvanceHeavyAttackCombo()
{
	if (!bIsHeavyAttacking ||
		!bBufferedHeavyAttackComboInput ||
		!HeavyAttackTapMontage ||
		HeavyAttackState != EJunHeavyAttackState::Tap)
	{
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (HeavyAttackComboIndex <= 1)
	{
		bBufferedHeavyAttackComboInput = false;
		bCanBufferHeavyAttackComboInput = false;
		bHeavyAttackComboAdvanceStateActive = false;
		bCanRestartHeavyAttackAfterComboAdvance = false;
		HeavyAttackComboIndex = 2;
		HeavyAttackSectionElapsedTime = 0.f;

		TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

		const int32 SecondSectionIndex = HeavyAttackTapMontage->GetSectionIndex(HeavyAttackTapSecondSectionName);
		if (SecondSectionIndex != INDEX_NONE)
		{
			float SectionStartTime = 0.f;
			float SectionEndTime = 0.f;
			HeavyAttackTapMontage->GetSectionStartAndEndTime(SecondSectionIndex, SectionStartTime, SectionEndTime);

			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
			const float MontagePlayResult = AnimInstance->Montage_PlayWithBlendIn(
				HeavyAttackTapMontage,
				FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackComboBlendInTime)),
				CurrentHeavyAttackPlayRate,
				EMontagePlayReturnType::MontageLength,
				SectionStartTime,
				true
			);
			AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);

			if (MontagePlayResult > 0.f)
			{
				return;
			}
		}

		AnimInstance->Montage_JumpToSection(HeavyAttackTapSecondSectionName, HeavyAttackTapMontage);
		return;
	}

	bBufferedHeavyAttackComboInput = false;
	bCanBufferHeavyAttackComboInput = false;
	bHeavyAttackComboAdvanceStateActive = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	CancelHeavyAttackForRecoveryTransition(HeavyAttackRestartBlendOutTime);
	StartHeavyAttackTap();
}

void AJunPlayer::CancelBasicAttackIntoDefense()
{
	CancelBasicAttackForRecoveryTransition();
}

void AJunPlayer::OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != BasicAttackMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	ResetBasicAttackCombo();

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}
}

void AJunPlayer::OnHeavyAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentHeavyAttackMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	switch (HeavyAttackState)
	{
	case EJunHeavyAttackState::ChargeStart:
		// ChargeStart transition is driven by UpdateHeavyAttackInput so blend-out timing
		// cannot skip or duplicate the startup pose.
		break;
	case EJunHeavyAttackState::ChargeLoop:
		// Loop exits through input release or HeavyAttackChargeLoopMaxDuration.
		break;
	case EJunHeavyAttackState::Tap:
	case EJunHeavyAttackState::ChargeEnd:
	default:
		FinishHeavyAttack();
		break;
	}
}

void AJunPlayer::OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != JumpAttackMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
	}

	if (bIsJumpAttacking)
	{
		FinishJumpAttack();
	}
}

void AJunPlayer::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeMontage)
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		FinishDodgeState();
	}
}

void AJunPlayer::OnDodgeAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeAttackMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
	}

	if (bIsDodgeAttacking)
	{
		FinishDodgeAttack();
	}
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

void AJunPlayer::AlignActorToDesiredMoveDirectionForDodge()
{
	if (!Controller)
	{
		return;
	}

	FVector2D DesiredMoveInput(DesiredMoveForward, DesiredMoveRight);
	if (DesiredMoveInput.IsNearlyZero())
	{
		DesiredMoveInput = FVector2D(PendingMoveForward, PendingMoveRight);
	}

	if (DesiredMoveInput.IsNearlyZero())
	{
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Pitch = 0.f;
	ControlRotation.Roll = 0.f;

	const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(ControlRotation);
	const FVector RightDirection = UKismetMathLibrary::GetRightVector(ControlRotation);

	FVector DesiredWorldDirection =
		(ForwardDirection * DesiredMoveInput.X) +
		(RightDirection * DesiredMoveInput.Y);

	DesiredWorldDirection.Z = 0.f;

	if (DesiredWorldDirection.IsNearlyZero())
	{
		return;
	}

	SetActorRotation(DesiredWorldDirection.GetSafeNormal().Rotation());
}

UAnimMontage* AJunPlayer::GetDodgeMontageToPlay() const
{
	FVector2D DodgeInput(
		FMath::Clamp(DesiredMoveForward, -1.f, 1.f),
		FMath::Clamp(DesiredMoveRight, -1.f, 1.f)
	);

	if (DodgeInput.IsNearlyZero())
	{
		DodgeInput = FVector2D(
			FMath::Clamp(PendingMoveForward, -1.f, 1.f),
			FMath::Clamp(PendingMoveRight, -1.f, 1.f)
		);
	}

	if (DodgeInput.IsNearlyZero())
	{
		return BackDashMontage ? BackDashMontage.Get() : DodgeMontage.Get();
	}

	if (!bLockOnActive)
	{
		return DodgeMontage.Get();
	}

	const float ForwardAbs = FMath::Abs(DodgeInput.X);
	const float RightAbs = FMath::Abs(DodgeInput.Y);

	// Lock-on dodge now uses 4-direction selection only.
	// If both axes are pressed, left/right wins on ties so diagonal input prefers side dodge.
	if (RightAbs >= ForwardAbs)
	{
		return DodgeInput.Y >= 0.f
			? (LockOnDodgeRightMontage ? LockOnDodgeRightMontage.Get() : DodgeMontage.Get())
			: (LockOnDodgeLeftMontage ? LockOnDodgeLeftMontage.Get() : DodgeMontage.Get());
	}

	return DodgeInput.X >= 0.f
		? DodgeMontage.Get()
		: (LockOnDodgeBackMontage ? LockOnDodgeBackMontage.Get() : DodgeMontage.Get());
}

float AJunPlayer::GetDodgePlayRateForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return 1.f;
	}

	if (Montage == BackDashMontage)
	{
		return FMath::Max(BackDashPlayRate, KINDA_SMALL_NUMBER);
	}

	if (Montage == LockOnDodgeBackMontage)
	{
		return FMath::Max(BackwardDodgePlayRate, KINDA_SMALL_NUMBER);
	}

	if (Montage == LockOnDodgeLeftMontage)
	{
		return FMath::Max(LeftDodgePlayRate, KINDA_SMALL_NUMBER);
	}

	if (Montage == LockOnDodgeRightMontage)
	{
		return FMath::Max(RightDodgePlayRate, KINDA_SMALL_NUMBER);
	}

	return FMath::Max(ForwardDodgePlayRate, KINDA_SMALL_NUMBER);
}

float AJunPlayer::GetDodgeRecoveryCancelOpenTimeForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return ForwardDodgeRecoveryCancelOpenTime;
	}

	if (Montage == BackDashMontage)
	{
		return BackDashRecoveryCancelOpenTime;
	}

	if (Montage == LockOnDodgeBackMontage)
	{
		return BackwardDodgeRecoveryCancelOpenTime;
	}

	if (Montage == LockOnDodgeLeftMontage || Montage == LockOnDodgeRightMontage)
	{
		return SideDodgeRecoveryCancelOpenTime;
	}

	return ForwardDodgeRecoveryCancelOpenTime;
}

float AJunPlayer::GetDodgeMoveCancelOpenTimeForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return ForwardDodgeMoveCancelOpenTime;
	}

	if (Montage == BackDashMontage)
	{
		return BackDashMoveCancelOpenTime;
	}

	if (Montage == LockOnDodgeBackMontage)
	{
		return BackwardDodgeMoveCancelOpenTime;
	}

	if (Montage == LockOnDodgeLeftMontage || Montage == LockOnDodgeRightMontage)
	{
		return SideDodgeMoveCancelOpenTime;
	}

	return ForwardDodgeMoveCancelOpenTime;
}

float AJunPlayer::GetCurrentDodgeTimelineRate() const
{
	return FMath::Max(CurrentDodgePlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::TryChooseFakeDeathDie()
{
	if (!IsWaitingForFakeDeathChoice())
	{
		return false;
	}

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->HideFakeDeathUI();
	}
	ConfirmRealDeathFromFakeDeath();
	return true;
}

bool AJunPlayer::TryChooseFakeDeathRevive()
{
	if (!IsWaitingForFakeDeathChoice() || CurrentReviveCount <= 0)
	{
		return false;
	}

	StartFakeDeathRevive();
	return true;
}

void AJunPlayer::EnterFakeDeath()
{
	DeathState = EJunPlayerDeathState::FakeDeath;
	Hp = 0;
	bDeathPresentationStarted = false;
	ApplyDeathControlLock();
	NotifyBossPlayerFakeDeathStarted();

	if (FakeDeathMontage)
	{
		PlayAnimMontage(FakeDeathMontage);
	}
	bPendingDeathPresentationIsRealDeath = false;
}

void AJunPlayer::EnterRealDeath()
{
	DeathState = EJunPlayerDeathState::RealDeath;
	Hp = 0;
	RealDeathHoldRemainTime = 0.f;
	RealDeathFullBlackHoldRemainTime = -1.f;
	bRealDeathBlackFadeStarted = false;
	bDeathPresentationStarted = false;
	AddGameplayTag(JunGameplayTags::State_Condition_Dead);
	ApplyDeathControlLock();
	NotifyBossPlayerRealDeathStarted();
	StopFakeDeathSound(false);

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	bPendingDeathPresentationIsRealDeath = true;
}

void AJunPlayer::ConfirmRealDeathFromFakeDeath()
{
	DeathState = EJunPlayerDeathState::RealDeath;
	Hp = 0;
	RealDeathHoldRemainTime = 0.f;
	RealDeathFullBlackHoldRemainTime = -1.f;
	bRealDeathBlackFadeStarted = false;
	bPendingDeathPresentationIsRealDeath = true;
	bDeathPresentationStarted = false;
	AddGameplayTag(JunGameplayTags::State_Condition_Dead);
	ApplyDeathControlLock();
	NotifyBossPlayerRealDeathStarted();
	StopFakeDeathSound(false);

	// Already lying in FakeDeath's Death loop, so there is no new death-section notify to wait for.
	TriggerPendingDeathPresentation();
}

void AJunPlayer::TriggerPendingDeathPresentation()
{
	if (DeathState != EJunPlayerDeathState::FakeDeath &&
		DeathState != EJunPlayerDeathState::RealDeath)
	{
		return;
	}

	if (bDeathPresentationStarted)
	{
		return;
	}

	bDeathPresentationStarted = true;
	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->SetLockOnMarkerSuppressed(true);

		if (bPendingDeathPresentationIsRealDeath)
		{
			RealDeathHoldRemainTime = RealDeathUIHoldDuration;
			StartRealDeathSound();
			JunPlayerController->ShowRealDeathUI();
		}
		else
		{
			StartFakeDeathSound();
			JunPlayerController->ShowFakeDeathUI();
		}
	}
}

void AJunPlayer::StartFakeDeathRevive()
{
	DeathState = EJunPlayerDeathState::FakeDeathReviving;
	CurrentReviveCount = FMath::Max(0, CurrentReviveCount - 1);
	bDeathPresentationStarted = false;
	StopFakeDeathSound();

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->HideFakeDeathUI();
	}

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnFakeDeathGetUpMontageEnded);
		PlayerAnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnFakeDeathGetUpMontageEnded);
	}

	if (!FakeDeathGetUpMontage || PlayAnimMontage(FakeDeathGetUpMontage) <= 0.f)
	{
		FinishFakeDeathRevive();
	}
}

void AJunPlayer::FinishFakeDeathRevive()
{
	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnFakeDeathGetUpMontageEnded);
	}

	Hp = FMath::Clamp(FMath::RoundToInt(static_cast<float>(MaxHp) * FakeDeathReviveHealthRatio), 1, MaxHp);
	DeathState = EJunPlayerDeathState::Alive;
	CurrentPosture = 0.f;
	bDeathPresentationStarted = false;
	ClearDeathControlLock();
	EndLockOn();
	NotifyBossPlayerRevivedFromFakeDeath();

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->SetLockOnMarkerSuppressed(false);
		JunPlayerController->HideDeathUI();
	}
}

void AJunPlayer::UpdateDeathState(float DeltaTime)
{
	if (DeathState != EJunPlayerDeathState::RealDeath)
	{
		return;
	}

	if (!bDeathPresentationStarted)
	{
		return;
	}

	if (RealDeathHoldRemainTime > 0.f)
	{
		RealDeathHoldRemainTime = FMath::Max(0.f, RealDeathHoldRemainTime - DeltaTime);
		return;
	}

	AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController());
	if (!bRealDeathBlackFadeStarted)
	{
		bRealDeathBlackFadeStarted = true;
		StopRealDeathSound();
		if (JunPlayerController)
		{
			JunPlayerController->StartDeathFullBlackFadeIn();
		}
		return;
	}

	if (JunPlayerController && JunPlayerController->IsDeathFullBlackOpaque())
	{
		if (RealDeathFullBlackHoldRemainTime < 0.f)
		{
			RealDeathFullBlackHoldRemainTime = RealDeathFullBlackHoldDuration;
			return;
		}

		if (RealDeathFullBlackHoldRemainTime > 0.f)
		{
			RealDeathFullBlackHoldRemainTime = FMath::Max(0.f, RealDeathFullBlackHoldRemainTime - DeltaTime);
			return;
		}

		DeathState = EJunPlayerDeathState::Respawning;
		ResetBossesAfterPlayerRealDeath();
		if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
		{
			BGMManager->SetBGMDucked(false);
			BGMManager->ReturnToMapBGM();
		}
		RespawnAtPlayerStart();
	}
}

void AJunPlayer::StartFakeDeathSound()
{
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->SetBGMDucked(true);
	}

	if (!FakeDeathSound || !FakeDeathAudioComponent)
	{
		return;
	}

	if (FakeDeathAudioComponent->IsPlaying())
	{
		return;
	}

	FakeDeathAudioComponent->SetSound(FakeDeathSound);
	if (FakeDeathSoundFadeInTime > 0.f)
	{
		FakeDeathAudioComponent->FadeIn(FakeDeathSoundFadeInTime, FakeDeathSoundVolume);
	}
	else
	{
		FakeDeathAudioComponent->SetVolumeMultiplier(FakeDeathSoundVolume);
		FakeDeathAudioComponent->Play();
	}
}

void AJunPlayer::StopFakeDeathSound(bool bReleaseBGMDuck)
{
	if (bReleaseBGMDuck)
	{
		if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
		{
			BGMManager->SetBGMDucked(false);
		}
	}

	if (!FakeDeathAudioComponent || !FakeDeathAudioComponent->IsPlaying())
	{
		return;
	}

	if (FakeDeathSoundFadeOutTime > 0.f)
	{
		FakeDeathAudioComponent->FadeOut(FakeDeathSoundFadeOutTime, 0.f);
	}
	else
	{
		FakeDeathAudioComponent->Stop();
	}
}

void AJunPlayer::StartRealDeathSound()
{
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->SetBGMDucked(true);
	}

	if (!RealDeathSound || !RealDeathAudioComponent)
	{
		return;
	}

	if (RealDeathAudioComponent->IsPlaying())
	{
		return;
	}

	RealDeathAudioComponent->SetSound(RealDeathSound);
	if (RealDeathSoundFadeInTime > 0.f)
	{
		RealDeathAudioComponent->FadeIn(RealDeathSoundFadeInTime, RealDeathSoundVolume);
	}
	else
	{
		RealDeathAudioComponent->SetVolumeMultiplier(RealDeathSoundVolume);
		RealDeathAudioComponent->Play();
	}
}

void AJunPlayer::StopRealDeathSound()
{
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->SetBGMDucked(false);
	}

	if (!RealDeathAudioComponent || !RealDeathAudioComponent->IsPlaying())
	{
		return;
	}

	if (RealDeathSoundFadeOutTime > 0.f)
	{
		RealDeathAudioComponent->FadeOut(RealDeathSoundFadeOutTime, 0.f);
	}
	else
	{
		RealDeathAudioComponent->Stop();
	}
}

void AJunPlayer::ApplyDeathControlLock()
{
	InterruptActionsForHitReaction();
	CancelLockOnTurn();
	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::ClearDeathControlLock()
{
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	RemoveGameplayTag(JunGameplayTags::State_Condition_Dead);
}

void AJunPlayer::NotifyBossPlayerFakeDeathStarted()
{
	for (TActorIterator<AJunMonster> It(GetWorld()); It; ++It)
	{
		It->EnterPlayerDeathWait();
	}
}

void AJunPlayer::NotifyBossPlayerRevivedFromFakeDeath()
{
	for (TActorIterator<AJunMonster> It(GetWorld()); It; ++It)
	{
		It->ResumeAfterPlayerFakeDeath();
	}
}

void AJunPlayer::NotifyBossPlayerRealDeathStarted()
{
	for (TActorIterator<AJunMonster> It(GetWorld()); It; ++It)
	{
		It->EnterPlayerDeathWait();
	}
}

void AJunPlayer::ResetBossesAfterPlayerRealDeath()
{
	for (TActorIterator<AJunMonster> It(GetWorld()); It; ++It)
	{
		It->ResetAfterPlayerRealDeath();
	}
}

void AJunPlayer::RespawnAtPlayerStart()
{
	StopFakeDeathSound();
	StopRealDeathSound();

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		if (FakeDeathMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, FakeDeathMontage);
		}
		if (DeathMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, DeathMontage);
		}
		if (FakeDeathGetUpMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, FakeDeathGetUpMontage);
		}
	}

	APlayerStart* PlayerStart = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			PlayerStart = *It;
			break;
		}
	}

	const FRotator SpawnViewRotation = PlayerStart
		? PlayerStart->GetActorRotation()
		: GetActorRotation();

	if (PlayerStart)
	{
		SetActorLocationAndRotation(
			PlayerStart->GetActorLocation(),
			SpawnViewRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	Hp = MaxHp;
	CurrentReviveCount = FMath::Max(0, MaxReviveCount);
	CurrentPosture = 0.f;
	DeathState = EJunPlayerDeathState::Alive;
	RealDeathHoldRemainTime = 0.f;
	RealDeathFullBlackHoldRemainTime = -1.f;
	bRealDeathBlackFadeStarted = false;
	bDeathPresentationStarted = false;
	EndLockOn();
	ResetCameraToSpawnView(SpawnViewRotation);
	StartCameraHardSnap(3);
	ClearDeathControlLock();

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->SetLockOnMarkerSuppressed(false);
		JunPlayerController->HideDeathUI();
	}
}

void AJunPlayer::OnFakeDeathGetUpMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != FakeDeathGetUpMontage)
	{
		return;
	}

	FinishFakeDeathRevive();
}

void AJunPlayer::UpdateDefenseInput(float DeltaTime)
{
	const float DefenseTimelineDeltaTime = DeltaTime * GetCurrentDefenseTimelineRate();

	// Tick owns the delayed movement release after GuardEnd.
	if (GuardMoveBlendRemainTime > 0.f)
	{
		GuardMoveBlendRemainTime = FMath::Max(0.f, GuardMoveBlendRemainTime - DeltaTime);
	}

	if (GuardExitMoveReleaseRemainTime > 0.f)
	{
		GuardExitMoveReleaseRemainTime = FMath::Max(0.f, GuardExitMoveReleaseRemainTime - DeltaTime);

		if (GuardExitMoveReleaseRemainTime <= 0.f)
		{
			bDeferGuardMoveInput = false;
			DesiredMoveForward = PendingMoveForward;
			DesiredMoveRight = PendingMoveRight;
			ClearDefenseBlockTags();
		}
	}

	if (DefenseState == EJunDefenseState::Starting || DefenseState == EJunDefenseState::Ending)
	{
		DefenseTransitionElapsedTime += DefenseTimelineDeltaTime;

		if (DefenseState == EJunDefenseState::Ending &&
			BufferedDefenseTransitionCancelAction != EJunBufferedDefenseCancelAction::None)
		{
			TryExecuteBufferedDefenseTransitionCancelAction();
		}
	}
	else
	{
		DefenseTransitionElapsedTime = 0.f;
	}

	if (DefenseState == EJunDefenseState::Starting && !bDeflectResolveReceived)
	{
		if (bHoldRequestedForCurrentDeflect)
		{
			CurrentDeflectHeldDuration += DeltaTime;
		}

		DeflectResolveRemainTime = FMath::Max(0.f, DeflectResolveRemainTime - DefenseTimelineDeltaTime);

		if (DeflectResolveRemainTime <= 0.f)
		{
			ResolveCurrentDeflectAttempt();
		}
	}

	if (DefenseState == EJunDefenseState::Ending && GuardEndFinishRemainTime > 0.f)
	{
		if (GuardEndBaseReleaseRemainTime > 0.f)
		{
			GuardEndBaseReleaseRemainTime = FMath::Max(0.f, GuardEndBaseReleaseRemainTime - DefenseTimelineDeltaTime);

			if (GuardEndBaseReleaseRemainTime <= 0.f)
			{
				bKeepGuardBaseWhileEnding = false;
			}
		}

		GuardEndFinishRemainTime = FMath::Max(0.f, GuardEndFinishRemainTime - DefenseTimelineDeltaTime);

		if (GuardEndFinishRemainTime <= 0.f)
		{
			DesiredMoveForward = 0.f;
			DesiredMoveRight = 0.f;
			PendingMoveForward = 0.f;
			PendingMoveRight = 0.f;

			if (BufferedDefenseTransitionCancelAction != EJunBufferedDefenseCancelAction::None)
			{
				TryExecuteBufferedDefenseTransitionCancelAction();
			}
			else
			{
				FinishDefense();
			}

			return;
		}
	}

	if (DefenseState == EJunDefenseState::None && PostDefenseTransitionCancelBufferRemainTime > 0.f)
	{
		PostDefenseTransitionCancelBufferRemainTime = FMath::Max(0.f, PostDefenseTransitionCancelBufferRemainTime - DeltaTime);

		if (BufferedDefenseTransitionCancelAction != EJunBufferedDefenseCancelAction::None)
		{
			ExecuteBufferedDefenseTransitionCancelAction();
			return;
		}
	}

	if (bParryWindowOpen)
	{
		ParryWindowRemainTime = FMath::Max(0.f, ParryWindowRemainTime - DeltaTime);

		if (ParryWindowRemainTime <= 0.f)
		{
			bParryWindowOpen = false;
		}
	}

}

EJunPlayerHitResolveResult AJunPlayer::ResolveIncomingHitResult(EHitReactType IncomingHitType) const
{
	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	if (bParryWindowOpen)
	{
		const float PerfectParryRemainThreshold =
			FMath::Max(0.f, DefaultParryWindowDuration - PerfectParryWindowDuration);
		return ParryWindowRemainTime >= PerfectParryRemainThreshold
			? EJunPlayerHitResolveResult::PerfectParrySuccess
			: EJunPlayerHitResolveResult::NormalParrySuccess;
	}

	if (DefenseState == EJunDefenseState::Guarding)
	{
		return EJunPlayerHitResolveResult::GuardBlock;
	}

	if (!CanBeInterruptedBy(IncomingHitType))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	return EJunPlayerHitResolveResult::HitReact;
}

bool AJunPlayer::CanBeInterruptedBy(EHitReactType IncomingHitType) const
{
	auto IsHeavyHitType = [](const EHitReactType HitType)
	{
		return HitType == EHitReactType::HeavyHit_A
			|| HitType == EHitReactType::HeavyHit_B
			|| HitType == EHitReactType::HeavyHit_C;
	};

	auto IsLargeHitType = [](const EHitReactType HitType)
	{
		return HitType == EHitReactType::LargeHit_Short
			|| HitType == EHitReactType::LargeHit_Long;
	};

	if (CurrentHitState == EJunPlayerHitState::None)
	{
		return true;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		return !bParryWindowOpen;
	}

	if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		return DefenseState == EJunDefenseState::Guarding;
	}

	if (CurrentHitState == EJunPlayerHitState::GuardBreak)
	{
		return true;
	}

	if (CurrentHitState != EJunPlayerHitState::HitReact)
	{
		return true;
	}

	if (IsHeavyHitType(CurrentHitReactType) &&
		IncomingHitType == EHitReactType::LightHit)
	{
		return false;
	}

	if (IsHeavyHitType(IncomingHitType) || IsLargeHitType(IncomingHitType))
	{
		return true;
	}

	if (CurrentHitReactType == EHitReactType::LightHit &&
		IncomingHitType == EHitReactType::LightHit)
	{
		return true;
	}

	return CurrentHitReactType == EHitReactType::None;
}

ECharacterHitReactDirection AJunPlayer::DetermineHitReactDirection(const AActor* DamageCauser, const FVector& SwingDirection) const
{
	auto ResolveFrontSwingDirection = [this, &SwingDirection]()
	{
		const FVector SafeSwingDirection = SwingDirection.GetSafeNormal();
		if (SafeSwingDirection.IsNearlyZero())
		{
			return ECharacterHitReactDirection::Front_F;
		}

		const FVector LocalSwingDirection = GetActorTransform().InverseTransformVectorNoScale(SafeSwingDirection);
		const float SwingYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalSwingDirection.Y, LocalSwingDirection.X));

		if (SwingYawDegrees > 25.f && SwingYawDegrees <= 70.f)
		{
			return ECharacterHitReactDirection::Front_FR;
		}
		if (SwingYawDegrees < -25.f && SwingYawDegrees >= -70.f)
		{
			return ECharacterHitReactDirection::Front_FL;
		}

		return ECharacterHitReactDirection::Front_F;
	};

	if (!DamageCauser)
	{
		return ResolveFrontSwingDirection();
	}

	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(DamageCauser->GetActorLocation() - GetActorLocation());
	ToAttackerLocal.Z = 0.f;

	if (ToAttackerLocal.IsNearlyZero())
	{
		return ResolveFrontSwingDirection();
	}

	const float AttackerYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(ToAttackerLocal.Y, ToAttackerLocal.X));

	if (AttackerYawDegrees > 60.f && AttackerYawDegrees <= 135.f)
	{
		return ECharacterHitReactDirection::Right_R;
	}
	if (AttackerYawDegrees < -60.f && AttackerYawDegrees >= -135.f)
	{
		return ECharacterHitReactDirection::Left_L;
	}
	if (AttackerYawDegrees > 135.f || AttackerYawDegrees < -135.f)
	{
		return ECharacterHitReactDirection::Back_B;
	}

	return ResolveFrontSwingDirection();
}

ECharacterKnockbackDirection AJunPlayer::DetermineKnockbackDirectionFromDamageCauser(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return ECharacterKnockbackDirection::Backward;
	}

	const FVector ToAttackerWorld = DamageCauser->GetActorLocation() - GetActorLocation();
	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(ToAttackerWorld);
	ToAttackerLocal.Z = 0.f;

	if (ToAttackerLocal.IsNearlyZero())
	{
		return ECharacterKnockbackDirection::Backward;
	}

	if (FMath::Abs(ToAttackerLocal.Y) > FMath::Abs(ToAttackerLocal.X))
	{
		return ToAttackerLocal.Y > 0.f
			? ECharacterKnockbackDirection::Left
			: ECharacterKnockbackDirection::Right;
	}

	return ToAttackerLocal.X > 0.f
		? ECharacterKnockbackDirection::Backward
		: ECharacterKnockbackDirection::Forward;
}

UAnimMontage* AJunPlayer::GetHitReactMontage(EHitReactType HitType, ECharacterHitReactDirection HitDirection) const
{
	auto ResolveDirectionalMontage = [HitDirection](
		UAnimMontage* BackMontage,
		UAnimMontage* FrontMontage,
		UAnimMontage* FrontLeftMontage,
		UAnimMontage* FrontRightMontage,
		UAnimMontage* LeftMontage,
		UAnimMontage* RightMontage) -> UAnimMontage*
	{
		switch (HitDirection)
		{
		case ECharacterHitReactDirection::Back_B:
			return BackMontage ? BackMontage : FrontMontage;
		case ECharacterHitReactDirection::Front_FL:
			return FrontLeftMontage ? FrontLeftMontage : (LeftMontage ? LeftMontage : FrontMontage);
		case ECharacterHitReactDirection::Front_FR:
			return FrontRightMontage ? FrontRightMontage : (RightMontage ? RightMontage : FrontMontage);
		case ECharacterHitReactDirection::Left_L:
			return LeftMontage ? LeftMontage : (FrontLeftMontage ? FrontLeftMontage : FrontMontage);
		case ECharacterHitReactDirection::Right_R:
			return RightMontage ? RightMontage : (FrontRightMontage ? FrontRightMontage : FrontMontage);
		case ECharacterHitReactDirection::Front_F:
		default:
			return FrontMontage;
		}
	};

	switch (HitType)
	{
	case EHitReactType::LightHit:
		return ResolveDirectionalMontage(
			LightHitBackMontage,
			LightHitFront_FMontage,
			LightHitFront_FLMontage,
			LightHitFront_FRMontage,
			LightHitLeftMontage,
			LightHitRightMontage
		);
	case EHitReactType::HeavyHit_A:
		if (HitDirection == ECharacterHitReactDirection::Back_B ||
			HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ResolveDirectionalMontage(
				LargeHitBackMontage,
				LargeHitFrontMontage,
				LargeHitFrontLeftMontage,
				LargeHitFrontRightMontage,
				LargeHitLeftMontage,
				LargeHitRightMontage
			);
		}
		return HeavyHitFront_AMontage;
	case EHitReactType::HeavyHit_B:
		if (HitDirection == ECharacterHitReactDirection::Back_B ||
			HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ResolveDirectionalMontage(
				LargeHitBackMontage,
				LargeHitFrontMontage,
				LargeHitFrontLeftMontage,
				LargeHitFrontRightMontage,
				LargeHitLeftMontage,
				LargeHitRightMontage
			);
		}
		return HeavyHitFront_BMontage;
	case EHitReactType::HeavyHit_C:
		if (HitDirection == ECharacterHitReactDirection::Back_B ||
			HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ResolveDirectionalMontage(
				LargeHitBackMontage,
				LargeHitFrontMontage,
				LargeHitFrontLeftMontage,
				LargeHitFrontRightMontage,
				LargeHitLeftMontage,
				LargeHitRightMontage
			);
		}
		return HeavyHitFront_CMontage;
	case EHitReactType::LargeHit_Short:
	case EHitReactType::LargeHit_Long:
		return ResolveDirectionalMontage(
			LargeHitBackMontage,
			LargeHitFrontMontage,
			LargeHitFrontLeftMontage,
			LargeHitFrontRightMontage,
			LargeHitLeftMontage,
			LargeHitRightMontage
		);
	default:
		return nullptr;
	}
}

UAnimMontage* AJunPlayer::GetParrySuccessMontage(const FVector& SwingDirection)
{
	if (!bHasSelectedInitialParrySuccessSide)
	{
		bNextParrySuccessUsesLeftSide = FMath::RandBool();
		bHasSelectedInitialParrySuccessSide = true;
	}

	const bool bUseLeft = bNextParrySuccessUsesLeftSide;
	bNextParrySuccessUsesLeftSide = !bNextParrySuccessUsesLeftSide;
	const bool bUseUp = FMath::RandBool();

	if (bUseLeft)
	{
		return bUseUp
			? (ParrySuccessL_UpMontage ? ParrySuccessL_UpMontage : ParrySuccessL_DownMontage)
			: (ParrySuccessL_DownMontage ? ParrySuccessL_DownMontage : ParrySuccessL_UpMontage);
	}

	return bUseUp
		? (ParrySuccessR_UpMontage ? ParrySuccessR_UpMontage : ParrySuccessR_DownMontage)
		: (ParrySuccessR_DownMontage ? ParrySuccessR_DownMontage : ParrySuccessR_UpMontage);
}

bool AJunPlayer::AddPosture(float Amount)
{
	if (GuardBreakVulnerableRemainTime > 0.f)
	{
		return IsPostureFull();
	}

	if (MaxPosture <= 0.f || Amount <= 0.f)
	{
		return IsPostureFull();
	}

	CurrentPosture = FMath::Clamp(CurrentPosture + Amount, 0.f, MaxPosture);
	PostureRecoveryDelayRemainTime = PostureRecoveryDelay;
	return IsPostureFull();
}

bool AJunPlayer::IsPostureFull() const
{
	return MaxPosture > 0.f && CurrentPosture >= MaxPosture;
}

void AJunPlayer::UpdatePostureRecovery(float DeltaTime)
{
	if (CurrentPosture <= 0.f || !CanRecoverPosture())
	{
		return;
	}

	if (PostureRecoveryDelayRemainTime > 0.f)
	{
		PostureRecoveryDelayRemainTime = FMath::Max(0.f, PostureRecoveryDelayRemainTime - DeltaTime);
		return;
	}

	const bool bGuardRecoveryBoost = DefenseState == EJunDefenseState::Guarding;
	const float GuardMultiplier = bGuardRecoveryBoost ? GuardPostureRecoveryMultiplier : 1.f;
	const float RecoveryAmount = PostureRecoveryRate * GuardMultiplier * DeltaTime;
	CurrentPosture = FMath::Max(0.f, CurrentPosture - RecoveryAmount);
}

bool AJunPlayer::CanRecoverPosture() const
{
	if (Is_Dead() || bIsExecuting)
	{
		return false;
	}

	if (CurrentHitState != EJunPlayerHitState::None)
	{
		return false;
	}

	if (bIsBasicAttacking || bIsHeavyAttacking || bIsJumpAttacking || bIsDodgeAttacking)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return false;
	}

	return GetMoveState() != EJunMoveState::Run;
}

void AJunPlayer::StartParrySuccess()
{
	InterruptActionsForHitReaction();
	PlayDefenseEffect(CachedParryParticleComponent, ParryParticleComponentName);

	CurrentHitState = EJunPlayerHitState::ParrySuccess;
	ChainParryWindowRemainTime = ChainParryWindowDuration;
	ParrySuccessElapsedTime = 0.f;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	ApplyCommonKnockback(
		LastIncomingKnockbackDirection,
		LastIncomingDefenseKnockbackData.ParrySuccess.Strength,
		LastIncomingDefenseKnockbackData.ParrySuccess.BrakingDeceleration,
		LastIncomingDefenseKnockbackData.ParrySuccess.GroundFriction,
		LastIncomingDefenseKnockbackData.ParrySuccess.BrakingFrictionFactor,
		LastIncomingDefenseKnockbackData.ParrySuccess.OverrideDuration
	);

	UAnimMontage* ParrySuccessMontage = GetParrySuccessMontage(LastIncomingSwingDirection);
	CurrentParrySuccessMontage = ParrySuccessMontage;
	PlayerHitStateRemainTime = ParrySuccessMontage ? ParrySuccessMontage->GetPlayLength() : ParrySuccessDuration;

	if (ParrySuccessMontage)
	{
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.f, ParrySuccessMontage);
		}

		PlayAnimMontage(ParrySuccessMontage);
	}
}

void AJunPlayer::StartGuardBlock()
{
	PlayDefenseEffect(CachedDeflectParticleComponent, DeflectParticleComponentName);

	CurrentHitState = EJunPlayerHitState::GuardBlock;
	PlayerHitStateRemainTime = GuardBlockMontage ? GuardBlockMontage->GetPlayLength() : GuardBlockDuration;
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	ApplyCommonKnockback(
		LastIncomingKnockbackDirection,
		LastIncomingDefenseKnockbackData.GuardBlock.Strength,
		LastIncomingDefenseKnockbackData.GuardBlock.BrakingDeceleration,
		LastIncomingDefenseKnockbackData.GuardBlock.GroundFriction,
		LastIncomingDefenseKnockbackData.GuardBlock.BrakingFrictionFactor,
		LastIncomingDefenseKnockbackData.GuardBlock.OverrideDuration
	);

	if (GuardBlockMontage)
	{
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.f, GuardBlockMontage);
		}

		PlayAnimMontage(GuardBlockMontage);
	}
}

void AJunPlayer::StartGuardBreak()
{
	InterruptActionsForHitReaction();
	PlayDefenseEffect(CachedGuardBreakParticleComponent, GuardBreakParticleComponentName);

	CurrentPosture = 0.f;
	CurrentHitState = EJunPlayerHitState::GuardBreak;
	PlayerHitStateRemainTime = GuardBreakMontage ? GuardBreakMontage->GetPlayLength() : GuardBreakDuration;
	PlayerHitControlLockRemainTime = GuardBreakControlLockDuration;
	GuardBreakVulnerableRemainTime = GuardBreakControlLockDuration;
	ChainParryWindowRemainTime = 0.f;
	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
	ParrySuccessElapsedTime = 0.f;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	CurrentParrySuccessMontage = nullptr;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		FVector NewVelocity = MovementComponent->Velocity;
		NewVelocity.X = 0.f;
		NewVelocity.Y = 0.f;
		MovementComponent->Velocity = NewVelocity;
	}
	ConsumeMovementInputVector();

	AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->PlayPlayerPostureBreakGlow();
	}

	if (GuardBreakMontage)
	{
		PlayAnimMontage(GuardBreakMontage);
	}
}

void AJunPlayer::StartHitReact(EHitReactType HitType, ECharacterHitReactDirection HitDirection)
{
	InterruptActionsForHitReaction();

	CurrentHitState = EJunPlayerHitState::HitReact;
	CurrentHitReactType = HitType;
	CurrentHitReactDirection = HitDirection;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	bFreeRunSlideActive = false;
	FreeRunSlideSpeed = 0.f;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		FVector NewVelocity = MovementComponent->Velocity;
		NewVelocity.X = 0.f;
		NewVelocity.Y = 0.f;
		MovementComponent->Velocity = NewVelocity;
	}
	ConsumeMovementInputVector();

	UAnimMontage* HitReactMontage = GetHitReactMontage(HitType, HitDirection);
	if (HitType == EHitReactType::LightHit)
	{
		PlayerHitStateRemainTime = LightHitReactDuration;
		PlayerHitControlLockRemainTime = LightHitControlLockDuration;
	}
	else if (HitType == EHitReactType::HeavyHit_A ||
		HitType == EHitReactType::HeavyHit_B ||
		HitType == EHitReactType::HeavyHit_C)
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : HeavyHitReactDuration;
		switch (HitType)
		{
		case EHitReactType::HeavyHit_A:
			PlayerHitControlLockRemainTime = HeavyHitAControlLockDuration;
			break;
		case EHitReactType::HeavyHit_B:
			PlayerHitControlLockRemainTime = HeavyHitBControlLockDuration;
			break;
		case EHitReactType::HeavyHit_C:
			PlayerHitControlLockRemainTime = HeavyHitCControlLockDuration;
			break;
		default:
			PlayerHitControlLockRemainTime = HeavyHitAControlLockDuration;
			break;
		}
	}
	else if (HitType == EHitReactType::LargeHit_Short)
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : LargeHitReactDuration;
		PlayerHitControlLockRemainTime = LargeHitControlLockDuration;
	}
	else if (HitType == EHitReactType::LargeHit_Long)
	{
		PlayerHitStateRemainTime = LargeHitLongReactDuration;
		PlayerHitControlLockRemainTime = LargeHitLongControlLockDuration;
	}
	else
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : HitReactDuration;
		PlayerHitControlLockRemainTime = HitReactDuration;
	}

	if (GuardBreakVulnerableRemainTime > 0.f)
	{
		PlayerHitControlLockRemainTime = FMath::Max(PlayerHitControlLockRemainTime, GuardBreakVulnerableRemainTime);
	}

	if (!DoesMontageUseRootMotion(HitReactMontage))
	{
		ApplyCommonKnockback(
			LastIncomingKnockbackDirection,
			LastIncomingDefenseKnockbackData.HitReact.Strength,
			LastIncomingDefenseKnockbackData.HitReact.BrakingDeceleration,
			LastIncomingDefenseKnockbackData.HitReact.GroundFriction,
			LastIncomingDefenseKnockbackData.HitReact.BrakingFrictionFactor,
			LastIncomingDefenseKnockbackData.HitReact.OverrideDuration
		);
	}

	AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);

	if (HitReactMontage)
	{
		if (AnimInstance)
		{
			//AnimInstance->Montage_Stop(0.f, HitReactMontage);
		}

		PlayAnimMontage(HitReactMontage);
	}
}

void AJunPlayer::ApplyCommonKnockback(
	ECharacterKnockbackDirection KnockbackDirectionType,
	float Strength,
	float BrakingDecelerationOverride,
	float GroundFrictionOverride,
	float BrakingFrictionFactorOverride,
	float OverrideDuration
)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || Strength <= 0.f)
	{
		return;
	}

	FVector KnockbackDirection = FVector::ZeroVector;
	switch (KnockbackDirectionType)
	{
	case ECharacterKnockbackDirection::Forward:
		KnockbackDirection = GetActorForwardVector();
		break;
	case ECharacterKnockbackDirection::Backward:
		KnockbackDirection = -GetActorForwardVector();
		break;
	case ECharacterKnockbackDirection::Left:
		KnockbackDirection = -GetActorRightVector();
		break;
	case ECharacterKnockbackDirection::Right:
		KnockbackDirection = GetActorRightVector();
		break;
	default:
		KnockbackDirection = -GetActorForwardVector();
		break;
	}

	KnockbackDirection.Z = 0.f;
	KnockbackDirection = KnockbackDirection.GetSafeNormal();
	if (KnockbackDirection.IsNearlyZero())
	{
		return;
	}

	KnockbackBrakingDecelerationOverride = BrakingDecelerationOverride;
	KnockbackGroundFrictionOverride = GroundFrictionOverride;
	KnockbackBrakingFrictionFactorOverride = BrakingFrictionFactorOverride;
	KnockbackBrakingOverrideRemainTime = OverrideDuration;

	MovementComponent->AddImpulse(KnockbackDirection * Strength, true);
}

void AJunPlayer::InterruptActionsForHitReaction()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		FinishDodgeState();
	}

	if (bIsBasicAttacking)
	{
		CancelBasicAttackForRecoveryTransition();
	}

	if (bIsHeavyAttacking)
	{
		if (AnimInstance && CurrentHeavyAttackMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnHeavyAttackMontageEnded);
			AnimInstance->Montage_Stop(0.05f, CurrentHeavyAttackMontage);
		}

		FinishHeavyAttack();
	}

	if (bIsJumpAttacking)
	{
		if (AnimInstance && JumpAttackMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
			AnimInstance->Montage_Stop(0.05f, JumpAttackMontage);
		}

		FinishJumpAttack();
	}

	if (bIsDodgeAttacking)
	{
		if (AnimInstance && CurrentDodgeAttackMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
			AnimInstance->Montage_Stop(0.05f, CurrentDodgeAttackMontage);
		}

		FinishDodgeAttack();
	}

	if (DefenseState != EJunDefenseState::None)
	{
		if (AnimInstance)
		{
			if (GuardStartMontage)
			{
				AnimInstance->Montage_Stop(0.05f, GuardStartMontage);
			}

			if (GuardEndMontage)
			{
				AnimInstance->Montage_Stop(0.05f, GuardEndMontage);
			}
		}

		FinishDefenseForCancel();
	}

	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::UpdatePlayerHitState(float DeltaTime)
{
	if (ChainParryWindowRemainTime > 0.f)
	{
		ChainParryWindowRemainTime = FMath::Max(0.f, ChainParryWindowRemainTime - DeltaTime);
	}

	if (CurrentHitState == EJunPlayerHitState::None)
	{
		return;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		ParrySuccessElapsedTime += DeltaTime;

		if (BufferedParrySuccessCancelAction != EJunBufferedParrySuccessCancelAction::None)
		{
			TryExecuteBufferedParrySuccessCancelAction();
			if (CurrentHitState == EJunPlayerHitState::None)
			{
				return;
			}
		}
	}

	if (PlayerHitControlLockRemainTime > 0.f)
	{
		PlayerHitControlLockRemainTime = FMath::Max(0.f, PlayerHitControlLockRemainTime - DeltaTime);
		if (PlayerHitControlLockRemainTime <= 0.f)
		{
			ReleaseHitReactControlLock();
		}
	}

	PlayerHitStateRemainTime = FMath::Max(0.f, PlayerHitStateRemainTime - DeltaTime);

	if (PlayerHitStateRemainTime <= 0.f)
	{
		FinishPlayerHitState();
	}
}

void AJunPlayer::UpdateGuardBreakVulnerability(float DeltaTime)
{
	if (GuardBreakVulnerableRemainTime <= 0.f)
	{
		return;
	}

	GuardBreakVulnerableRemainTime = FMath::Max(0.f, GuardBreakVulnerableRemainTime - DeltaTime);
}

void AJunPlayer::ReleaseHitReactControlLock()
{
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	if (CurrentHitState == EJunPlayerHitState::HitReact)
	{
		if (TryCancelHitReactIntoMove())
		{
			return;
		}

		return;
	}

	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
}

bool AJunPlayer::TryCancelHitReactIntoMove()
{
	if (CurrentHitState != EJunPlayerHitState::HitReact ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	const bool bHasMoveInput =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight);
	if (!bHasMoveInput)
	{
		return false;
	}

	if (AnimInstance)
	{
		if (UAnimMontage* HitReactMontage = GetHitReactMontage(CurrentHitReactType, CurrentHitReactDirection))
		{
			AnimInstance->Montage_Stop(HitReactMoveCancelBlendOutTime, HitReactMontage);
		}
	}

	CurrentHitState = EJunPlayerHitState::None;
	PlayerHitStateRemainTime = 0.f;
	PlayerHitControlLockRemainTime = 0.f;
	ChainParryWindowRemainTime = 0.f;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;

	if (GetCharacterMovement())
	{
		RestoreDefaultMovementBrakingSettings();
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	return true;
}

void AJunPlayer::FinishPlayerHitState()
{
	CurrentHitState = EJunPlayerHitState::None;
	PlayerHitStateRemainTime = 0.f;
	PlayerHitControlLockRemainTime = 0.f;
	ChainParryWindowRemainTime = 0.f;
	ParrySuccessElapsedTime = 0.f;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	CurrentParrySuccessMontage = nullptr;

	if (GetCharacterMovement())
	{
		RestoreDefaultMovementBrakingSettings();
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	ReleaseHitReactControlLock();

	if (DefenseState == EJunDefenseState::Guarding)
	{
		ApplyGuardOnBlockTags();
	}
	else if (DefenseState == EJunDefenseState::Starting || DefenseState == EJunDefenseState::Ending)
	{
		ApplyDefenseStartBlockTags();
	}
}

void AJunPlayer::StartDefenseSequence()
{
	// Every tap/press starts a fresh deflect attempt from frame 0 of GuardStart.
	const bool bIsImmediateRestartCycle =
		DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Ending;

	CurrentDefensePlayRate = FMath::Max(GuardStartPlayRate, KINDA_SMALL_NUMBER);
	bParryWindowOpen = true;
	ParryWindowRemainTime = DefaultParryWindowDuration;
	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	bHoldRequestedForCurrentDeflect = bDefenseButtonHeld;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime =
		(GuardStartMontage ? GuardStartMontage->GetPlayLength() : DefaultDeflectResolveTime);
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	bKeepGuardBaseWhileEnding = false;
	GuardExitMoveReleaseRemainTime = 0.f;
	bDeferGuardMoveInput = true;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;

	ApplyDefenseStartBlockTags();

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);

		if (GuardEndMontage)
		{
			AnimInstance->Montage_Stop(0.05f, GuardEndMontage);
		}

		if (GuardStartMontage)
		{
			AnimInstance->Montage_Stop(0.02f, GuardStartMontage);
		}
	}

	DefenseState = EJunDefenseState::Starting;

	if (!AnimInstance || !GuardStartMontage)
	{
		if (bDefenseButtonHeld)
		{
			EnterGuardLoop();
		}
		else
		{
			BeginGuardEnd();
		}
		return;
	}

	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
	AnimInstance->Montage_Play(GuardStartMontage, CurrentDefensePlayRate);
}

void AJunPlayer::BeginDefenseFromBufferedInput()
{
	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return;
	}

	StartDefenseSequence();
}

void AJunPlayer::ResolveCurrentDeflectAttempt()
{
	if (DefenseState != EJunDefenseState::Starting || bDeflectResolveReceived)
	{
		return;
	}

	bDeflectResolveReceived = true;

	const bool bShouldEnterGuardLoop =
		bHoldRequestedForCurrentDeflect &&
		CurrentDeflectHeldDuration >= GuardEnterHoldThreshold;

	if (bShouldEnterGuardLoop)
	{
		EnterGuardLoop();
		return;
	}
	BeginGuardEnd();
}

void AJunPlayer::ApplyDefenseStartBlockTags()
{
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::ApplyGuardOnBlockTags()
{
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::ClearDefenseBlockTags()
{
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void AJunPlayer::EnterGuardLoop()
{
	// Guarding is the only sustained block state. Start/End remain montage-driven.
	DefenseState = EJunDefenseState::Guarding;
	CurrentDefensePlayRate = 1.f;
	bParryWindowOpen = false;
	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	bDeferGuardMoveInput = false;
	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;
	GuardMoveBlendRemainTime = GuardMoveBlendDuration;
	ApplyGuardOnBlockTags();
}

void AJunPlayer::BeginGuardEnd()
{
	// Ending keeps the guard base, but movement should already be available once
	// GuardStart has fully resolved. Only non-move defense blocks remain here.
	if (DefenseState == EJunDefenseState::None || DefenseState == EJunDefenseState::Ending)
	{
		return;
	}

	DefenseState = EJunDefenseState::Ending;
	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	CurrentDefensePlayRate = FMath::Max(GuardEndPlayRate, KINDA_SMALL_NUMBER);
	GuardEndFinishRemainTime =
		GuardEndMontage
			? FMath::Max(0.f, GuardEndMontage->GetPlayLength() - GuardEndFinishTimeOffset)
			: 0.f;
	GuardEndBaseReleaseRemainTime =
		GuardEndMontage
			? FMath::Max(0.f, GuardEndMontage->GetPlayLength() - GuardEndBaseReleaseTimeOffset)
			: 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	bKeepGuardBaseWhileEnding = true;
	bDeferGuardMoveInput = true;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	GuardMoveBlendRemainTime = GuardMoveBlendDuration;
	ApplyDefenseStartBlockTags();

	if (!AnimInstance || !GuardEndMontage)
	{
		FinishDefense();
		return;
	}

	const bool bEndingGuardWhileMoving =
		!FMath::IsNearlyZero(PendingMoveForward) ||
		!FMath::IsNearlyZero(PendingMoveRight) ||
		!FMath::IsNearlyZero(DesiredMoveForward) ||
		!FMath::IsNearlyZero(DesiredMoveRight);
	const float GuardStartBlendOutTime = bEndingGuardWhileMoving ? GuardStartToEndBlendOutTime : 0.f;
	const float GuardEndBlendInTimeToUse = bEndingGuardWhileMoving ? GuardEndBlendInTime : 0.f;
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
	AnimInstance->Montage_Stop(GuardStartBlendOutTime, GuardStartMontage);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);

	const FMontageBlendSettings GuardEndBlendSettings(GuardEndBlendInTimeToUse);
	if (AnimInstance->Montage_PlayWithBlendSettings(GuardEndMontage, GuardEndBlendSettings, CurrentDefensePlayRate) <= 0.f)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
		FinishDefense();
		return;
	}

	if (BufferedDefenseTransitionCancelAction != EJunBufferedDefenseCancelAction::None)
	{
		TryExecuteBufferedDefenseTransitionCancelAction();
	}
}

void AJunPlayer::FinishDefense()
{
	DefenseState = EJunDefenseState::None;
	CurrentDefensePlayRate = 1.f;
	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	bKeepGuardBaseWhileEnding = false;
	bDeferGuardMoveInput = true;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = PostDefenseTransitionCancelBufferDuration;
	CurrentDeflectHeldDuration = 0.f;
	GuardMoveBlendRemainTime = GuardExitMoveBlendDuration;
	GuardExitMoveReleaseRemainTime = GuardExitMoveReleaseDelay;
}

float AJunPlayer::GetCurrentDefenseTimelineRate() const
{
	return FMath::Max(CurrentDefensePlayRate, KINDA_SMALL_NUMBER);
}

void AJunPlayer::OnGuardStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != GuardStartMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
	}

	if (DefenseState != EJunDefenseState::Starting)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	ResolveCurrentDeflectAttempt();
}

void AJunPlayer::OnGuardEndMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != GuardEndMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
	}

	if (BufferedDefenseTransitionCancelAction != EJunBufferedDefenseCancelAction::None)
	{
		TryExecuteBufferedDefenseTransitionCancelAction();
		return;
	}

	if (GuardEndFinishRemainTime <= 0.f)
	{
		FinishDefense();
	}
}

void AJunPlayer::ApplyRunningStateToAnimInstance(bool bNewIsRunning)
{
	if (!AnimInstance)
	{
		AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), TEXT("bIsRunning")))
	{
		BoolProp->SetPropertyValue_InContainer(AnimInstance, bNewIsRunning);
	}

	if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), TEXT("bUseRunLocomotion")))
	{
		BoolProp->SetPropertyValue_InContainer(AnimInstance, bNewIsRunning);
	}
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

		if (IsSameTeam(Candidate))
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

	if (!TargetActor)
	{
		TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();
		if (!TargetActor)
		{
			EndAttackFacingWindow();
			return;
		}
	}

	FVector Direction = TargetActor->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = Direction.Rotation();
	const float ResolvedFacingInterpSpeed = AttackFacingWindowInterpSpeed > 0.f ? AttackFacingWindowInterpSpeed : 30.f;

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		ResolvedFacingInterpSpeed
	);

	SetActorRotation(NewRotation);
}

void AJunPlayer::BeginAttackFacingWindow(float FacingInterpSpeed)
{
	TargetActor = bIsExecuting && CurrentExecutionTarget
		? CurrentExecutionTarget.Get()
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
		const float TargetArmLength = CameraMode == EJunCameraMode::Death
			? (SpringArm ? SpringArm->TargetArmLength : DefaultSpringArmLength)
			: GetPitchAdjustedSpringArmLength();
		const bool bShouldShortenArm = TargetArmLength < SpringArm->TargetArmLength;

		SpringArm->TargetArmLength = FMath::FInterpTo(
			SpringArm->TargetArmLength,
			TargetArmLength,
			DeltaTime,
			bShouldShortenArm ? PitchArmShortenInterpSpeed : GetCurrentExecutionCameraArmLengthRestoreInterpSpeed()
		);

		if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, TargetArmLength, 1.f))
		{
			bExecutionCameraRestoreUsesFinishTuning = false;
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
		TargetFOV = LockOnCameraFOV;
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
	if (!Controller)
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FVector2D Input = PendingCameraLookInput;

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

	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AJunPlayer::EndLockOn()
{
	bLockOnActive = false;
	LockOnTarget = nullptr;
	CameraMode = EJunCameraMode::Free;
	CachedLockOnTargetPoint = FVector::ZeroVector;
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

void AJunPlayer::UpdateLockOnCamera(float DeltaTime)
{
	if (!Controller || !LockOnTarget)
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	FVector CameraBasePoint = CameraAnchor
		? CameraAnchor->GetComponentLocation()
		: GetActorLocation();
	CameraBasePoint.Z += 50.f;

	const FVector TargetPoint = GetFilteredLockOnTargetPoint();
	const FVector ToTarget = TargetPoint - CameraBasePoint;
	const FVector DebugSpherePoint = GetLockOnDebugSpherePoint();

	//DrawDebugSphere(GetWorld(), DebugSpherePoint, 6.f, 12, FColor::Red, false, 0.f);

	if (ToTarget.IsNearlyZero())
	{
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
	}

	const float Distance2D = FVector(ToTarget.X, ToTarget.Y, 0.f).Length();
	const float DeltaZ = ToTarget.Z;

	FRotator DesiredRot = ToTarget.Rotation();
	DesiredRot.Roll = 0.f;

	const float TargetPitchDeg = FMath::RadiansToDegrees(
		FMath::Atan2(DeltaZ, FMath::Max(Distance2D, 1.f))
	);

	const float LockOnDistanceAlpha = FMath::Clamp(
		Distance2D / FMath::Max(LockOnAcquireDistance, 1.f),
		0.f,
		1.f
	);
	const float DynamicLockOnPitchOffset = FMath::Lerp(
		LockOnClosePitchOffset,
		LockOnPitchOffset,
		LockOnDistanceAlpha
	);

	DesiredRot.Pitch = DynamicLockOnPitchOffset + TargetPitchDeg;

	const FRotator CurrentRot = Controller->GetControlRotation();

	if (GetCharacterMovement()->IsFalling())
	{
		DesiredRot.Pitch = FMath::Lerp(CurrentRot.Pitch, DesiredRot.Pitch, 0.35f);
	}

	DesiredRot.Pitch = FMath::Clamp(DesiredRot.Pitch, MinLockOnCameraPitch, MaxLockOnCameraPitch);

	FRotator NewRot = FMath::RInterpTo(
		CurrentRot,
		DesiredRot,
		DeltaTime,
		LockOnRotationInterpSpeed
	);

	Controller->SetControlRotation(NewRot);
	TargetCameraYaw = NewRot.Yaw;
	TargetCameraPitch = NewRot.Pitch;
	PendingCameraLookInput = FVector2D::ZeroVector;
}

void AJunPlayer::StartExecutionCamera(AJunMonster* Monster)
{
	if (!Monster)
	{
		return;
	}

	CameraModeBeforeExecution = CameraMode;
	ExecutionCameraTarget = Monster;
	bCurrentExecutionIsFinish = Monster->IsFinalExecution();
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
	if (!Controller || !ExecutionCameraTarget)
	{
		EndExecutionCamera();
		return;
	}

	FVector CameraBasePoint = CameraAnchor
		? CameraAnchor->GetComponentLocation()
		: GetActorLocation();
	CameraBasePoint.Z += 50.f;

	const FVector TargetPoint = ExecutionCameraTarget->GetLockOnTargetPoint();
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

	const FRotator NewRot = FMath::RInterpTo(
		CurrentRot,
		DesiredRot,
		DeltaTime,
		LockOnCharacterRotationInterpSpeed
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
		const FVector TargetLockOnPoint = LockOnTarget->GetLockOnTargetPoint();
		RawPoint.Z = TargetLockOnPoint.Z;
	}
	
	if (CachedLockOnTargetPoint.IsNearlyZero())
	{
		CachedLockOnTargetPoint = RawPoint;
		return CachedLockOnTargetPoint;
	}

	const FVector2D CachedXY(CachedLockOnTargetPoint.X, CachedLockOnTargetPoint.Y);
	const FVector2D RawXY(RawPoint.X, RawPoint.Y);

	const float XYDelta = FVector2D::Distance(CachedXY, RawXY);
	const float ZDelta = FMath::Abs(RawPoint.Z - CachedLockOnTargetPoint.Z);

	if (XYDelta >= LockOnTargetXYIgnoreThreshold)
	{
		CachedLockOnTargetPoint.X = RawPoint.X;
		CachedLockOnTargetPoint.Y = RawPoint.Y;
	}

	if (ZDelta >= LockOnTargetZIgnoreThreshold)
	{
		const float ZInterpSpeed = RawPoint.Z > CachedLockOnTargetPoint.Z
			? LockOnTargetZRiseInterpSpeed
			: LockOnTargetZFallInterpSpeed;

		const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
		CachedLockOnTargetPoint.Z = FMath::FInterpTo(
			CachedLockOnTargetPoint.Z,
			RawPoint.Z,
			DeltaTime,
			ZInterpSpeed
		);
	}

	return CachedLockOnTargetPoint;
}




