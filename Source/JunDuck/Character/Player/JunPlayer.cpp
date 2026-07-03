#include "Character/Player/JunPlayer.h"
#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/JunSpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Character/Player/PlayerComponent/JunPlayerActionStateComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerPotionComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Equipment/HatActor.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunExecutionTargetInterface.h"
#include "Interface/JunAttackTargetInterface.h"
#include "Interface/JunDefenseReactionTargetInterface.h"
#include "Interface/JunJumpCounterTargetInterface.h"
#include "Interface/JunLockOnTargetInterface.h"
#include "Interface/JunMikiriCounterTargetInterface.h"
#include "Interface/JunPlayerCombatTargetInterface.h"
#include "Interface/JunTutorialTargetInterface.h"
#include "PlayerController/JunPlayerController.h"
#include "Weapon/WeaponActor.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Level/JunPlayerRespawnPoint.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "System/JunBGMManager.h"
#include "System/JunCombatVFXSubsystem.h"
#include "System/JunTimeEffectSubsystem.h"
#include "UObject/UnrealType.h"

namespace
{
	bool DoesMontageUseRootMotion(const UAnimMontage* Montage)
	{
		return Montage && Montage->HasRootMotion();
	}
}

#pragma region Construction / Setup
// ============================================================================
// Construction / Setup
// ============================================================================

AJunPlayer::AJunPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	HitDamageSoundVolume = 2.f;
	PlayerAttackHitStopDuration = 0.02f;
	bEnableDefenseHitStop = false;
	
	SetupMeshAndCollision();
	SetupCameraComponents();
	SetupMovementDefaults();

	EquipmentComponent = CreateDefaultSubobject<UJunPlayerEquipmentComponent>(TEXT("EquipmentComponent"));
	PotionComponent = CreateDefaultSubobject<UJunPlayerPotionComponent>(TEXT("PotionComponent"));
	ExecutionComponent = CreateDefaultSubobject<UJunPlayerExecutionComponent>(TEXT("ExecutionComponent"));
	DefenseComponent = CreateDefaultSubobject<UJunPlayerDefenseComponent>(TEXT("DefenseComponent"));
	CombatReactionComponent = CreateDefaultSubobject<UJunPlayerCombatReactionComponent>(TEXT("CombatReactionComponent"));
	ActionStateComponent = CreateDefaultSubobject<UJunPlayerActionStateComponent>(TEXT("ActionStateComponent"));

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
	SpringArm->SocketOffset = FVector(0.f, 25.f, 0.f);
	SpringArm->TargetOffset = FVector::ZeroVector;
	SpringArm->bUsePawnControlRotation = true;

	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;

	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = false;
	SpringArm->bDoCollisionTest = true;
	SpringArm->ProbeSize = 35.f;
	SpringArm->ProbeChannel = ECC_Camera;
	SpringArm->CameraLagSpeed = 8.f;
	SpringArm->CameraLagMaxDistance = 0.f;
	SpringArm->bClampToMaxPhysicsDeltaTime = false;
	SpringArm->PrimaryComponentTick.bStartWithTickEnabled = true;
	SpringArm->PrimaryComponentTick.TickInterval = 0.f;
	
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
	MovementComponent->RotationRate = FRotator(0.f, FreeRunMaxTurnRotationRate, 0.f);
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
	RefillDrinkPotionCharges();
	SpawnAndAttachWeapon();
	SpawnAndAttachScabbard();
	SpawnAndAttachDefaultHat();
	GetPlayerAnimInstance();
	CacheDefenseEffectComponents();
	CacheDefaultMovementBrakingSettings();
	PrimeSoundForFirstPlayback(FakeDeathSound);
	PrimeSoundForFirstPlayback(RealDeathSound);
	PrimeSoundForFirstPlayback(ExecutionStartSound);
	PrimeSoundArrayForFirstPlayback(PreloadSFXSounds);
	ResetCameraToSpawnView(GetActorRotation(), SpawnCameraPitch);
	StartCameraHardSnap();

	if (CameraAnchor)
	{
		CameraAnchor->SetWorldLocation(GetCapsuleComponent()->GetComponentLocation());
	}
}

#pragma endregion


#pragma region Main Tick
// ============================================================================
// Main Tick
// ============================================================================

void AJunPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TryInitializeCameraRotationFromController();
	UpdateLocomotionState();
	UpdateMoveInputState();
	UpdateCameraHardSnap();
	const float CameraDeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : DeltaTime;
	UpdateCameraAnchor(CameraDeltaTime);
	UpdateJunCamera(CameraDeltaTime);
	UpdateCameraProximityVisibility();
	if (LandingAnimSuppressRemainTime > 0.f)
	{
		LandingAnimSuppressRemainTime = FMath::Max(0.f, LandingAnimSuppressRemainTime - DeltaTime);
	}
	if (LocomotionLandingRemainTime > 0.f)
	{
		LocomotionLandingRemainTime = FMath::Max(0.f, LocomotionLandingRemainTime - DeltaTime);
	}

	if (DeathState != EJunPlayerDeathState::Alive)
	{
		CancelDrinkPotionHeal();
		UpdateAirHeavyHitReact(DeltaTime);
		UpdateDeathState(DeltaTime);
		return;
	}

	ValidateLockOnTarget();
	UpdateFakeDeathReviveAutoLockOn(DeltaTime);

	UpdateCharacterRotationForCurrentCameraMode(DeltaTime);
	UpdateHitReactFacing(DeltaTime);

	UpdateMovementSpeed(DeltaTime);
	UpdateDrinkPotionHeal(DeltaTime);

	ApplyRunningStateToAnimInstance(ShouldUseRunningAnim());

	UpdateBasicAttackJumpAndMoveCancelState(DeltaTime);

	UpdateBasicAttackRecoveryBuffer(DeltaTime);
	UpdateHeavyAttackInput(DeltaTime);
	if (bIsJigenAttacking)
	{
		JigenSectionElapsedTime += DeltaTime * FMath::Max(JigenPlayRate, KINDA_SMALL_NUMBER);
		TryProcessBufferedJigenRecoveryAction();
	}
	UpdateJumpCounterStompOpportunity();
	UpdateJumpCounterStompFollowUp(DeltaTime);
	UpdateJumpAttackState(DeltaTime);
	if (bIsDodgeAttacking)
	{
		DodgeAttackElapsedTime += DeltaTime * GetCurrentDodgeAttackTimelineRate();

		// A defense press before the dodge-attack cancel window is a hold buffer.
		// Consume it only while the physical input is still held, so a released tap
		// cannot leak into a later dodge and start guard unexpectedly.
		const bool bDefenseHeld = DefenseComponent
			? DefenseComponent->IsDefenseButtonHeld()
			: false;
		if (bDefenseHeld)
		{
			TryCancelDodgeAttackIntoDefense();
		}
	}

	UpdateDodgeState(DeltaTime);

	UpdateMikiriCounterWindow(DeltaTime);
	UpdateDefenseInput(DeltaTime);

	UpdateGuardBreakVulnerability(DeltaTime);
	UpdatePlayerHitState(DeltaTime);
	UpdatePostureRecovery(DeltaTime);

	UpdateJumpStartAnimTrigger(DeltaTime);

	DrawDefenseTimingDebug();
}

#pragma endregion


















