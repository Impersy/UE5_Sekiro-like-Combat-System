#include "Character/JunPlayer.h"
#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/JunSpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Character/JunMonster.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Equipment/HatActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/JunPlayerController.h"
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

AJunPlayer::AJunPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	HitDamageSoundVolume = 2.f;
	PlayerAttackHitStopDuration = 0.02f;
	bEnableDefenseHitStop = false;
	
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
	CachedParryEffectLocationComponent = FindSceneComponentByName(ParryEffectLocationComponentName);
	CachedDeflectEffectLocationComponent = FindSceneComponentByName(DeflectEffectLocationComponentName);
	CachedGuardBreakEffectLocationComponent = FindSceneComponentByName(GuardBreakEffectLocationComponentName);
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

USceneComponent* AJunPlayer::FindSceneComponentByName(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == ComponentName)
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void AJunPlayer::PlayDefenseEffect(
	TObjectPtr<UNiagaraComponent>& CachedComponent,
	FName ComponentName,
	TObjectPtr<USceneComponent>& CachedLocationComponent,
	FName LocationComponentName)
{
	if (!CachedComponent)
	{
		CachedComponent = FindNiagaraComponentByName(ComponentName);
	}

	if (!CachedLocationComponent)
	{
		CachedLocationComponent = FindSceneComponentByName(LocationComponentName);
	}

	UNiagaraSystem* NiagaraAsset = CachedComponent ? CachedComponent->GetAsset() : nullptr;
	if (!NiagaraAsset)
	{
		return;
	}

	if (UJunCombatVFXSubsystem* VFXSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UJunCombatVFXSubsystem>() : nullptr)
	{
		const USceneComponent* SpawnLocationComponent = CachedLocationComponent ? CachedLocationComponent.Get() : GetMesh();
		VFXSubsystem->SpawnCombatNiagaraAtComponent(NiagaraAsset, SpawnLocationComponent);
	}
}

void AJunPlayer::PlayDefenseCameraShake(float Scale)
{
	if (!DefenseCameraShakeClass || Scale <= 0.f)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PlayerController)
	{
		PlayerController->ClientStartCameraShake(
			DefenseCameraShakeClass,
			Scale,
			ECameraShakePlaySpace::CameraLocal,
			FRotator::ZeroRotator);
	}
}

void AJunPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TryInitializeCameraRotationFromController();
	UpdateCameraHardSnap();
	const float CameraDeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : DeltaTime;
	UpdateCameraAnchor(CameraDeltaTime);
	UpdateJunCamera(CameraDeltaTime);
	if (LandingAnimSuppressRemainTime > 0.f)
	{
		LandingAnimSuppressRemainTime = FMath::Max(0.f, LandingAnimSuppressRemainTime - DeltaTime);
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
		TryExecuteBufferedJigenRecoveryAction();
	}
	UpdateJumpCounterStompOpportunity();
	UpdateJumpCounterStompFollowUp(DeltaTime);
	UpdateJumpAttackState(DeltaTime);
	if (bIsDodgeAttacking)
	{
		DodgeAttackElapsedTime += DeltaTime * GetCurrentDodgeAttackTimelineRate();
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

void AJunPlayer::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	if (Damage <= 0 || DeathState != EJunPlayerDeathState::Alive)
	{
		return;
	}

	const int32 PreviousHp = Hp;
	Hp = FMath::Clamp(Hp - Damage, 0, MaxHp);
	if (PreviousHp > Hp)
	{
		PlayHitDamageSound();
		FinishDrinkPotion(true);
	}
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

bool AJunPlayer::TryStartDrinkPotion()
{
	if (!CanStartDrinkPotion())
	{
		return false;
	}

	UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance();
	if (!PlayerAnimInstance || !DrinkMontage)
	{
		return false;
	}

	bIsDrinkingPotion = true;
	bDrinkPotionSavedWalkRequested = bWalkRequested;
	SetWalkRequested(true);
	HideWeaponForDrinkPotion();

	PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDrinkPotionMontageEnded);
	PlayerAnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnDrinkPotionMontageEnded);

	const float PlayResult = PlayerAnimInstance->Montage_PlayWithBlendIn(
		DrinkMontage,
		FAlphaBlendArgs(FMath::Max(0.f, DrinkPotionBlendInTime)),
		FMath::Max(DrinkPotionPlayRate, KINDA_SMALL_NUMBER));
	if (PlayResult <= 0.f)
	{
		FinishDrinkPotion(true);
		return false;
	}

	CurrentDrinkPotionCharges = FMath::Max(0, CurrentDrinkPotionCharges - 1);
	ApplyDrinkPotionHeal();
	return true;
}

bool AJunPlayer::CanStartDrinkPotion() const
{
	if (!DrinkMontage || CurrentDrinkPotionCharges <= 0)
	{
		return false;
	}

	if (bIsDrinkingPotion ||
		DeathState != EJunPlayerDeathState::Alive ||
		Is_Dead() ||
		bIsExecuting ||
		IsInDeathSequence() ||
		CurrentHitState != EJunPlayerHitState::None)
	{
		return false;
	}

	if (bIsBasicAttacking ||
		bIsHeavyAttacking ||
		bIsJigenAttacking ||
		bIsJumpAttacking ||
		bIsDodgeAttacking ||
		DefenseState != EJunDefenseState::None ||
		HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked) ||
		HasGameplayTag(JunGameplayTags::State_Block_Input))
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent && MovementComponent->IsFalling())
	{
		return false;
	}

	return true;
}

void AJunPlayer::RefillDrinkPotionCharges()
{
	CurrentDrinkPotionCharges = FMath::Max(0, MaxDrinkPotionCharges);
}

void AJunPlayer::ApplyDrinkPotionHeal()
{
	if (MaxHp <= 0 || DrinkPotionHealRatio <= 0.f)
	{
		return;
	}

	const int32 HealAmount = FMath::RoundToInt(static_cast<float>(MaxHp) * DrinkPotionHealRatio);
	if (HealAmount <= 0)
	{
		return;
	}

	if (DrinkPotionHealDuration <= 0.f)
	{
		Hp = FMath::Clamp(Hp + HealAmount, 0, MaxHp);
		return;
	}

	DrinkPotionPendingHealAmount += HealAmount;
	DrinkPotionHealRemainTime = FMath::Max(DrinkPotionHealRemainTime, DrinkPotionHealDuration);
}

void AJunPlayer::UpdateDrinkPotionHeal(float DeltaTime)
{
	if (DrinkPotionHealRemainTime <= 0.f || DrinkPotionPendingHealAmount <= 0.f)
	{
		return;
	}

	if (Hp >= MaxHp)
	{
		CancelDrinkPotionHeal();
		return;
	}

	const float StepRatio = DrinkPotionHealRemainTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(DeltaTime / DrinkPotionHealRemainTime, 0.f, 1.f)
		: 1.f;
	const float StepHeal = DrinkPotionPendingHealAmount * StepRatio;
	DrinkPotionPendingHealAmount = FMath::Max(0.f, DrinkPotionPendingHealAmount - StepHeal);
	DrinkPotionHealRemainTime = FMath::Max(0.f, DrinkPotionHealRemainTime - DeltaTime);

	DrinkPotionHealAccumulator += StepHeal;
	const int32 HealToApply = FMath::FloorToInt(DrinkPotionHealAccumulator);
	if (HealToApply > 0)
	{
		const int32 PreviousHp = Hp;
		Hp = FMath::Clamp(Hp + HealToApply, 0, MaxHp);
		DrinkPotionHealAccumulator -= static_cast<float>(Hp - PreviousHp);
	}

	if (DrinkPotionHealRemainTime <= 0.f && DrinkPotionPendingHealAmount > 0.f)
	{
		const int32 FinalHealToApply = FMath::CeilToInt(DrinkPotionPendingHealAmount + DrinkPotionHealAccumulator);
		if (FinalHealToApply > 0)
		{
			Hp = FMath::Clamp(Hp + FinalHealToApply, 0, MaxHp);
		}
		CancelDrinkPotionHeal();
	}
}

void AJunPlayer::CancelDrinkPotionHeal()
{
	DrinkPotionHealRemainTime = 0.f;
	DrinkPotionPendingHealAmount = 0.f;
	DrinkPotionHealAccumulator = 0.f;
}

void AJunPlayer::HideWeaponForDrinkPotion()
{
	bDrinkPotionWeaponHidden = false;
	bDrinkPotionSavedWeaponHidden = false;

	if (!EquippedWeapon)
	{
		return;
	}

	bDrinkPotionSavedWeaponHidden = EquippedWeapon->IsHidden();
	EquippedWeapon->SetActorHiddenInGame(true);
	EquippedWeapon->SetWeaponEffectsEnabled(false);
	bDrinkPotionWeaponHidden = true;
}

void AJunPlayer::RestoreDrinkPotionWeaponVisibility()
{
	if (!bDrinkPotionWeaponHidden)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(bDrinkPotionSavedWeaponHidden);
		EquippedWeapon->SetWeaponEffectsEnabled(!bDrinkPotionSavedWeaponHidden);
	}

	bDrinkPotionWeaponHidden = false;
}

void AJunPlayer::FinishDrinkPotion(bool bInterrupted)
{
	if (bInterrupted)
	{
		CancelDrinkPotionHeal();
	}

	if (!bIsDrinkingPotion && !bDrinkPotionWeaponHidden)
	{
		return;
	}

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDrinkPotionMontageEnded);
	if (bInterrupted && DrinkMontage && PlayerAnimInstance->Montage_IsPlaying(DrinkMontage))
		{
			PlayerAnimInstance->Montage_Stop(FMath::Max(0.f, DrinkPotionInterruptBlendOutTime), DrinkMontage);
		}
	}

	RestoreDrinkPotionWeaponVisibility();
	if (bIsDrinkingPotion)
	{
		SetWalkRequested(bDrinkPotionSavedWalkRequested);
	}

	bIsDrinkingPotion = false;
}

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

	const float MaxTurnRate = FMath::Max(0.f, FreeRunMaxTurnRotationRate);
	if (CameraMode != EJunCameraMode::Free ||
		bLockOnActive ||
		!MovementComponent->bOrientRotationToMovement ||
		!Controller)
	{
		MovementComponent->RotationRate = FRotator(0.f, MaxTurnRate, 0.f);
		return;
	}

	FVector2D MoveInput(DesiredMoveForward, DesiredMoveRight);
	if (MoveInput.IsNearlyZero())
	{
		MoveInput = FVector2D(PendingMoveForward, PendingMoveRight);
	}

	if (MoveInput.IsNearlyZero())
	{
		MovementComponent->RotationRate = FRotator(0.f, MaxTurnRate, 0.f);
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Pitch = 0.f;
	ControlRotation.Roll = 0.f;

	const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(ControlRotation);
	const FVector RightDirection = UKismetMathLibrary::GetRightVector(ControlRotation);
	const FVector DesiredWorldDirection = ((ForwardDirection * MoveInput.X) + (RightDirection * MoveInput.Y)).GetSafeNormal2D();
	if (DesiredWorldDirection.IsNearlyZero())
	{
		MovementComponent->RotationRate = FRotator(0.f, MaxTurnRate, 0.f);
		return;
	}

	const float TurnAngle = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredWorldDirection.Rotation().Yaw));
	const float AngleAlpha = FMath::Clamp(TurnAngle / 180.f, 0.f, 1.f);
	const float CurvedAlpha = FMath::Pow(AngleAlpha, FMath::Max(0.01f, FreeRunTurnAngleExponent));
	const float TurnRate = FMath::Lerp(
		FMath::Max(0.f, FreeRunMinTurnRotationRate),
		MaxTurnRate,
		CurvedAlpha
	);

	MovementComponent->RotationRate = FRotator(0.f, TurnRate, 0.f);
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

	if (bIsJigenAttacking)
	{
		if (!CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction::Jump))
		{
			BufferJigenRecoveryAction(EJunBufferedRecoveryAction::Jump);
			return;
		}

		CancelJigenForRecoveryTransition(JigenJumpCancelBlendOutTime);
	}

	if (CurrentHitState == EJunPlayerHitState::HitReact && !TryCancelHitReactIntoJump())
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Block_Jump))
	{
		return;
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

	if (bJumpCounterStompFollowUpActive)
	{
		FinishJumpCounterStompFollowUp(false);
	}

	if (bAirParrySequenceActive)
	{
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

			if (AirGuardStartMontage)
			{
				AnimInstance->Montage_Stop(AirGuardLandCancelBlendOutTime, AirGuardStartMontage);
			}

			if (AirGuardEndMontage)
			{
				AnimInstance->Montage_Stop(AirGuardLandCancelBlendOutTime, AirGuardEndMontage);
			}
		}

		FinishDefense();
	}

	if (CurrentHitState == EJunPlayerHitState::HitReact &&
		(AirHeavyHitStage == EJunAirHeavyHitStage::Launch ||
			AirHeavyHitStage == EJunAirHeavyHitStage::Down))
	{
		StartAirHeavyHitLandStage();
		bJumpCounterFollowUpDefenseBypassAvailable = false;
		bJumpCounterStompFollowUpAvailable = false;
		JumpCounterStompTarget = nullptr;
		return;
	}

	if (CurrentHitState == EJunPlayerHitState::GuardBreak && bAirGuardBreakLandMontagePending)
	{
		StartAirGuardBreakLandMontage();
		bJumpCounterFollowUpDefenseBypassAvailable = false;
		bJumpCounterStompFollowUpAvailable = false;
		JumpCounterStompTarget = nullptr;
		return;
	}

	if (bIsJumpAttacking)
	{
		RequestJumpAttackEnd();
		return;
	}

	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	JumpCounterStompTarget = nullptr;
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
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Jigen_ComboWindow))
	{
		OnJigenComboWindowBegin();
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

void AJunPlayer::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, const FJunAttackDefenseRuleData& DefenseRuleData, EJunWeaponNiagaraComponent NiagaraComponent, const FJunAttackTraceOverrideData& TraceOverrideData)
{
	if (!EquippedWeapon)
	{
		return;
	}

	FJunAttackDefenseRuleData ResolvedDefenseRuleData = DefenseRuleData;
	if (bJumpCounterFollowUpDefenseBypassAvailable && bIsJumpAttacking)
	{
		ResolvedDefenseRuleData.bCanBeParried = false;
	}

	Super::BeginAttackTraceWindow(HitReactType, DamageData, DefenseKnockbackData, ResolvedDefenseRuleData, NiagaraComponent, TraceOverrideData);

	EquippedWeapon->SetAttackHitReactType(HitReactType);
	EquippedWeapon->SetAttackDamageData(DamageData);
	EquippedWeapon->SetAttackDefenseKnockbackData(DefenseKnockbackData);
	EquippedWeapon->SetAttackDefenseRuleData(ResolvedDefenseRuleData);
	EquippedWeapon->ApplyAttackTraceOverride(TraceOverrideData);
	EquippedWeapon->StartAttackTrace(NiagaraComponent);
}

void AJunPlayer::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
	Super::EndAttackTraceWindow(NiagaraComponent);

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
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

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
	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		BeginHeavyAttackHoldInput();
		BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::HeavyAttack);
		return;
	}

	if (CanBufferDefenseTransitionCancel())
	{
		BeginHeavyAttackHoldInput();
		BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::HeavyAttack);
		return;
	}

	if (bIsDodgeAttacking)
	{
		if (!TryCancelDodgeAttackIntoHeavyAttack())
		{
			return;
		}

		BeginHeavyAttackHoldInput();
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

		BeginHeavyAttackHoldInput();
		return;
	}

	if (bIsBasicAttacking)
	{
		if (!TryCancelBasicAttackIntoHeavyAttack())
		{
			return;
		}

		BeginHeavyAttackHoldInput();
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
				BeginHeavyAttackHoldInput();
				return;
			}

			BeginHeavyAttackHoldInput();
			bBufferedHeavyAttackInput = true;
			return;
		}

		if (TryCancelHeavyAttackIntoHeavyAttack())
		{
			BeginHeavyAttackHoldInput();
		}
		return;
	}

	if (bIsJigenAttacking)
	{
		if (!CanCancelJigenIntoHeavyAttack())
		{
			return;
		}

		CancelJigenForRecoveryTransition(JigenHeavyAttackCancelBlendOutTime);
		bHeavyAttackInputHeld = true;
		HeavyAttackInputHoldTime = FMath::Max(HeavyAttackInputHoldTime, 0.f);
		HeavyAttackChargeLoopElapsedTime = 0.f;
		if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
		{
			StartHeavyAttackCharge();
		}
		return;
	}

	BeginHeavyAttackHoldInput();
}

void AJunPlayer::OnHeavyAttackReleased()
{
	bParrySuccessHeavyAttackInputHeld = false;

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

	if (bIsJigenAttacking)
	{
		if (!CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction::Dodge))
		{
			BufferJigenRecoveryAction(EJunBufferedRecoveryAction::Dodge);
			return;
		}

		CancelJigenForRecoveryTransition(JigenDodgeCancelBlendOutTime);
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
	if (IsForwardDashMontage(DodgeMontageToPlay))
	{
		DodgeInternalCooldownRemainTime = DashForwardInternalCooldownDuration;
		DodgeFinishRemainTime = DashForwardFinishTime;
		DodgeInvincibleRemainTime = DashForwardInvincibleDuration;
	}
	else if (IsBackDashMontage(DodgeMontageToPlay))
	{
		DodgeInternalCooldownRemainTime = DashBackInternalCooldownDuration;
		DodgeFinishRemainTime = DashBackFinishTime;
		DodgeInvincibleRemainTime = DashBackInvincibleDuration;
	}
	else if (IsSideDashMontage(DodgeMontageToPlay))
	{
		DodgeInternalCooldownRemainTime = DashSideInternalCooldownDuration;
		DodgeFinishRemainTime = DashSideFinishTime;
		DodgeInvincibleRemainTime = DashSideInvincibleDuration;
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

	bDodgeCameraAnchorProjectionActive = false;
	DodgeCameraAnchorStartLocation = FVector::ZeroVector;
	DodgeCameraAnchorDirection = FVector::ZeroVector;
	DodgeCameraAnchorProjectedDistance = 0.f;

	const bool bShouldProjectDodgeCameraAnchor =
		!IsDashDodgeMontage(DodgeMontageToPlay) &&
		(DodgeMontageToPlay == DodgeMontage ||
			DodgeMontageToPlay == LockOnDodgeBackMontage);

	if (bShouldProjectDodgeCameraAnchor && GetCapsuleComponent())
	{
		FVector DodgeDirection = GetActorForwardVector();
		if (DodgeMontageToPlay == BackDashMontage || DodgeMontageToPlay == LockOnDodgeBackMontage)
		{
			DodgeDirection = -GetActorForwardVector();
		}

		DodgeDirection.Z = 0.f;
		DodgeCameraAnchorDirection = DodgeDirection.GetSafeNormal();
		DodgeCameraAnchorStartLocation = GetCapsuleComponent()->GetComponentLocation();
		DodgeCameraAnchorProjectedDistance = 0.f;
		bDodgeCameraAnchorProjectionActive = !DodgeCameraAnchorDirection.IsNearlyZero();

		if (bShowDodgeCameraDebug)
		{
			UE_LOG(LogJun, Warning,
				TEXT("DodgeCameraDebug | Start=%s Montage=%s Chain=%d Dir=%s Start=%s Anchor=%s"),
				bDodgeCameraAnchorProjectionActive ? TEXT("Projection") : TEXT("None"),
				*GetNameSafe(DodgeMontageToPlay),
				bIgnoreDodgeBlockAndReleaseGate ? 1 : 0,
				*DodgeCameraAnchorDirection.ToCompactString(),
				*DodgeCameraAnchorStartLocation.ToCompactString(),
				CameraAnchor ? *CameraAnchor->GetComponentLocation().ToCompactString() : TEXT("None")
			);
		}

	}

	AnimInstance->Montage_Play(DodgeMontageToPlay, CurrentDodgePlayRate);
}

void AJunPlayer::OnDefenseStarted()
{
	if (bIsDodgeAttacking)
	{
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = true;
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
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = true;
		if (ChainParryWindowRemainTime > 0.f)
		{
			OpenParryWindow(true);
		}

		if (ParrySuccessElapsedTime >= ParrySuccessDefenseCancelOpenTime)
		{
			if (ParrySuccessDefenseHoldElapsedTime >= ParrySuccessDefenseHoldConfirmTime)
			{
				CancelParrySuccessForCancelTransition(ParrySuccessDefenseCancelBlendOutTime);
				EnterGuardLoop();
			}
		}
		else
		{
			ParrySuccessDefenseHoldElapsedTime = 0.f;
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

	if (bIsJigenAttacking)
	{
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = true;
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	CancelLockOnTurn();

	if (GetCharacterMovement()->IsFalling())
	{
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = false;
		StartAirParrySequence();
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = true;
		if (!TryCancelDodgeIntoDefense())
		{
			bHoldRequestedForCurrentDeflect = false;
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

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess &&
		BufferedParrySuccessCancelAction == EJunBufferedParrySuccessCancelAction::Defense)
	{
		BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	}
	ParrySuccessDefenseHoldElapsedTime = 0.f;

	if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		if (AnimInstance && GuardBlockMontage)
		{
			AnimInstance->Montage_Stop(GuardBlockReleaseBlendOutTime, GuardBlockMontage);
		}

		bGuardBlockReleasePending = true;
		PlayerHitStateRemainTime = FMath::Max(PlayerHitStateRemainTime, GuardBlockReleaseBlendOutTime);
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

void AJunPlayer::SnapCameraToLookAt(const FVector& WorldTarget, float Pitch)
{
	if (!Controller)
	{
		bCameraRotationInitialized = false;
		return;
	}

	const FVector CameraBasePoint = GetActorLocation();
	FVector ToTarget = WorldTarget - CameraBasePoint;
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		ToTarget = GetActorForwardVector();
	}

	TargetCameraYaw = ToTarget.Rotation().Yaw;
	TargetCameraPitch = FMath::Clamp(Pitch, MinCameraPitch, MaxCameraPitch);
	PendingCameraLookInput = FVector2D::ZeroVector;
	Controller->SetControlRotation(FRotator(TargetCameraPitch, TargetCameraYaw, 0.f));
	bCameraRotationInitialized = true;
	StartCameraHardSnap();
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
	return DefenseState != EJunDefenseState::None && !bAirParrySequenceActive;
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

bool AJunPlayer::IsGuardBlockReacting() const
{
	return CurrentHitState == EJunPlayerHitState::GuardBlock;
}

bool AJunPlayer::IsJumpStartAnimTriggered() const
{
	return JumpStartAnimTriggerRemainTime > 0.f;
}

EJunMoveState AJunPlayer::GetMoveState() const
{
	if (!bAirParrySequenceActive &&
		(DefenseState == EJunDefenseState::Starting ||
		 DefenseState == EJunDefenseState::Guarding))
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

bool AJunPlayer::CanCancelBasicAttackIntoJigen() const
{
	if (!bIsBasicAttacking)
	{
		return false;
	}

	const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
	const float JigenCancelOpenTime = BasicAttackJigenCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackJigenCancelOpenTimes[ComboArrayIndex]
		: 0.f;

	return BasicAttackSectionElapsedTime >= JigenCancelOpenTime;
}

bool AJunPlayer::TryCancelBasicAttackIntoJigen()
{
	if (!CanCancelBasicAttackIntoJigen())
	{
		return false;
	}

	CancelBasicAttackForRecoveryTransition(BasicAttackJigenCancelBlendOutTime);
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
	case EJunBufferedParrySuccessCancelAction::Jigen:
		return ParrySuccessElapsedTime >= ParrySuccessHeavyAttackCancelOpenTime;
	case EJunBufferedParrySuccessCancelAction::Defense:
		return ParrySuccessElapsedTime >= ParrySuccessDefenseCancelOpenTime;
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
	if (bAirParrySequenceActive)
	{
		return false;
	}

	return DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Guarding ||
		(DefenseState == EJunDefenseState::Ending && bKeepGuardBaseWhileEnding) ||
		CurrentHitState == EJunPlayerHitState::GuardBlock ||
		bGuardBlockReleasePending;
}

EJunDefenseState AJunPlayer::GetDefenseState() const
{
	return DefenseState;
}

bool AJunPlayer::IsParryWindowOpen() const
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

void AJunPlayer::BeginTutorialControlLock()
{
	bTutorialControlLocked = true;
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

void AJunPlayer::EndTutorialControlLock()
{
	if (!bTutorialControlLocked)
	{
		return;
	}

	bTutorialControlLocked = false;
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
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

	if (bHasMoveInput &&
		bIsJigenAttacking &&
		TryCancelJigenIntoMove())
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
	if (Is_Dead() || IsInDeathSequence())
	{
		BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;
		return;
	}

	if (!CanCancelBasicAttackIntoRecoveryAction(Action))
	{
		return;
	}

	BufferedBasicAttackRecoveryAction = Action;
	TryExecuteBufferedBasicAttackRecoveryAction();
}

void AJunPlayer::BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction Action)
{
	if (Is_Dead() || IsInDeathSequence())
	{
		BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
		return;
	}

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
	if (Is_Dead() || IsInDeathSequence())
	{
		BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
		bParrySuccessHeavyAttackInputHeld = false;
		return;
	}

	if (CurrentHitState != EJunPlayerHitState::ParrySuccess)
	{
		return;
	}

	BufferedParrySuccessCancelAction = Action;
	if (Action == EJunBufferedParrySuccessCancelAction::HeavyAttack)
	{
		bParrySuccessHeavyAttackInputHeld = true;
	}
	TryExecuteBufferedParrySuccessCancelAction();
}

void AJunPlayer::ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (Is_Dead() || bIsExecuting || IsInDeathSequence())
	{
		return;
	}

	LastIncomingSwingDirection = SwingDirection;
	LastIncomingHitReactType = HitType;
	LastIncomingKnockbackDirection = DetermineKnockbackDirectionFromDamageCauser(DamageCauser);
	LastIncomingDefenseKnockbackData = DefenseKnockbackData;
	LastIncomingDefenseRuleData = DefenseRuleData;
	HitReactFacingTarget = DamageCauser;

	if (DefenseRuleData.DangerAttackType == EJunDangerAttackType::JumpCounter &&
		IsJumpCounterEvadeSuccessful())
	{
		bJumpCounterFollowUpDefenseBypassAvailable = true;
		bJumpCounterStompFollowUpAvailable = true;
		JumpCounterStompTarget = Cast<AJunMonster>(DamageCauser);
		return;
	}

	if (DefenseRuleData.DangerAttackType == EJunDangerAttackType::MikiriCounter &&
		TryHandleMikiriCounter(DamageCauser))
	{
		return;
	}

	const EJunPlayerHitResolveResult ResolveResult = ResolveIncomingHitResult(HitType, DamageCauser);
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);

	switch (ResolveResult)
	{
	case EJunPlayerHitResolveResult::Ignored:
		return;
	case EJunPlayerHitResolveResult::PerfectParrySuccess:
		PlayDefenseSound(EJunDefenseSoundType::PerfectParry);
		PlayDefenseCameraShake(PerfectParryCameraShakeScale);
		if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
		{
			ReducePosture(PerfectAirParryPostureReduction);
		}
		else
		{
			AddPosture(PerfectParryPostureGain);
		}
		StartParrySuccess();
		if (bCanBuildAttackerPostureOnParry)
		{
			if (AJunMonster* AttackingMonster = Cast<AJunMonster>(DamageCauser))
			{
				AttackingMonster->NotifyAttackParriedBy(this, 1.f, DefenseRuleData);
			}
		}
		return;
	case EJunPlayerHitResolveResult::NormalParrySuccess:
		PlayDefenseSound(EJunDefenseSoundType::NormalParry);
		PlayDefenseCameraShake(NormalParryCameraShakeScale);
		if (IsPostureFull())
		{
			StartGuardBreak();
			return;
		}
		if (bCanBuildAttackerPostureOnParry)
		{
			if (AJunMonster* AttackingMonster = Cast<AJunMonster>(DamageCauser))
			{
				AttackingMonster->NotifyAttackParriedBy(this, 0.5f, DefenseRuleData);
			}
		}
		if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
		{
			ReducePosture(NormalAirParryPostureReduction);
		}
		else
		{
			AddPosture(NormalParryPostureGain);
		}
		StartParrySuccess();
		return;
	case EJunPlayerHitResolveResult::GuardBlock:
		PlayDefenseSound(EJunDefenseSoundType::GuardHit);
		PlayDefenseCameraShake(GuardHitCameraShakeScale);
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
		StartHitReact(HitType, DetermineHitReactDirection(DamageCauser, SwingDirection, DefenseRuleData));
		return;
	}
}

void AJunPlayer::OnBasicAttackComboWindowBegin()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	bCanBufferBasicAttackComboInput = true;

	if (bBufferedBasicAttackInput)
	{
		bBufferedBasicAttackComboInput = true;
		bBufferedBasicAttackInput = false;
	}
}

void AJunPlayer::OnAttackComboAdvanceStateBegin(EJunAttackComboType ComboType)
{
	if (ComboType == EJunAttackComboType::Jigen)
	{
		OnJigenComboAdvanceStateBegin();
		return;
	}

	if (ComboType == EJunAttackComboType::HeavyAttack)
	{
		OnHeavyAttackComboAdvanceStateBegin();
		return;
	}

	OnBasicAttackComboAdvanceStateBegin();
}

void AJunPlayer::OnAttackComboAdvanceStateEnd(EJunAttackComboType ComboType)
{
	if (ComboType == EJunAttackComboType::Jigen)
	{
		OnJigenComboAdvanceStateEnd();
		return;
	}

	if (ComboType == EJunAttackComboType::HeavyAttack)
	{
		OnHeavyAttackComboAdvanceStateEnd();
		return;
	}

	OnBasicAttackComboAdvanceStateEnd();
}

void AJunPlayer::OnBasicAttackComboAdvanceStateBegin()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

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
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

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

void AJunPlayer::OnJigenComboWindowBegin()
{
	if (!bIsJigenAttacking)
	{
		return;
	}

	bCanBufferJigenComboInput = true;

	if (bBufferedJigenInput)
	{
		bBufferedJigenComboInput = true;
		bBufferedJigenInput = false;
	}
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
	if (bDefenseButtonHeld && CanCancelDodgeIntoRecoveryAction())
	{
		TryCancelDodgeIntoDefense();
		return;
	}

	if (DodgeElapsedTime >= GetDodgeMoveCancelOpenTimeForMontage(CurrentDodgeMontage))
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Move);

		if (IsDashDodgeMontage(CurrentDodgeMontage))
		{
			const bool bHasMoveInput =
				!FMath::IsNearlyZero(DesiredMoveForward) ||
				!FMath::IsNearlyZero(DesiredMoveRight) ||
				!FMath::IsNearlyZero(PendingMoveForward) ||
				!FMath::IsNearlyZero(PendingMoveRight);
			if (bHasMoveInput && TryCancelDashDodgeIntoMove())
			{
				return;
			}
		}
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

bool AJunPlayer::TryCancelDashDodgeIntoMove()
{
	if (!HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		!IsDashDodgeMontage(CurrentDodgeMontage))
	{
		return false;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	if (AnimInstance && DodgeMontageToStop)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
		AnimInstance->Montage_Stop(DashMoveCancelBlendOutTime, DodgeMontageToStop);
	}

	FinishDodgeState();
	DodgeInternalCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	CurrentDodgePlayRate = 1.f;
	DodgeElapsedTime = 0.f;
	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;
	return true;
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

bool AJunPlayer::TryCancelDodgeIntoJigen()
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(DodgeJigenCancelBlendOutTime);
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

void AJunPlayer::NotifyGuardRestartAnchorReached(UAnimMontage* Montage, float AnchorPosition)
{
	if (!Montage)
	{
		return;
	}

	if (Montage != GuardStartMontage && Montage != AirGuardStartMontage)
	{
		return;
	}

	GuardRestartAnchorMontage = Montage;
	GuardRestartAnchorPosition = FMath::Max(0.f, AnchorPosition);
	bGuardRestartAnchorReached = true;
}

void AJunPlayer::StartGuardRestartBlend(UAnimMontage* Montage, float TargetPosition, float BlendDuration)
{
	if (!AnimInstance || !Montage || !AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	const float CurrentPosition = AnimInstance->Montage_GetPosition(Montage);
	const float ClampedTargetPosition = FMath::Clamp(TargetPosition, 0.f, Montage->GetPlayLength());
	if (BlendDuration <= KINDA_SMALL_NUMBER || FMath::IsNearlyEqual(CurrentPosition, ClampedTargetPosition, KINDA_SMALL_NUMBER))
	{
		AnimInstance->Montage_SetPosition(Montage, ClampedTargetPosition);
		AnimInstance->Montage_SetPlayRate(Montage, CurrentDefensePlayRate);
		FinishGuardRestartBlend(false);
		return;
	}

	GuardRestartBlendMontage = Montage;
	GuardRestartBlendStartPosition = CurrentPosition;
	GuardRestartBlendTargetPosition = ClampedTargetPosition;
	GuardRestartBlendElapsedTime = 0.f;
	GuardRestartBlendDuration = BlendDuration;
	GuardRestartBlendRestorePlayRate = CurrentDefensePlayRate;
	bGuardRestartBlendActive = true;
	AnimInstance->Montage_SetPlayRate(Montage, 0.f);
}

void AJunPlayer::UpdateGuardRestartBlend(float DeltaTime)
{
	if (!bGuardRestartBlendActive)
	{
		return;
	}

	UAnimMontage* Montage = GuardRestartBlendMontage.Get();
	if (!AnimInstance || !Montage || !AnimInstance->Montage_IsPlaying(Montage))
	{
		FinishGuardRestartBlend(false);
		return;
	}

	GuardRestartBlendElapsedTime = FMath::Min(
		GuardRestartBlendDuration,
		GuardRestartBlendElapsedTime + DeltaTime);
	const float Alpha = GuardRestartBlendDuration > KINDA_SMALL_NUMBER
		? GuardRestartBlendElapsedTime / GuardRestartBlendDuration
		: 1.f;
	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	const float NewPosition = FMath::Lerp(
		GuardRestartBlendStartPosition,
		GuardRestartBlendTargetPosition,
		SmoothAlpha);
	AnimInstance->Montage_SetPosition(Montage, NewPosition);

	if (GuardRestartBlendElapsedTime >= GuardRestartBlendDuration)
	{
		FinishGuardRestartBlend(true);
	}
}

void AJunPlayer::FinishGuardRestartBlend(bool bSnapToTarget)
{
	UAnimMontage* Montage = GuardRestartBlendMontage.Get();
	if (AnimInstance && Montage && AnimInstance->Montage_IsPlaying(Montage))
	{
		if (bSnapToTarget)
		{
			AnimInstance->Montage_SetPosition(Montage, GuardRestartBlendTargetPosition);
		}

		AnimInstance->Montage_SetPlayRate(Montage, GuardRestartBlendRestorePlayRate);
	}

	GuardRestartBlendMontage = nullptr;
	GuardRestartBlendStartPosition = 0.f;
	GuardRestartBlendTargetPosition = 0.f;
	GuardRestartBlendElapsedTime = 0.f;
	GuardRestartBlendDuration = 0.f;
	GuardRestartBlendRestorePlayRate = 1.f;
	bGuardRestartBlendActive = false;
}

void AJunPlayer::ResetGuardRestartAnchor()
{
	FinishGuardRestartBlend(false);
	GuardRestartAnchorMontage = nullptr;
	GuardRestartAnchorPosition = 0.f;
	bGuardRestartAnchorReached = false;
}

void AJunPlayer::TryExecuteBufferedBasicAttackRecoveryAction()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;
		return;
	}

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

	if (bIsHeavyAttacking &&
		bHeavyAttackInputHeld &&
		HeavyAttackState == EJunHeavyAttackState::Tap)
	{
		HeavyAttackInputHoldTime += DeltaTime;
		if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold &&
			CanCancelHeavyAttackIntoHeavyAttack())
		{
			CancelHeavyAttackForRecoveryTransition(HeavyAttackRestartBlendOutTime);
			StartHeavyAttackCharge();
			return;
		}
	}

	if (!bIsHeavyAttacking)
	{
		if (!CanStartHeavyAttack())
		{
			if (bHeavyAttackInputHeld && bIsJigenAttacking)
			{
				HeavyAttackInputHoldTime += DeltaTime;
				return;
			}

			if (bHeavyAttackInputHeld && CanBufferDefenseTransitionCancel())
			{
				HeavyAttackInputHoldTime += DeltaTime;
				return;
			}

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

void AJunPlayer::BeginHeavyAttackHoldInput()
{
	if (bHeavyAttackInputHeld)
	{
		return;
	}

	bHeavyAttackInputHeld = true;
	HeavyAttackInputHoldTime = FMath::Max(HeavyAttackInputHoldTime, 0.f);
	HeavyAttackChargeLoopElapsedTime = 0.f;
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
	AnimInstance->Montage_PlayWithBlendIn(
		HeavyAttackTapMontage,
		FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackStartBlendInTime)),
		CurrentHeavyAttackPlayRate);
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
	AnimInstance->Montage_PlayWithBlendIn(
		HeavyAttackChargeMontage,
		FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackStartBlendInTime)),
		CurrentHeavyAttackPlayRate);
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
		AnimInstance->Montage_PlayWithBlendIn(
			HeavyAttackChargeMontage,
			FAlphaBlendArgs(FMath::Max(0.f, HeavyAttackStartBlendInTime)),
			CurrentHeavyAttackPlayRate);
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

	CancelHeavyAttackForRecoveryTransition(HeavyAttackHeavyAttackCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelHeavyAttackIntoJigen() const
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

	const float JigenCancelOpenTime = HeavyAttackState == EJunHeavyAttackState::Tap
		? HeavyAttackTapJigenCancelOpenTime
		: HeavyAttackChargeEndJigenCancelOpenTime;

	return HeavyAttackSectionElapsedTime >= JigenCancelOpenTime;
}

bool AJunPlayer::TryCancelHeavyAttackIntoJigen()
{
	if (!CanCancelHeavyAttackIntoJigen())
	{
		return false;
	}

	CancelHeavyAttackForRecoveryTransition(HeavyAttackJigenCancelBlendOutTime);
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

float AJunPlayer::GetDistanceFromGround() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return 0.f;
	}

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float TraceDistance = CapsuleHalfHeight + FMath::Max(JumpCounterEvadeMinGroundDistance + 200.f, JumpAttackMinGroundDistance + 20.f);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JumpCounterGroundCheck), false, this);
	FHitResult Hit;
	const bool bHitGround = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);

	if (!bHitGround)
	{
		return TraceDistance - CapsuleHalfHeight;
	}

	return FMath::Max(0.f, Hit.Distance - CapsuleHalfHeight);
}

bool AJunPlayer::IsJumpCounterEvadeSuccessful() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	return MovementComponent &&
		MovementComponent->IsFalling() &&
		GetDistanceFromGround() >= JumpCounterEvadeMinGroundDistance;
}

void AJunPlayer::UpdateJumpCounterStompOpportunity()
{
	if (CurrentHitState != EJunPlayerHitState::None)
	{
		bJumpCounterStompOpportunityPending = false;
		JumpCounterStompPendingTarget = nullptr;
		return;
	}

	if (bJumpCounterStompFollowUpAvailable ||
		bJumpCounterStompFollowUpActive ||
		!GetCharacterMovement() ||
		!GetCharacterMovement()->IsFalling())
	{
		return;
	}

	const float OpportunityDistance = FMath::Max(JumpCounterStompOpportunityDistance, JumpCounterStompMaxStartDistance);
	AJunMonster* CandidateMonster = Cast<AJunMonster>(LockOnTarget.Get());
	if (CandidateMonster &&
		!CandidateMonster->Is_Dead() &&
		CandidateMonster->IsJumpCounterThreatActive() &&
		FVector::Dist2D(GetActorLocation(), CandidateMonster->GetActorLocation()) <= OpportunityDistance)
	{
		bJumpCounterStompOpportunityPending = true;
		JumpCounterStompPendingTarget = CandidateMonster;
		return;
	}

	if (bJumpCounterStompOpportunityPending)
	{
		AJunMonster* PendingMonster = JumpCounterStompPendingTarget.Get();
		if (!PendingMonster || PendingMonster->Is_Dead())
		{
			bJumpCounterStompOpportunityPending = false;
			JumpCounterStompPendingTarget = nullptr;
			return;
		}

		if (!PendingMonster->IsJumpCounterThreatActive())
		{
			bJumpCounterFollowUpDefenseBypassAvailable = true;
			bJumpCounterStompFollowUpAvailable = true;
			JumpCounterStompTarget = PendingMonster;
			bJumpCounterStompOpportunityPending = false;
			JumpCounterStompPendingTarget = nullptr;
			return;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float MaxDistanceSq = FMath::Square(OpportunityDistance);
	AJunMonster* BestMonster = nullptr;
	float BestDistanceSq = MaxDistanceSq;
	for (TActorIterator<AJunMonster> It(World); It; ++It)
	{
		AJunMonster* Monster = *It;
		if (!Monster ||
			Monster->Is_Dead() ||
			!Monster->IsJumpCounterThreatActive() ||
			!IsEnemyTo(Monster))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Monster->GetActorLocation());
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestMonster = Monster;
		}
	}

	if (!BestMonster)
	{
		return;
	}

	bJumpCounterStompOpportunityPending = true;
	JumpCounterStompPendingTarget = BestMonster;
}

void AJunPlayer::UpdateJumpCounterStompFollowUp(float DeltaTime)
{
	if (!bJumpCounterStompFollowUpActive)
	{
		return;
	}

	const float CorrectionDuration = FMath::Max(JumpCounterStompCorrectionDuration, KINDA_SMALL_NUMBER);
	if (JumpCounterStompCorrectionElapsedTime < CorrectionDuration)
	{
		JumpCounterStompCorrectionElapsedTime = FMath::Min(
			JumpCounterStompCorrectionElapsedTime + DeltaTime,
			CorrectionDuration);

		const float Alpha = FMath::Clamp(JumpCounterStompCorrectionElapsedTime / CorrectionDuration, 0.f, 1.f);
		const float SmoothAlpha = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
		const FVector NewLocation = FMath::Lerp(
			JumpCounterStompCorrectionStartLocation,
			JumpCounterStompCorrectionTargetLocation,
			SmoothAlpha);

		SetActorLocation(NewLocation, true);
	}

	if (JumpCounterStompBounceMoveRemainTime > 0.f && !JumpCounterStompBounceMoveDirection.IsNearlyZero())
	{
		const float StepTime = FMath::Min(DeltaTime, JumpCounterStompBounceMoveRemainTime);
		JumpCounterStompBounceMoveRemainTime = FMath::Max(0.f, JumpCounterStompBounceMoveRemainTime - DeltaTime);

		const FVector MoveDelta = JumpCounterStompBounceMoveDirection * JumpCounterStompBounceMoveSpeed * StepTime;
		FHitResult MoveHit;
		AddActorWorldOffset(MoveDelta, true, &MoveHit);

		if (MoveHit.bBlockingHit)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("JumpCounterStompBounceBlocked | Delta=%s HitActor=%s HitComp=%s Normal=%s Location=%s"),
				*MoveDelta.ToCompactString(),
				*GetNameSafe(MoveHit.GetActor()),
				*GetNameSafe(MoveHit.GetComponent()),
				*MoveHit.ImpactNormal.ToCompactString(),
				*GetActorLocation().ToCompactString());
		}
	}
}

bool AJunPlayer::CanStartJumpCounterStompFollowUp() const
{
	const AJunMonster* TargetMonster = JumpCounterStompTarget.Get();
	return bJumpCounterStompFollowUpAvailable &&
		!bJumpCounterStompFollowUpActive &&
		JumpCounterStompFollowUpMontage &&
		AnimInstance &&
		TargetMonster &&
		!TargetMonster->Is_Dead() &&
		GetCharacterMovement() &&
		GetCharacterMovement()->IsFalling() &&
		DeathState == EJunPlayerDeathState::Alive &&
		!Is_Dead() &&
		!bIsExecuting &&
		!IsInDeathSequence() &&
		CurrentHitState == EJunPlayerHitState::None &&
		!bIsJumpAttacking &&
		!bIsDodgeAttacking &&
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		FVector::Dist2D(GetActorLocation(), TargetMonster->GetActorLocation()) <= JumpCounterStompMaxStartDistance;
}

FVector AJunPlayer::GetJumpCounterStompTargetLocation(const AJunMonster* TargetMonster) const
{
	if (!TargetMonster)
	{
		return GetActorLocation();
	}

	const FVector TargetPoint = TargetMonster->GetLockOnTargetPoint();
	const FVector BossLocation = TargetMonster->GetActorLocation();
	const FVector DesiredPoint =
		TargetPoint +
		TargetMonster->GetActorForwardVector().GetSafeNormal2D() * JumpCounterStompTargetForwardOffset +
		FVector::UpVector * JumpCounterStompTargetHeightOffset;

	FVector DesiredOffset2D = DesiredPoint - BossLocation;
	DesiredOffset2D.Z = 0.f;

	if (DesiredOffset2D.IsNearlyZero())
	{
		DesiredOffset2D = GetActorLocation() - BossLocation;
		DesiredOffset2D.Z = 0.f;
	}

	if (DesiredOffset2D.IsNearlyZero())
	{
		DesiredOffset2D = -TargetMonster->GetActorForwardVector();
		DesiredOffset2D.Z = 0.f;
	}

	const float BossCapsuleRadius = TargetMonster->GetCapsuleComponent()
		? TargetMonster->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.f;
	const float PlayerCapsuleRadius = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.f;
	const float MinSafeDistance = bJumpCounterStompIgnoreTargetCollisionDuringFollowUp
		? FMath::Max(0.f, JumpCounterStompMinVisualDistance)
		: BossCapsuleRadius + PlayerCapsuleRadius + FMath::Max(0.f, JumpCounterStompCapsuleMargin);
	const float DesiredDistance = DesiredOffset2D.Size();
	const float ResolvedDistance = FMath::Max(DesiredDistance, MinSafeDistance);
	const FVector SafeOffset2D = DesiredOffset2D.GetSafeNormal2D() * ResolvedDistance;

	return FVector(
		BossLocation.X + SafeOffset2D.X,
		BossLocation.Y + SafeOffset2D.Y,
		DesiredPoint.Z);
}

void AJunPlayer::SetJumpCounterStompTargetCollisionIgnored(AJunMonster* TargetMonster, bool bIgnore)
{
	UCapsuleComponent* PlayerCapsule = GetCapsuleComponent();
	UCapsuleComponent* TargetCapsule = TargetMonster ? TargetMonster->GetCapsuleComponent() : nullptr;
	if (!PlayerCapsule || !TargetMonster)
	{
		return;
	}

	PlayerCapsule->IgnoreActorWhenMoving(TargetMonster, bIgnore);
	if (TargetCapsule)
	{
		TargetCapsule->IgnoreActorWhenMoving(this, bIgnore);
	}

	JumpCounterStompIgnoredCollisionTarget = bIgnore ? TargetMonster : nullptr;
}

void AJunPlayer::RestoreJumpCounterStompTargetCollisionIgnored()
{
	AJunMonster* IgnoredTarget = JumpCounterStompIgnoredCollisionTarget.Get();
	if (!IgnoredTarget)
	{
		return;
	}

	SetJumpCounterStompTargetCollisionIgnored(IgnoredTarget, false);
}

bool AJunPlayer::TryStartJumpCounterStompFollowUp()
{
	if (!CanStartJumpCounterStompFollowUp())
	{
		return false;
	}

	AJunMonster* TargetMonster = JumpCounterStompTarget.Get();
	if (!TargetMonster)
	{
		return false;
	}

	InterruptActionsForHitReaction();
	CancelLockOnTurn();

	bJumpCounterStompFollowUpActive = true;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterFollowUpDefenseBypassAvailable = true;
	AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	bJumpCounterStompInvincibilityActive = true;
	if (bJumpCounterStompIgnoreTargetCollisionDuringFollowUp)
	{
		SetJumpCounterStompTargetCollisionIgnored(TargetMonster, true);
	}
	JumpCounterStompCorrectionElapsedTime = 0.f;
	JumpCounterStompCorrectionStartLocation = GetActorLocation();
	JumpCounterStompCorrectionTargetLocation = GetJumpCounterStompTargetLocation(TargetMonster);

	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	ConsumeMovementInputVector();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpCounterStompFollowUpMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJumpCounterStompFollowUpMontageEnded);
	const float PlayedDuration = AnimInstance->Montage_Play(
		JumpCounterStompFollowUpMontage,
		FMath::Max(JumpCounterStompPlayRate, KINDA_SMALL_NUMBER));
	if (PlayedDuration <= 0.f)
	{
		FinishJumpCounterStompFollowUp(false);
		return false;
	}

	return true;
}

bool AJunPlayer::TryCancelJumpCounterStompFollowUpIntoJumpAttack()
{
	if (!bJumpCounterStompFollowUpActive ||
		!bJumpCounterStompJumpAttackWindowOpen ||
		!JumpAttackMontage ||
		!AnimInstance ||
		!GetCharacterMovement() ||
		!GetCharacterMovement()->IsFalling() ||
		bIsJumpAttacking ||
		DeathState != EJunPlayerDeathState::Alive ||
		Is_Dead())
	{
		return false;
	}

	if (AnimInstance && JumpCounterStompFollowUpMontage)
	{
		AnimInstance->Montage_Stop(
			FMath::Max(0.f, JumpCounterStompJumpAttackCancelBlendOutTime),
			JumpCounterStompFollowUpMontage);
	}

	FinishJumpCounterStompFollowUp(false);
	StartJumpAttack();
	return bIsJumpAttacking;
}

void AJunPlayer::BeginJumpCounterStompJumpAttackWindow()
{
	if (bJumpCounterStompFollowUpActive)
	{
		bJumpCounterStompJumpAttackWindowOpen = true;
	}
}

void AJunPlayer::EndJumpCounterStompJumpAttackWindow()
{
	bJumpCounterStompJumpAttackWindowOpen = false;
}

void AJunPlayer::ApplyJumpCounterStompBounce(float UpVelocity, float BackwardVelocity)
{
	ApplyJumpCounterStompBounce(UpVelocity, BackwardVelocity, 0.15f);
}

void AJunPlayer::ApplyJumpCounterStompBounce(float UpVelocity, float BackwardVelocity, float BackwardMoveDuration)
{
	if (!bJumpCounterStompFollowUpActive || !GetCharacterMovement())
	{
		return;
	}

	JumpCounterStompCorrectionElapsedTime = FMath::Max(JumpCounterStompCorrectionDuration, 0.f);
	if (!JumpCounterStompCorrectionTargetLocation.IsNearlyZero())
	{
		FHitResult SnapHit;
		SetActorLocation(JumpCounterStompCorrectionTargetLocation, true, &SnapHit);
		if (SnapHit.bBlockingHit)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("JumpCounterStompSnapBlocked | Target=%s Actual=%s HitActor=%s HitComp=%s Normal=%s"),
				*JumpCounterStompCorrectionTargetLocation.ToCompactString(),
				*GetActorLocation().ToCompactString(),
				*GetNameSafe(SnapHit.GetActor()),
				*GetNameSafe(SnapHit.GetComponent()),
				*SnapHit.ImpactNormal.ToCompactString());
		}
	}

	const FVector BounceReferenceLocation = JumpCounterStompTarget
		? JumpCounterStompTarget->GetActorLocation()
		: (LockOnTarget ? LockOnTarget->GetActorLocation() : GetActorLocation() + GetActorForwardVector());
	FVector BounceAwayDirection = (GetActorLocation() - BounceReferenceLocation).GetSafeNormal2D();
	if (JumpCounterStompTarget && BounceAwayDirection.IsNearlyZero())
	{
		BounceAwayDirection = -JumpCounterStompTarget->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector SafeBounceAwayDirection = BounceAwayDirection.IsNearlyZero()
		? -GetActorForwardVector().GetSafeNormal2D()
		: BounceAwayDirection;
	const FVector BounceVelocity =
		SafeBounceAwayDirection * FMath::Max(0.f, BackwardVelocity) +
		FVector::UpVector * UpVelocity;

	JumpCounterStompBounceMoveDirection = SafeBounceAwayDirection;
	JumpCounterStompBounceMoveSpeed = FMath::Max(0.f, BackwardVelocity);
	JumpCounterStompBounceMoveRemainTime = FMath::Max(0.f, BackwardMoveDuration);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->Velocity.X = BounceVelocity.X;
		MovementComponent->Velocity.Y = BounceVelocity.Y;
	}

	UE_LOG(
		LogJun,
		Warning,
		TEXT("JumpCounterStompBounceApply | Loc=%s Ref=%s Dir=%s Velocity=%s CodeMoveSpeed=%.1f CodeMoveDuration=%.3f Active=%d"),
		*GetActorLocation().ToCompactString(),
		*BounceReferenceLocation.ToCompactString(),
		*SafeBounceAwayDirection.ToCompactString(),
		*BounceVelocity.ToCompactString(),
		JumpCounterStompBounceMoveSpeed,
		JumpCounterStompBounceMoveRemainTime,
		bJumpCounterStompFollowUpActive ? 1 : 0);

	LaunchCharacter(BounceVelocity, true, true);
}

void AJunPlayer::ApplyJumpCounterStompHit(
	EHitReactType HitReactType,
	const FJunAttackDamageData& DamageData,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	AJunMonster* TargetMonster = JumpCounterStompTarget.Get();
	if (!bJumpCounterStompFollowUpActive || !TargetMonster || TargetMonster->Is_Dead())
	{
		return;
	}

	FJunAttackDefenseRuleData DefenseRuleData;
	DefenseRuleData.bCanBeParried = false;
	DefenseRuleData.bCanBeGuarded = true;
	DefenseRuleData.bCanBeDodgedByInvincibility = false;

	const FVector SwingDirection = (TargetMonster->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	TargetMonster->ReceiveHit(
		HitReactType,
		DamageData.GetFinalDamage(),
		this,
		SwingDirection.IsNearlyZero() ? GetActorForwardVector() : SwingDirection,
		DefenseKnockbackData,
		DefenseRuleData,
		DamageData.PoiseDamage);

	TargetMonster->AddPosture(JumpCounterStompPostureGain);
}

void AJunPlayer::FinishJumpCounterStompFollowUp(bool bApplyBounce)
{
	if (AnimInstance && JumpCounterStompFollowUpMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpCounterStompFollowUpMontageEnded);
	}

	bJumpCounterStompFollowUpActive = false;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompOpportunityPending = false;
	bJumpCounterStompJumpAttackWindowOpen = false;
	if (bJumpCounterStompInvincibilityActive)
	{
		RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
		bJumpCounterStompInvincibilityActive = false;
	}
	JumpCounterStompCorrectionElapsedTime = 0.f;
	JumpCounterStompCorrectionStartLocation = FVector::ZeroVector;
	JumpCounterStompCorrectionTargetLocation = FVector::ZeroVector;
	JumpCounterStompBounceMoveRemainTime = 0.f;
	JumpCounterStompBounceMoveDirection = FVector::ZeroVector;
	JumpCounterStompBounceMoveSpeed = 0.f;
	RestoreJumpCounterStompTargetCollisionIgnored();
	JumpCounterStompPendingTarget = nullptr;
	JumpCounterStompTarget = nullptr;

	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	if (bApplyBounce)
	{
		ApplyJumpCounterStompBounce(JumpCounterStompBounceUpVelocity, JumpCounterStompBounceBackwardVelocity);
	}
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
	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	JumpCounterStompTarget = nullptr;
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

bool AJunPlayer::CanCancelJumpAttackEndIntoJigen() const
{
	return bIsJumpAttacking &&
		JumpAttackState == EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackEndJigenCancelOpenTime;
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

bool AJunPlayer::TryCancelJumpAttackEndIntoJigen()
{
	if (!CanCancelJumpAttackEndIntoJigen())
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(JumpAttackEndJigenCancelBlendOutTime);
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

	if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		FinishPlayerHitState();
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
	case EJunBufferedDefenseCancelAction::HeavyAttack:
	{
		const float PreservedHeavyAttackHoldTime = HeavyAttackInputHoldTime;
		CancelDefenseForCancelTransition();
		bHeavyAttackInputHeld = true;
		HeavyAttackInputHoldTime = FMath::Max(PreservedHeavyAttackHoldTime, 0.f);
		HeavyAttackChargeLoopElapsedTime = 0.f;
		if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
		{
			StartHeavyAttackCharge();
		}
		break;
	}
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
	if (Is_Dead() || IsInDeathSequence())
	{
		BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
		bParrySuccessHeavyAttackInputHeld = false;
		return;
	}

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
	{
		const bool bShouldChargeFromHeldInput = bParrySuccessHeavyAttackInputHeld;
		const float PreservedHeavyAttackHoldTime = HeavyAttackInputHoldTime;
		CancelParrySuccessForCancelTransition(ParrySuccessHeavyAttackCancelBlendOutTime);
		if (bShouldChargeFromHeldInput)
		{
			bHeavyAttackInputHeld = true;
			HeavyAttackInputHoldTime = FMath::Max(PreservedHeavyAttackHoldTime, 0.f);
			HeavyAttackChargeLoopElapsedTime = 0.f;
			if (HeavyAttackInputHoldTime >= HeavyAttackChargeThreshold)
			{
				StartHeavyAttackCharge();
			}
		}
		else
		{
			StartHeavyAttackTap();
		}
		bParrySuccessHeavyAttackInputHeld = false;
		break;
	}
	case EJunBufferedParrySuccessCancelAction::Jigen:
		CancelParrySuccessForCancelTransition(ParrySuccessHeavyAttackCancelBlendOutTime);
		StartJigen();
		break;
	case EJunBufferedParrySuccessCancelAction::Defense:
		CancelParrySuccessForCancelTransition(ParrySuccessDefenseCancelBlendOutTime);
		EnterGuardLoop();
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
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

		if (GuardStartMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, GuardStartMontage);
		}

		if (GuardEndMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, GuardEndMontage);
		}

		if (AirGuardStartMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, AirGuardStartMontage);
		}

		if (AirGuardEndMontage)
		{
			AnimInstance->Montage_Stop(ResolvedBlendOutTime, AirGuardEndMontage);
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
	bDodgeCameraAnchorProjectionActive = false;
	DodgeCameraAnchorStartLocation = FVector::ZeroVector;
	DodgeCameraAnchorDirection = FVector::ZeroVector;
	DodgeCameraAnchorProjectedDistance = 0.f;
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

bool AJunPlayer::CanCancelDodgeAttackIntoJigen() const
{
	return bIsDodgeAttacking && DodgeAttackElapsedTime >= DodgeAttackJigenCancelOpenTime;
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

bool AJunPlayer::TryCancelDodgeAttackIntoJigen()
{
	if (!CanCancelDodgeAttackIntoJigen())
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(DodgeAttackJigenCancelBlendOutTime);
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
	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		return false;
	}

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
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->SetExecutionBGMDucked(true);
	}
	if (ExecutionStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExecutionStartSound, GetActorLocation(), ExecutionStartSoundVolume);
	}
	if (GetWorld())
	{
		const bool bUseFinishSlowMotion = bCurrentExecutionIsFinish && bEnableExecutionFinishStartSlowMotion;
		const bool bUseNormalSlowMotion = !bCurrentExecutionIsFinish && bEnableExecutionStartSlowMotion;
		if (bUseFinishSlowMotion || bUseNormalSlowMotion)
		{
			const float SlowMotionDuration = bUseFinishSlowMotion
				? ExecutionFinishStartSlowMotionDuration
				: ExecutionStartSlowMotionDuration;
			const float SlowMotionTimeScale = bUseFinishSlowMotion
				? ExecutionFinishStartSlowMotionTimeScale
				: ExecutionStartSlowMotionTimeScale;
			const float SlowMotionBlendInTime = bUseFinishSlowMotion
				? ExecutionFinishStartSlowMotionBlendInTime
				: ExecutionStartSlowMotionBlendInTime;
			const float SlowMotionBlendOutTime = bUseFinishSlowMotion
				? ExecutionFinishStartSlowMotionBlendOutTime
				: ExecutionStartSlowMotionBlendOutTime;

			if (UJunTimeEffectSubsystem* TimeEffectSubsystem = GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
			{
				TimeEffectSubsystem->RequestSlowMotion(
					SlowMotionDuration,
					SlowMotionTimeScale,
					SlowMotionBlendInTime,
					SlowMotionBlendOutTime,
					ExecutionStartSlowMotionPriority);
			}
		}
	}
	SetExecutionCapsuleCollisionIgnore(Monster, true);
	ReduceExecutionCapsuleRadius(Monster);
	StartExecutionCamera(Monster);

	const FVector ToTarget = Monster->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.ToOrientationRotator());
	}

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnExecutionMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnExecutionMontageEnded);
	const float PlayResult = PlayAnimMontage(CurrentExecutionMontage);
	if (PlayResult <= 0.f)
	{
		UE_LOG(LogJun, Warning,
			TEXT("Execution montage play failed. Player=%s Target=%s Montage=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Monster),
			*GetNameSafe(CurrentExecutionMontage));
		FinishExecution(true);
	}
}

void AJunPlayer::TriggerExecutionTargetMontage()
{
	if (!bIsExecuting || !CurrentExecutionTarget)
	{
		return;
	}

	CurrentExecutionTarget->TriggerPendingExecutionMontage(!bCurrentExecutionIsFinish);
}

void AJunPlayer::FinishExecution(bool bInterrupted)
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
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->SetExecutionBGMDucked(false);
	}
	const bool bWasFinishExecution = bCurrentExecutionIsFinish;
	if (CurrentExecutionTarget && !CurrentExecutionTarget->HasExecutionResultApplied())
	{
		if (bInterrupted)
		{
			CurrentExecutionTarget->CancelPendingExecution();
		}
		else
		{
			UE_LOG(LogJun, Warning,
				TEXT("Execution finished before target result was applied. Forcing result. Player=%s Target=%s Montage=%s"),
				*GetNameSafe(this),
				*GetNameSafe(CurrentExecutionTarget),
				*GetNameSafe(CurrentExecutionMontage));
			CurrentExecutionTarget->TriggerPendingExecutionMontage(!bWasFinishExecution);
			if (!CurrentExecutionTarget->HasExecutionResultApplied())
			{
				CurrentExecutionTarget->ApplyPendingExecutionResult();
			}
		}
	}
	bCurrentExecutionIsFinish = false;
	SetExecutionCapsuleCollisionIgnore(CurrentExecutionTarget.Get(), false);
	RestoreExecutionCapsuleRadius(CurrentExecutionTarget.Get());
	CurrentExecutionTarget = nullptr;
	CurrentExecutionMontage = nullptr;

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
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

	FinishExecution(bInterrupted);
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
	bAirParrySequenceActive = false;
	bGuardBlockReleasePending = false;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	ResetGuardRestartAnchor();
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
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

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
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

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

void AJunPlayer::OnJigenComboAdvanceStateBegin()
{
	if (!bIsJigenAttacking)
	{
		return;
	}

	bJigenComboAdvanceStateActive = true;
	bCanBufferJigenComboInput = false;

	if (bBufferedJigenComboInput)
	{
		TryAdvanceJigenCombo();
	}
}

void AJunPlayer::OnJigenComboAdvanceStateEnd()
{
	bJigenComboAdvanceStateActive = false;
	bBufferedJigenComboInput = false;
}

bool AJunPlayer::TryStartJigen()
{
	if (bIsJigenAttacking)
	{
		const bool bCanAcceptJigenComboInput = bDefenseButtonHeld || JigenComboIndex <= 1;

		if (bCanAcceptJigenComboInput && bJigenComboAdvanceStateActive)
		{
			bBufferedJigenComboInput = true;
			TryAdvanceJigenCombo();
			return true;
		}

		if (bCanAcceptJigenComboInput && bCanBufferJigenComboInput)
		{
			bBufferedJigenComboInput = true;
			return true;
		}

		if (!bDefenseButtonHeld)
		{
			if (JigenComboIndex <= 1)
			{
				bBufferedJigenInput = true;
				return true;
			}

			if (TryCancelJigenIntoBasicAttack())
			{
				return true;
			}

			return true;
		}

		bBufferedJigenInput = true;
		return true;
	}

	if (!bDefenseButtonHeld)
	{
		return false;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::Jigen;
		TryExecuteBufferedParrySuccessCancelAction();
		return true;
	}

	if (TryChooseFakeDeathDie())
	{
		return true;
	}

	if (!AnimInstance || !JigenMontage)
	{
		return false;
	}

	if (bIsBasicAttacking)
	{
		if (!TryCancelBasicAttackIntoJigen())
		{
			return false;
		}
	}

	if (bIsHeavyAttacking)
	{
		if (!TryCancelHeavyAttackIntoJigen())
		{
			return false;
		}
	}

	if (bIsJumpAttacking)
	{
		if (!TryCancelJumpAttackEndIntoJigen())
		{
			return false;
		}
	}

	if (bIsDodgeAttacking)
	{
		if (!TryCancelDodgeAttackIntoJigen())
		{
			return false;
		}
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (!TryCancelDodgeIntoJigen())
		{
			return false;
		}
	}

	if (CurrentHitState != EJunPlayerHitState::None)
	{
		return false;
	}

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	if (DefenseState != EJunDefenseState::None)
	{
		FinishDefenseForCancel();
	}

	StartJigen();
	return true;
}

void AJunPlayer::StartJigen()
{
	if (!AnimInstance || !JigenMontage)
	{
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJigenAttacking = true;
	JigenComboIndex = 1;
	bCanBufferJigenComboInput = false;
	bBufferedJigenComboInput = false;
	bBufferedJigenInput = false;
	bJigenComboAdvanceStateActive = false;
	JigenSectionElapsedTime = 0.f;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJigenMontageEnded);
	AnimInstance->Montage_PlayWithBlendIn(
		JigenMontage,
		FAlphaBlendArgs(FMath::Max(0.f, JigenStartBlendInTime)),
		FMath::Max(JigenPlayRate, KINDA_SMALL_NUMBER));
	AnimInstance->Montage_JumpToSection(JigenFirstSectionName, JigenMontage);
}

void AJunPlayer::TryAdvanceJigenCombo()
{
	if (!bIsJigenAttacking || !bBufferedJigenComboInput || !JigenMontage || !AnimInstance)
	{
		return;
	}

	if (JigenComboIndex <= 1)
	{
		bBufferedJigenComboInput = false;
		bCanBufferJigenComboInput = false;
		bJigenComboAdvanceStateActive = false;
		JigenComboIndex = 2;
		JigenSectionElapsedTime = 0.f;

		TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

		const int32 SecondSectionIndex = JigenMontage->GetSectionIndex(JigenSecondSectionName);
		if (SecondSectionIndex != INDEX_NONE)
		{
			float SectionStartTime = 0.f;
			float SectionEndTime = 0.f;
			JigenMontage->GetSectionStartAndEndTime(SecondSectionIndex, SectionStartTime, SectionEndTime);

			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
			const float MontagePlayResult = AnimInstance->Montage_PlayWithBlendIn(
				JigenMontage,
				FAlphaBlendArgs(FMath::Max(0.f, JigenComboBlendInTime)),
				FMath::Max(JigenPlayRate, KINDA_SMALL_NUMBER),
				EMontagePlayReturnType::MontageLength,
				SectionStartTime,
				true
			);
			AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJigenMontageEnded);

			if (MontagePlayResult > 0.f)
			{
				return;
			}
		}

		AnimInstance->Montage_JumpToSection(JigenSecondSectionName, JigenMontage);
		return;
	}

	bBufferedJigenComboInput = false;
	bCanBufferJigenComboInput = false;
	bJigenComboAdvanceStateActive = false;
}

void AJunPlayer::FinishJigen()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJigenAttacking = false;
	JigenComboIndex = 0;
	bCanBufferJigenComboInput = false;
	bBufferedJigenComboInput = false;
	bBufferedJigenInput = false;
	bJigenComboAdvanceStateActive = false;
	JigenSectionElapsedTime = 0.f;
	BufferedJigenRecoveryAction = EJunBufferedRecoveryAction::None;

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}
}

bool AJunPlayer::CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction Action) const
{
	if (!bIsJigenAttacking)
	{
		return false;
	}

	float CancelOpenTime = 0.f;
	switch (Action)
	{
	case EJunBufferedRecoveryAction::Dodge:
		CancelOpenTime = JigenDodgeCancelOpenTime;
		break;
	case EJunBufferedRecoveryAction::Jump:
		CancelOpenTime = JigenJumpCancelOpenTime;
		break;
	case EJunBufferedRecoveryAction::Defense:
		CancelOpenTime = JigenDefenseCancelOpenTime;
		break;
	default:
		return false;
	}

	return JigenSectionElapsedTime >= CancelOpenTime;
}

void AJunPlayer::BufferJigenRecoveryAction(EJunBufferedRecoveryAction Action)
{
	BufferedJigenRecoveryAction = Action;
	TryExecuteBufferedJigenRecoveryAction();
}

void AJunPlayer::TryExecuteBufferedJigenRecoveryAction()
{
	if (BufferedJigenRecoveryAction == EJunBufferedRecoveryAction::None ||
		!CanCancelJigenIntoRecoveryAction(BufferedJigenRecoveryAction))
	{
		return;
	}

	const EJunBufferedRecoveryAction PendingAction = BufferedJigenRecoveryAction;
	BufferedJigenRecoveryAction = EJunBufferedRecoveryAction::None;

	switch (PendingAction)
	{
	case EJunBufferedRecoveryAction::Dodge:
		CancelJigenForRecoveryTransition(JigenDodgeCancelBlendOutTime);
		StartDodge();
		break;
	case EJunBufferedRecoveryAction::Jump:
		CancelJigenForRecoveryTransition(JigenJumpCancelBlendOutTime);
		Jump();
		break;
	case EJunBufferedRecoveryAction::Defense:
		CancelJigenForRecoveryTransition(JigenDefenseCancelBlendOutTime);
		BeginDefenseFromBufferedInput();
		break;
	default:
		break;
	}
}

void AJunPlayer::CancelJigenForRecoveryTransition(float BlendOutTime)
{
	CancelJigen(BlendOutTime);
}

bool AJunPlayer::CanCancelJigenIntoMove() const
{
	return bIsJigenAttacking && JigenSectionElapsedTime >= JigenMoveCancelOpenTime;
}

bool AJunPlayer::TryCancelJigenIntoMove()
{
	if (!CanCancelJigenIntoMove())
	{
		return false;
	}

	CancelJigenForRecoveryTransition(JigenMoveCancelBlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelJigenIntoBasicAttack() const
{
	return bIsJigenAttacking && JigenSectionElapsedTime >= JigenBasicAttackCancelOpenTime;
}

bool AJunPlayer::TryCancelJigenIntoBasicAttack()
{
	if (!CanCancelJigenIntoBasicAttack())
	{
		return false;
	}

	CancelJigenForRecoveryTransition(JigenBasicAttackCancelBlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::CanCancelJigenIntoHeavyAttack() const
{
	return bIsJigenAttacking && JigenSectionElapsedTime >= JigenHeavyAttackCancelOpenTime;
}

void AJunPlayer::CancelJigen(float BlendOutTime)
{
	if (!bIsJigenAttacking)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StopAttackTrace();
	}

	if (AnimInstance && JigenMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, JigenMontage);
	}

	FinishJigen();
}

void AJunPlayer::OnJigenMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != JigenMontage)
	{
		return;
	}

	if (bInterrupted && AnimInstance && AnimInstance->Montage_IsPlaying(JigenMontage))
	{
		return;
	}

	FinishJigen();
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

void AJunPlayer::OnJumpCounterStompFollowUpMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != JumpCounterStompFollowUpMontage)
	{
		return;
	}

	FinishJumpCounterStompFollowUp(false);
}

void AJunPlayer::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeMontage)
	{
		return;
	}

	if (bInterrupted && AnimInstance && Montage && AnimInstance->Montage_IsPlaying(Montage))
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

void AJunPlayer::OnDrinkPotionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != DrinkMontage)
	{
		return;
	}

	FinishDrinkPotion(bInterrupted);
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
	const auto ResolveForwardDashMontage = [this]() -> UAnimMontage*
	{
		return DashForwardMontage ? DashForwardMontage.Get() : DodgeMontage.Get();
	};
	const auto ResolveLeftDashMontage = [this, &ResolveForwardDashMontage]() -> UAnimMontage*
	{
		return DashLeftMontage ? DashLeftMontage.Get() : ResolveForwardDashMontage();
	};
	const auto ResolveRightDashMontage = [this, &ResolveForwardDashMontage]() -> UAnimMontage*
	{
		return DashRightMontage ? DashRightMontage.Get() : ResolveForwardDashMontage();
	};

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
		return bLockOnActive && BackDashMontage
			? BackDashMontage.Get()
			: ResolveForwardDashMontage();
	}

	if (!bLockOnActive)
	{
		return ResolveForwardDashMontage();
	}

	const float ForwardAbs = FMath::Abs(DodgeInput.X);
	const float RightAbs = FMath::Abs(DodgeInput.Y);

	constexpr float DiagonalInputThreshold = 0.25f;
	const bool bHasForwardInput = ForwardAbs >= DiagonalInputThreshold;
	const bool bHasRightInput = RightAbs >= DiagonalInputThreshold;

	if (bHasForwardInput && bHasRightInput)
	{
		if (DodgeInput.X >= 0.f)
		{
			if (DodgeInput.Y >= 0.f)
			{
				return LockOnDashForwardRightMontage ? LockOnDashForwardRightMontage.Get() : ResolveRightDashMontage();
			}

			return LockOnDashForwardLeftMontage ? LockOnDashForwardLeftMontage.Get() : ResolveLeftDashMontage();
		}

		if (DodgeInput.Y >= 0.f)
		{
			return LockOnDashBackRightMontage ? LockOnDashBackRightMontage.Get() : (BackDashMontage ? BackDashMontage.Get() : ResolveRightDashMontage());
		}

		return LockOnDashBackLeftMontage ? LockOnDashBackLeftMontage.Get() : (BackDashMontage ? BackDashMontage.Get() : ResolveLeftDashMontage());
	}

	if (RightAbs >= ForwardAbs)
	{
		return DodgeInput.Y >= 0.f
			? ResolveRightDashMontage()
			: ResolveLeftDashMontage();
	}

	return DodgeInput.X >= 0.f
		? ResolveForwardDashMontage()
		: (BackDashMontage ? BackDashMontage.Get() : ResolveForwardDashMontage());
}

bool AJunPlayer::IsDashDodgeMontage(const UAnimMontage* Montage) const
{
	return IsForwardDashMontage(Montage) ||
		IsBackDashMontage(Montage) ||
		IsSideDashMontage(Montage);
}

bool AJunPlayer::IsForwardDashMontage(const UAnimMontage* Montage) const
{
	return Montage &&
		(Montage == DashForwardMontage ||
			Montage == LockOnDashForwardLeftMontage ||
			Montage == LockOnDashForwardRightMontage);
}

bool AJunPlayer::IsBackDashMontage(const UAnimMontage* Montage) const
{
	return Montage &&
		(Montage == BackDashMontage ||
			Montage == LockOnDodgeBackMontage ||
			Montage == LockOnDashBackLeftMontage ||
			Montage == LockOnDashBackRightMontage);
}

bool AJunPlayer::IsSideDashMontage(const UAnimMontage* Montage) const
{
	return Montage &&
		(Montage == DashLeftMontage ||
			Montage == DashRightMontage);
}

float AJunPlayer::GetDodgePlayRateForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return 1.f;
	}

	if (IsForwardDashMontage(Montage))
	{
		return FMath::Max(DashForwardPlayRate, KINDA_SMALL_NUMBER);
	}

	if (IsBackDashMontage(Montage))
	{
		return FMath::Max(DashBackPlayRate, KINDA_SMALL_NUMBER);
	}

	if (IsSideDashMontage(Montage))
	{
		return FMath::Max(DashSidePlayRate, KINDA_SMALL_NUMBER);
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

	if (IsForwardDashMontage(Montage))
	{
		return DashForwardRecoveryCancelOpenTime;
	}

	if (IsBackDashMontage(Montage))
	{
		return DashBackRecoveryCancelOpenTime;
	}

	if (IsSideDashMontage(Montage))
	{
		return DashSideRecoveryCancelOpenTime;
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

	if (IsForwardDashMontage(Montage))
	{
		return DashForwardMoveCancelOpenTime;
	}

	if (IsBackDashMontage(Montage))
	{
		return DashBackMoveCancelOpenTime;
	}

	if (IsSideDashMontage(Montage))
	{
		return DashSideMoveCancelOpenTime;
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
	bPendingDeathPresentationIsRealDeath = false;
	ApplyDeathControlLock();
	NotifyBossPlayerFakeDeathStarted();

	if (!TryStartAirDeathSequence() && FakeDeathMontage)
	{
		PlayAnimMontage(FakeDeathMontage);
	}
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
	bPendingDeathPresentationIsRealDeath = true;

	if (!TryStartAirDeathSequence() && DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}

bool AJunPlayer::TryStartAirDeathSequence()
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		return false;
	}

	if (GetDistanceFromGround() < AirDeathMinGroundDistance)
	{
		return false;
	}

	if (!AirHitFHeavyMontage && !AirHitFHeavyDownMontage && !GetAirDeathLandMontage())
	{
		return false;
	}

	StartAirDeathHitFallSequence();
	return true;
}

void AJunPlayer::StartAirDeathHitFallSequence()
{
	InterruptActionsForHitReaction();
	ResetAirHeavyHitReactState();

	bAirDeathSequenceActive = true;
	CurrentHitState = EJunPlayerHitState::HitReact;
	CurrentHitReactType = EHitReactType::HeavyHit_A;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	bCurrentHitReactUsesAirMontage = true;
	CurrentHitReactMontage = AirHitFHeavyMontage;

	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	bFreeRunSlideActive = false;
	FreeRunSlideSpeed = 0.f;
	ConsumeMovementInputVector();

	AirHeavyHitStage = EJunAirHeavyHitStage::Launch;
	const float LaunchMontageDuration = AirHitFHeavyMontage
		? AirHitFHeavyMontage->GetPlayLength()
		: AirHeavyHitReactDuration;
	AirHeavyHitStageRemainTime = AirHeavyHitLaunchStageMaxDuration > 0.f
		? FMath::Min(LaunchMontageDuration, AirHeavyHitLaunchStageMaxDuration)
		: LaunchMontageDuration;

	PlayerHitStateRemainTime = FMath::Max(AirHeavyHitSequenceMaxDuration, AirHeavyHitStageRemainTime);
	PlayerHitControlLockRemainTime = FMath::Max(AirHeavyHitControlLockDuration, PlayerHitStateRemainTime);

	ApplyAirHeavyHitFallTuning();
	ApplyAirHitKnockback(EJunAirHitReactType::Heavy);

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	if (AirHitFHeavyMontage)
	{
		PlayAnimMontage(AirHitFHeavyMontage);
	}
	else
	{
		StartAirHeavyHitDownStage();
	}
}

UAnimMontage* AJunPlayer::GetAirDeathLandMontage() const
{
	if (DeathState == EJunPlayerDeathState::FakeDeath)
	{
		return AirFakeDeathMontage ? AirFakeDeathMontage.Get() : FakeDeathMontage.Get();
	}

	if (DeathState == EJunPlayerDeathState::RealDeath)
	{
		return AirDeathMontage ? AirDeathMontage.Get() : DeathMontage.Get();
	}

	return nullptr;
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
		if (AirHitFHeavyMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFHeavyMontage);
		}
		if (AirHitFHeavyDownMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFHeavyDownMontage);
		}
		if (AirHitFHeavyLandMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFHeavyLandMontage);
		}
		if (AirHitFLightMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFLightMontage);
		}

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
	LandingAnimSuppressRemainTime = 0.25f;
	CurrentPosture = 0.f;
	bDeathPresentationStarted = false;
	bAirDeathSequenceActive = false;
	bAirGuardBreakLandMontagePending = false;
	ClearHitReactRuntimeStateForRevive();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	ClearDeathControlLock();
	EndLockOn();
	NotifyBossPlayerRevivedFromFakeDeath();

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->SetLockOnMarkerSuppressed(false);
		JunPlayerController->ResetPlayerPostureVisibilityState();
		JunPlayerController->HideDeathUI();
	}

	if (AJunCharacter* ReviveLockOnTarget = FindBestLockOnTarget())
	{
		PendingFakeDeathReviveLockOnTarget = ReviveLockOnTarget;
		ScheduleFakeDeathReviveAutoLockOn();
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
	FinishDrinkPotion(true);
	InterruptActionsForHitReaction();
	ClearCombatInputBuffers();
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

void AJunPlayer::ClearCombatInputBuffers()
{
	bBufferedBasicAttackInput = false;
	bBufferedBasicAttackComboInput = false;
	bCanBufferBasicAttackComboInput = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	BufferedBasicAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	PostBasicAttackJumpDodgeBufferRemainTime = 0.f;
	PostBasicAttackDefenseBufferRemainTime = 0.f;

	bBufferedHeavyAttackInput = false;
	bBufferedHeavyAttackComboInput = false;
	bCanBufferHeavyAttackComboInput = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	BufferedHeavyAttackRecoveryAction = EJunBufferedRecoveryAction::None;
	ResetHeavyAttackChargeInput();

	bBufferedJigenInput = false;
	bBufferedJigenComboInput = false;
	bCanBufferJigenComboInput = false;
	bJigenComboAdvanceStateActive = false;
	BufferedJigenRecoveryAction = EJunBufferedRecoveryAction::None;

	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	bParrySuccessHeavyAttackInputHeld = false;
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
	ClearFakeDeathReviveAutoLockOn();

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		if (AirHitFHeavyMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFHeavyMontage);
		}
		if (AirHitFHeavyDownMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFHeavyDownMontage);
		}
		if (AirHitFHeavyLandMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFHeavyLandMontage);
		}
		if (AirHitFLightMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirHitFLightMontage);
		}
		if (FakeDeathMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, FakeDeathMontage);
		}
		if (DeathMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, DeathMontage);
		}
		if (AirFakeDeathMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirFakeDeathMontage);
		}
		if (AirDeathMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, AirDeathMontage);
		}
		if (FakeDeathGetUpMontage)
		{
			PlayerAnimInstance->Montage_Stop(0.f, FakeDeathGetUpMontage);
		}
	}

	Hp = MaxHp;
	CurrentReviveCount = FMath::Max(0, MaxReviveCount);
	RefillDrinkPotionCharges();
	CurrentPosture = 0.f;
	DeathState = EJunPlayerDeathState::Alive;
	LandingAnimSuppressRemainTime = 0.25f;
	RealDeathHoldRemainTime = 0.f;
	RealDeathFullBlackHoldRemainTime = -1.f;
	bRealDeathBlackFadeStarted = false;
	bDeathPresentationStarted = false;
	bAirDeathSequenceActive = false;
	bAirGuardBreakLandMontagePending = false;
	ClearHitReactRuntimeStateForRevive();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_None);
	}

	AJunPlayerRespawnPoint* RespawnPoint = nullptr;
	APlayerStart* PlayerStart = nullptr;
	if (UWorld* World = GetWorld())
	{
		RespawnPoint = AJunPlayerRespawnPoint::FindPlayerRespawnPoint(
			this,
			EJunPlayerRespawnPointType::BossRespawn);

		if (!RespawnPoint)
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				PlayerStart = *It;
				break;
			}
		}
	}

	const FRotator SpawnViewRotation = RespawnPoint
		? RespawnPoint->GetActorRotation()
		: (PlayerStart ? PlayerStart->GetActorRotation() : GetActorRotation());

	if (RespawnPoint)
	{
		SetActorLocationAndRotation(
			RespawnPoint->GetActorLocation(),
			SpawnViewRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}
	else if (PlayerStart)
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
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	EndLockOn();
	ResetCameraToSpawnView(SpawnViewRotation);
	StartCameraHardSnap(3);
	ClearDeathControlLock();

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->SetLockOnMarkerSuppressed(false);
		JunPlayerController->ResetPlayerPostureVisibilityState();
		JunPlayerController->HideDeathUI();
	}
}

void AJunPlayer::ScheduleFakeDeathReviveAutoLockOn()
{
	if (!PendingFakeDeathReviveLockOnTarget.IsValid())
	{
		ClearFakeDeathReviveAutoLockOn();
		return;
	}

	FakeDeathReviveAutoLockOnRemainTime = FMath::Max(0.f, FakeDeathReviveAutoLockOnDelay);
	if (FakeDeathReviveAutoLockOnRemainTime <= 0.f)
	{
		AJunCharacter* LockOnCharacter = PendingFakeDeathReviveLockOnTarget.Get();
		ClearFakeDeathReviveAutoLockOn();
		StartLockOnAfterFakeDeathRevive(LockOnCharacter);
	}
}

void AJunPlayer::UpdateFakeDeathReviveAutoLockOn(float DeltaTime)
{
	if (FakeDeathReviveAutoLockOnRemainTime <= 0.f)
	{
		return;
	}

	FakeDeathReviveAutoLockOnRemainTime = FMath::Max(0.f, FakeDeathReviveAutoLockOnRemainTime - DeltaTime);
	if (FakeDeathReviveAutoLockOnRemainTime > 0.f)
	{
		return;
	}

	AJunCharacter* LockOnCharacter = PendingFakeDeathReviveLockOnTarget.Get();
	ClearFakeDeathReviveAutoLockOn();
	if (!LockOnCharacter || !IsValid(LockOnCharacter))
	{
		return;
	}

	StartLockOnAfterFakeDeathRevive(LockOnCharacter);
}

void AJunPlayer::ClearFakeDeathReviveAutoLockOn()
{
	FakeDeathReviveAutoLockOnRemainTime = 0.f;
	PendingFakeDeathReviveLockOnTarget = nullptr;
}

void AJunPlayer::StartLockOnAfterFakeDeathRevive(AJunCharacter* NewTarget)
{
	if (!NewTarget)
	{
		return;
	}

	StartLockOn(NewTarget);

	CachedLockOnTargetPoint = GetRawLockOnCapsulePoint();
	if (LockOnTarget)
	{
		const FVector TargetLockOnPoint = LockOnTarget->GetLockOnTargetPoint();
		CachedLockOnTargetPoint.Z = TargetLockOnPoint.Z;
	}

	SmoothedLockOnDistance2D = FVector::Dist2D(GetActorLocation(), NewTarget->GetActorLocation());
	const float FarPitchEnterDistance = FMath::Max(0.f, LockOnFarPitchEnterDistance);
	bLockOnFarPitchModeActive = SmoothedLockOnDistance2D >= FarPitchEnterDistance;
	CachedLockOnRangeAlpha = FMath::Clamp(
		(SmoothedLockOnDistance2D - LockOnCloseDistance) / FMath::Max(LockOnFarDistance - LockOnCloseDistance, 1.f),
		0.f,
		1.f);
	if (bLockOnFarPitchModeActive)
	{
		CachedLockOnRangeAlpha = 1.f;
	}

	bLockOnClosePitchModeActive = SmoothedLockOnDistance2D <= FMath::Max(0.f, LockOnClosePitchEnterDistance);
	const bool bUseJumpCounterPitch = ShouldUseJumpCounterLockOnCameraPitch();
	const AJunMonster* LockOnMonster = Cast<AJunMonster>(LockOnTarget.Get());
	const bool bTargetExecutionReady = LockOnMonster && LockOnMonster->IsExecutionReady();
	SmoothedLockOnPitchOffset = bTargetExecutionReady
		? ExecutionReadyLockOnPitchOffset
		: (bUseJumpCounterPitch
			? JumpCounterLockOnPitchOffset
			: (bLockOnFarPitchModeActive
				? LockOnPitchOffset
				: (bLockOnClosePitchModeActive
					? LockOnClosePitchOffset
					: FMath::Lerp(LockOnClosePitchOffset, LockOnPitchOffset, CachedLockOnRangeAlpha))));
}

void AJunPlayer::ClearHitReactRuntimeStateForRevive()
{
	CurrentHitState = EJunPlayerHitState::None;
	PlayerHitStateRemainTime = 0.f;
	PlayerHitControlLockRemainTime = 0.f;
	ChainParryWindowRemainTime = 0.f;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = false;
	bAirGuardBreakLandMontagePending = false;
	ResetAirHeavyHitReactState();
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	bParrySuccessHeavyAttackInputHeld = false;
	CurrentParrySuccessMontage = nullptr;
	bGuardBlockReleasePending = false;
	RestoreDefaultMovementBrakingSettings();
	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
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
	UpdateGuardRestartBlend(DeltaTime);

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
			if (bAirParrySequenceActive)
			{
				FinishDefense();
				return;
			}

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

	if (ParrySpamPenaltyResetRemainTime > 0.f)
	{
		ParrySpamPenaltyResetRemainTime = FMath::Max(0.f, ParrySpamPenaltyResetRemainTime - DeltaTime);

		if (ParrySpamPenaltyResetRemainTime <= 0.f)
		{
			ParrySpamPenaltyStack = 0;
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

EJunPlayerHitResolveResult AJunPlayer::ResolveIncomingHitResult(EHitReactType IncomingHitType, const AActor* DamageCauser) const
{
	if (LastIncomingDefenseRuleData.bCanBeDodgedByInvincibility &&
		HasGameplayTag(JunGameplayTags::State_Condition_Invincible))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	const bool bCanDefendFromIncomingAngle = IsDamageCauserInDefenseAngle(DamageCauser);

	if (LastIncomingDefenseRuleData.bCanBeParried && bParryWindowOpen && bCanDefendFromIncomingAngle)
	{
		const float PerfectParryRemainThreshold =
			FMath::Max(0.f, CurrentParryWindowDuration - CurrentPerfectParryWindowDuration);
		return ParryWindowRemainTime >= PerfectParryRemainThreshold
			? EJunPlayerHitResolveResult::PerfectParrySuccess
			: EJunPlayerHitResolveResult::NormalParrySuccess;
	}

	if (LastIncomingDefenseRuleData.bCanBeGuarded && DefenseState == EJunDefenseState::Guarding && bCanDefendFromIncomingAngle)
	{
		return EJunPlayerHitResolveResult::GuardBlock;
	}

	if (!CanBeInterruptedBy(IncomingHitType))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	return EJunPlayerHitResolveResult::HitReact;
}

bool AJunPlayer::IsDamageCauserInDefenseAngle(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return true;
	}

	const FVector ToDamageCauser = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToDamageCauser.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, ToDamageCauser);
	const float HalfAngle = FMath::Clamp(DefenseFrontAngle * 0.5f, 0.f, 180.f);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(HalfAngle));
	return Dot >= MinDot;
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

	if (IncomingHitType == EHitReactType::LightHit ||
		IsLargeHitType(IncomingHitType) ||
		IsHeavyHitType(IncomingHitType) ||
		IncomingHitType == EHitReactType::Lighting_Shock)
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

ECharacterHitReactDirection AJunPlayer::DetermineHitReactDirection(
	const AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	const ECharacterHitReactDirection AutoDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	const bool bIsFrontDirection =
		AutoDirection == ECharacterHitReactDirection::Front_F ||
		AutoDirection == ECharacterHitReactDirection::Front_FL ||
		AutoDirection == ECharacterHitReactDirection::Front_FR;

	if (!bIsFrontDirection)
	{
		return AutoDirection;
	}

	switch (DefenseRuleData.FrontHitReactDirectionOverride)
	{
	case EJunFrontHitReactDirectionOverride::Front_FL:
		return ECharacterHitReactDirection::Front_FL;
	case EJunFrontHitReactDirectionOverride::Front_FR:
		return ECharacterHitReactDirection::Front_FR;
	case EJunFrontHitReactDirectionOverride::Front_F:
		return ECharacterHitReactDirection::Front_F;
	case EJunFrontHitReactDirectionOverride::Auto:
	default:
		return AutoDirection;
	}
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
	case EHitReactType::Lighting_Shock:
		return LightingShockMontage;
	default:
		return nullptr;
	}
}

EJunAirHitReactType AJunPlayer::ResolveAirHitReactType() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		return EJunAirHitReactType::None;
	}

	const EJunAirHitReactType RequestedAirHitReactType = LastIncomingDefenseRuleData.AirHitReactType;
	if (RequestedAirHitReactType == EJunAirHitReactType::Heavy &&
		GetDistanceFromGround() < AirHeavyHitMinGroundDistance)
	{
		return EJunAirHitReactType::None;
	}

	return GetAirHitReactMontage(RequestedAirHitReactType) ? RequestedAirHitReactType : EJunAirHitReactType::None;
}

UAnimMontage* AJunPlayer::GetAirHitReactMontage(EJunAirHitReactType AirHitReactType) const
{
	switch (AirHitReactType)
	{
	case EJunAirHitReactType::Light:
		return AirHitFLightMontage;
	case EJunAirHitReactType::Heavy:
		return AirHitFHeavyMontage;
	case EJunAirHitReactType::None:
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

	if (bUseLeft)
	{
		bLastParrySuccessUsedLeftSide = ParrySuccessL_UpMontage != nullptr || ParrySuccessR_UpMontage == nullptr;
		return ParrySuccessL_UpMontage ? ParrySuccessL_UpMontage : ParrySuccessR_UpMontage;
	}

	bLastParrySuccessUsedLeftSide = ParrySuccessR_UpMontage == nullptr && ParrySuccessL_UpMontage != nullptr;
	return ParrySuccessR_UpMontage ? ParrySuccessR_UpMontage : ParrySuccessL_UpMontage;
}

UAnimMontage* AJunPlayer::GetAirParrySuccessMontage()
{
	if (!bHasSelectedInitialParrySuccessSide)
	{
		bNextParrySuccessUsesLeftSide = FMath::RandBool();
		bHasSelectedInitialParrySuccessSide = true;
	}

	const bool bUseLeft = bNextParrySuccessUsesLeftSide;
	bNextParrySuccessUsesLeftSide = !bNextParrySuccessUsesLeftSide;

	if (bUseLeft)
	{
		bLastParrySuccessUsedLeftSide = AirParrySuccessL_UpMontage != nullptr ||
			(AirParrySuccessR_UpMontage == nullptr && ParrySuccessL_UpMontage != nullptr);
		return AirParrySuccessL_UpMontage
			? AirParrySuccessL_UpMontage.Get()
			: (AirParrySuccessR_UpMontage ? AirParrySuccessR_UpMontage.Get() : GetParrySuccessMontage(FVector::ZeroVector));
	}

	bLastParrySuccessUsedLeftSide = AirParrySuccessR_UpMontage == nullptr &&
		(AirParrySuccessL_UpMontage != nullptr || ParrySuccessR_UpMontage == nullptr);
	return AirParrySuccessR_UpMontage
		? AirParrySuccessR_UpMontage.Get()
		: (AirParrySuccessL_UpMontage ? AirParrySuccessL_UpMontage.Get() : GetParrySuccessMontage(FVector::ZeroVector));
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

void AJunPlayer::ReducePosture(float Amount)
{
	if (MaxPosture <= 0.f || Amount <= 0.f)
	{
		return;
	}

	CurrentPosture = FMath::Max(0.f, CurrentPosture - Amount);
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
	const bool bRunRecoveryPenalty = GetMoveState() == EJunMoveState::Run;
	const float RecoveryMultiplier = bGuardRecoveryBoost
		? GuardPostureRecoveryMultiplier
		: (bRunRecoveryPenalty ? RunPostureRecoveryMultiplier : 1.f);
	const float RecoveryAmount = PostureRecoveryRate * RecoveryMultiplier * DeltaTime;
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

	if (bIsBasicAttacking || bIsHeavyAttacking || bIsJigenAttacking || bIsJumpAttacking || bIsDodgeAttacking)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return false;
	}

	return true;
}

bool AJunPlayer::TryOpenMikiriCounterWindow(bool bRequireForwardInput)
{
	if (bMikiriCounterWindowOpen || MikiriCounterReadyRemainTime > 0.f || CurrentMikiriParryReadyMontage)
	{
		return true;
	}

	if (DeathState != EJunPlayerDeathState::Alive ||
		Is_Dead() ||
		bIsExecuting ||
		IsInDeathSequence() ||
		(GetCharacterMovement() && GetCharacterMovement()->IsFalling()))
	{
		return false;
	}

	if (bRequireForwardInput &&
		(HasGameplayTag(JunGameplayTags::State_Block_Dodge) ||
			HasGameplayTag(JunGameplayTags::State_Action_Dodge)))
	{
		return false;
	}

	const float ForwardInput = FMath::Max(DesiredMoveForward, PendingMoveForward);
	if (bRequireForwardInput && ForwardInput < MikiriCounterForwardInputThreshold)
	{
		return false;
	}

	if (!IsMikiriCounterThreatAvailable())
	{
		return false;
	}

	if (!bRequireForwardInput && HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (AnimInstance && CurrentDodgeMontage)
		{
			AnimInstance->Montage_Stop(FMath::Max(0.f, MikiriCounterDodgeBlendOutTime), CurrentDodgeMontage);
		}
		FinishDodgeState();
		RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	}

	CurrentMikiriParryReadyMontage = GetMikiriParryReadyMontage(GetMikiriCounterReferenceActor());
	const float ReadyDuration = CurrentMikiriParryReadyMontage
		? FMath::Max(MikiriCounterReadyMinDuration, CurrentMikiriParryReadyMontage->GetPlayLength())
		: MikiriCounterReadyMinDuration;

	bMikiriCounterWindowOpen = true;
	MikiriCounterWindowRemainTime = FMath::Max(MikiriCounterWindowDuration, KINDA_SMALL_NUMBER);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	ConsumeMovementInputVector();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	if (CurrentMikiriParryReadyMontage)
	{
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.f, CurrentMikiriParryReadyMontage);
		}

		PlayAnimMontage(CurrentMikiriParryReadyMontage);
	}
	MikiriCounterReadyRemainTime = ReadyDuration;

	return true;
}

bool AJunPlayer::IsMikiriCounterThreatAvailable() const
{
	if (LockOnTarget && LockOnTarget->IsMikiriCounterThreatActive())
	{
		return true;
	}

	const UWorld* World = GetWorld();
	if (!World || MikiriCounterThreatSearchRadius <= 0.f)
	{
		return false;
	}

	const float SearchRadiusSq = FMath::Square(MikiriCounterThreatSearchRadius);
	for (TActorIterator<AJunCharacter> CharacterIt(World); CharacterIt; ++CharacterIt)
	{
		const AJunCharacter* OtherCharacter = *CharacterIt;
		if (!OtherCharacter ||
			OtherCharacter == this ||
			!OtherCharacter->IsMikiriCounterThreatActive() ||
			!IsEnemyTo(OtherCharacter))
		{
			continue;
		}

		if (FVector::DistSquared2D(GetActorLocation(), OtherCharacter->GetActorLocation()) <= SearchRadiusSq)
		{
			return true;
		}
	}

	return false;
}

AActor* AJunPlayer::GetMikiriCounterReferenceActor() const
{
	if (LockOnTarget && LockOnTarget->IsMikiriCounterThreatActive())
	{
		return LockOnTarget.Get();
	}

	const UWorld* World = GetWorld();
	if (!World || MikiriCounterThreatSearchRadius <= 0.f)
	{
		return nullptr;
	}

	const float SearchRadiusSq = FMath::Square(MikiriCounterThreatSearchRadius);
	AActor* BestActor = nullptr;
	float BestDistanceSq = SearchRadiusSq;
	for (TActorIterator<AJunCharacter> CharacterIt(World); CharacterIt; ++CharacterIt)
	{
		AJunCharacter* OtherCharacter = *CharacterIt;
		if (!OtherCharacter ||
			OtherCharacter == this ||
			!OtherCharacter->IsMikiriCounterThreatActive() ||
			!IsEnemyTo(OtherCharacter))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), OtherCharacter->GetActorLocation());
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestActor = OtherCharacter;
		}
	}

	return BestActor;
}

void AJunPlayer::UpdateMikiriCounterWindow(float DeltaTime)
{
	if (MikiriCounterReadyRemainTime > 0.f)
	{
		MikiriCounterReadyRemainTime = FMath::Max(0.f, MikiriCounterReadyRemainTime - DeltaTime);
		if (MikiriCounterReadyRemainTime <= 0.f)
		{
			FinishMikiriCounterReady();
			return;
		}
	}

	if (!bMikiriCounterWindowOpen)
	{
		return;
	}

	MikiriCounterWindowRemainTime = FMath::Max(0.f, MikiriCounterWindowRemainTime - DeltaTime);
	if (MikiriCounterWindowRemainTime <= 0.f)
	{
		FinishMikiriCounterReady();
	}
}

bool AJunPlayer::TryHandleMikiriCounter(AActor* DamageCauser)
{
	if (!bMikiriCounterWindowOpen)
	{
		return false;
	}

	if (!IsDamageCauserInDefenseAngle(DamageCauser))
	{
		FinishMikiriCounterReady();
		return false;
	}

	AJunMonster* AttackingMonster = Cast<AJunMonster>(DamageCauser);
	if (!AttackingMonster || !AttackingMonster->NotifyMikiriCounteredBy(this))
	{
		FinishMikiriCounterReady();
		return false;
	}

	bMikiriCounterWindowOpen = false;
	MikiriCounterWindowRemainTime = 0.f;
	PlayDefenseSound(EJunDefenseSoundType::PerfectParry);
	PlayDefenseCameraShake(MikiriParryCameraShakeScale);
	StartMikiriCounterSuccess(DamageCauser);
	return true;
}

void AJunPlayer::StartMikiriCounterSuccess(AActor* DamageCauser)
{
	ResetParrySpamPenalty();

	InterruptActionsForHitReaction();
	PlayDefenseEffect(
		CachedParryParticleComponent,
		ParryParticleComponentName,
		CachedParryEffectLocationComponent,
		ParryEffectLocationComponentName);
	if (AnimInstance && CurrentMikiriParryReadyMontage)
	{
		AnimInstance->Montage_Stop(0.05f, CurrentMikiriParryReadyMontage);
	}
	CurrentMikiriParryReadyMontage = nullptr;
	MikiriCounterReadyRemainTime = 0.f;
	bMikiriCounterWindowOpen = false;
	MikiriCounterWindowRemainTime = 0.f;

	CurrentHitState = EJunPlayerHitState::ParrySuccess;
	ChainParryWindowRemainTime = ChainParryWindowDuration;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	bParrySuccessHeavyAttackInputHeld = false;
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	CurrentParrySuccessMontage = GetMikiriParryMontage(DamageCauser);
	PlayerHitStateRemainTime = CurrentParrySuccessMontage
		? CurrentParrySuccessMontage->GetPlayLength()
		: ParrySuccessDuration;

	if (CurrentParrySuccessMontage)
	{
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.f, CurrentParrySuccessMontage);
		}

		PlayAnimMontage(CurrentParrySuccessMontage);
	}
}

void AJunPlayer::FinishMikiriCounterReady()
{
	bMikiriCounterWindowOpen = false;
	MikiriCounterWindowRemainTime = 0.f;
	MikiriCounterReadyRemainTime = 0.f;
	CurrentMikiriParryReadyMontage = nullptr;
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
}

UAnimMontage* AJunPlayer::GetMikiriParryMontage(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return MikiriParryMontage.Get();
	}

	const FVector ToPlayer = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
	if (ToPlayer.IsNearlyZero())
	{
		return MikiriParryMontage.Get();
	}

	const float RightDot = FVector::DotProduct(DamageCauser->GetActorRightVector().GetSafeNormal2D(), ToPlayer);
	if (RightDot > 0.f && MikiriParryRMontage)
	{
		return MikiriParryRMontage.Get();
	}

	return MikiriParryMontage.Get();
}

UAnimMontage* AJunPlayer::GetMikiriParryReadyMontage(const AActor* ReferenceActor) const
{
	if (!ReferenceActor)
	{
		return MikiriParryReadyMontage.Get();
	}

	const FVector ToPlayer = (GetActorLocation() - ReferenceActor->GetActorLocation()).GetSafeNormal2D();
	if (ToPlayer.IsNearlyZero())
	{
		return MikiriParryReadyMontage.Get();
	}

	const float RightDot = FVector::DotProduct(ReferenceActor->GetActorRightVector().GetSafeNormal2D(), ToPlayer);
	if (RightDot > 0.f && MikiriParryReadyRMontage)
	{
		return MikiriParryReadyRMontage.Get();
	}

	return MikiriParryReadyMontage.Get();
}

void AJunPlayer::StartParrySuccess()
{
	ResetParrySpamPenalty();

	InterruptActionsForHitReaction();
	PlayDefenseEffect(
		CachedParryParticleComponent,
		ParryParticleComponentName,
		CachedParryEffectLocationComponent,
		ParryEffectLocationComponentName);

	CurrentHitState = EJunPlayerHitState::ParrySuccess;
	ChainParryWindowRemainTime = ChainParryWindowDuration;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	bParrySuccessHeavyAttackInputHeld = false;
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

	const bool bUseAirParrySuccessMontage =
		bAirParrySequenceActive ||
		(GetCharacterMovement() && GetCharacterMovement()->IsFalling());
	UAnimMontage* ParrySuccessMontage = bUseAirParrySuccessMontage
		? GetAirParrySuccessMontage()
		: GetParrySuccessMontage(LastIncomingSwingDirection);
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
	PlayDefenseEffect(
		CachedDeflectParticleComponent,
		DeflectParticleComponentName,
		CachedDeflectEffectLocationComponent,
		DeflectEffectLocationComponentName);

	bGuardBlockReleasePending = false;
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
	const bool bRestartingGuardBreak = CurrentHitState == EJunPlayerHitState::GuardBreak;

	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterStompOpportunityPending = false;
	JumpCounterStompTarget = nullptr;
	JumpCounterStompPendingTarget = nullptr;

	InterruptActionsForHitReaction();
	PlayDefenseEffect(
		CachedGuardBreakParticleComponent,
		GuardBreakParticleComponentName,
		CachedGuardBreakEffectLocationComponent,
		GuardBreakEffectLocationComponentName);

	const bool bUseLightingShockGuardBreak =
		LastIncomingHitReactType == EHitReactType::Lighting_Shock &&
		LightingShockMontage != nullptr;
	const bool bUseAirGuardBreak =
		GetCharacterMovement() &&
		GetCharacterMovement()->IsFalling() &&
		AirGuardBreakMontage &&
		!bUseLightingShockGuardBreak;
	UAnimMontage* GuardBreakMontageToPlay = bUseLightingShockGuardBreak
		? LightingShockMontage.Get()
		: (bUseAirGuardBreak
		? AirGuardBreakMontage.Get()
		: GuardBreakMontage.Get());

	CurrentPosture = 0.f;
	PostureRecoveryDelayRemainTime = 0.f;
	CurrentHitState = EJunPlayerHitState::GuardBreak;
	PlayerHitStateRemainTime = bUseAirGuardBreak
		? FMath::Max(AirGuardBreakAirborneMaxDuration, GuardBreakControlLockDuration)
		: (bUseLightingShockGuardBreak
			? (GuardBreakMontageToPlay ? GuardBreakMontageToPlay->GetPlayLength() : LightingShockDuration)
			: (GuardBreakMontageToPlay ? GuardBreakMontageToPlay->GetPlayLength() : GuardBreakDuration));
	PlayerHitControlLockRemainTime = bUseAirGuardBreak
		? FMath::Max(AirGuardBreakAirborneMaxDuration, GuardBreakControlLockDuration)
		: (bUseLightingShockGuardBreak ? LightingShockControlLockDuration : GuardBreakControlLockDuration);
	GuardBreakVulnerableRemainTime = PlayerHitControlLockRemainTime;
	ChainParryWindowRemainTime = 0.f;
	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	bParrySuccessHeavyAttackInputHeld = false;
	CurrentParrySuccessMontage = nullptr;
	CurrentHitReactType = bUseLightingShockGuardBreak ? EHitReactType::Lighting_Shock : EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	CurrentHitReactMontage = bUseLightingShockGuardBreak
		? LightingShockMontage.Get()
		: (bUseAirGuardBreak ? AirHitFLightMontage.Get() : nullptr);
	bCurrentHitReactUsesAirMontage = bUseAirGuardBreak;
	ResetAirHeavyHitReactState();
	bAirGuardBreakLandMontagePending = bUseAirGuardBreak;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (bUseAirGuardBreak)
		{
			ApplyAirGuardBreakTuning();
		}
		else
		{
			FVector NewVelocity = MovementComponent->Velocity;
			NewVelocity.X = 0.f;
			NewVelocity.Y = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
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
		JunPlayerController->StartPlayerPostureBreakHidePresentation();
	}

	if (GuardBreakMontageToPlay && (!bUseAirGuardBreak || bUseLightingShockGuardBreak))
	{
		PlayHitReactMontageWithBlend(GuardBreakMontageToPlay, bRestartingGuardBreak);
	}
	else if (bUseAirGuardBreak && AirHitFLightMontage)
	{
		PlayHitReactMontageWithBlend(AirHitFLightMontage, bRestartingGuardBreak);
	}
}

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

void AJunPlayer::StartHitReact(EHitReactType HitType, ECharacterHitReactDirection HitDirection)
{
	const bool bRestartingHitReact = CurrentHitState == EJunPlayerHitState::HitReact;

	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterStompOpportunityPending = false;
	JumpCounterStompTarget = nullptr;
	JumpCounterStompPendingTarget = nullptr;

	InterruptActionsForHitReaction();
	ResetAirHeavyHitReactState();

	const EJunAirHitReactType AirHitReactType = ResolveAirHitReactType();
	const bool bUseAirHitReact = AirHitReactType != EJunAirHitReactType::None;

	CurrentHitState = EJunPlayerHitState::HitReact;
	CurrentHitReactType = HitType;
	CurrentHitReactDirection = HitDirection;
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = bUseAirHitReact;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	bFreeRunSlideActive = false;
	FreeRunSlideSpeed = 0.f;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (!bUseAirHitReact)
		{
			FVector NewVelocity = MovementComponent->Velocity;
			NewVelocity.X = 0.f;
			NewVelocity.Y = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}
	ConsumeMovementInputVector();

	UAnimMontage* HitReactMontage = bUseAirHitReact ? GetAirHitReactMontage(AirHitReactType) : GetHitReactMontage(HitType, HitDirection);
	CurrentHitReactMontage = HitReactMontage;
	if (AirHitReactType == EJunAirHitReactType::Light)
	{
		PlayerHitStateRemainTime = HitReactMontage ? FMath::Min(HitReactMontage->GetPlayLength(), AirLightHitReactDuration) : AirLightHitReactDuration;
		PlayerHitControlLockRemainTime = AirLightHitControlLockDuration;
		ApplyAirLightHitFallTuning();
	}
	else if (AirHitReactType == EJunAirHitReactType::Heavy)
	{
		AirHeavyHitStage = EJunAirHeavyHitStage::Launch;
		const float LaunchMontageDuration = HitReactMontage ? HitReactMontage->GetPlayLength() : AirHeavyHitReactDuration;
		AirHeavyHitStageRemainTime = AirHeavyHitLaunchStageMaxDuration > 0.f
			? FMath::Min(LaunchMontageDuration, AirHeavyHitLaunchStageMaxDuration)
			: LaunchMontageDuration;
		PlayerHitStateRemainTime = FMath::Max(AirHeavyHitSequenceMaxDuration, AirHeavyHitStageRemainTime);
		PlayerHitControlLockRemainTime = FMath::Max(AirHeavyHitControlLockDuration, PlayerHitStateRemainTime);
		ApplyAirHeavyHitFallTuning();
	}
	else if (HitType == EHitReactType::LightHit)
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
	else if (HitType == EHitReactType::Lighting_Shock)
	{
		PlayerHitStateRemainTime = HitReactMontage ? HitReactMontage->GetPlayLength() : LightingShockDuration;
		PlayerHitControlLockRemainTime = LightingShockControlLockDuration;
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

	if (bUseAirHitReact)
	{
		ApplyAirHitKnockback(AirHitReactType);
	}
	else if (!DoesMontageUseRootMotion(HitReactMontage))
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

	if (!bUseAirHitReact)
	{
		AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	}
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	if (AirHitReactType == EJunAirHitReactType::Heavy)
	{
		AddGameplayTag(JunGameplayTags::State_Block_Move);
		AddGameplayTag(JunGameplayTags::State_Block_Jump);
		AddGameplayTag(JunGameplayTags::State_Block_Attack);
		AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	}

	if (HitReactMontage)
	{
		RequestPlayerHitReactHitStop();
		PlayHitReactMontageWithBlend(HitReactMontage, bRestartingHitReact);
	}
}

void AJunPlayer::RequestPlayerHitReactHitStop()
{
	if (!bEnablePlayerHitReactHitStop || !GetWorld() || PlayerHitReactHitStopDuration <= 0.f)
	{
		return;
	}

	AActor* AttackerActor = HitReactFacingTarget.Get();
	if (!IsValid(AttackerActor) || AttackerActor == this)
	{
		return;
	}

	if (UJunTimeEffectSubsystem* TimeEffectSubsystem = GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
	{
		TimeEffectSubsystem->RequestHitStopForActors(
			{ this, AttackerActor },
			PlayerHitReactHitStopDuration,
			PlayerHitReactHitStopTimeScale,
			PlayerHitReactHitStopPriority);
	}
}

void AJunPlayer::PlayHitReactMontageWithBlend(UAnimMontage* HitReactMontage, bool bRestartingHitReact)
{
	if (!HitReactMontage)
	{
		return;
	}

	UAnimInstance* PlayerAnimInstance = AnimInstance ? AnimInstance.Get() : GetPlayerAnimInstance();
	if (!PlayerAnimInstance)
	{
		PlayAnimMontage(HitReactMontage);
		return;
	}

	const float BlendInTime = bRestartingHitReact ? HitReactRestartBlendInTime : HitReactBlendInTime;
	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, BlendInTime));
	if (PlayerAnimInstance->Montage_PlayWithBlendSettings(HitReactMontage, BlendInSettings) <= 0.f)
	{
		PlayAnimMontage(HitReactMontage);
	}
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
	FinishDrinkPotion(true);

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

	if (bIsJigenAttacking)
	{
		if (AnimInstance && JigenMontage)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
			AnimInstance->Montage_Stop(0.05f, JigenMontage);
		}

		FinishJigen();
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
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

			if (GuardStartMontage)
			{
				AnimInstance->Montage_Stop(0.05f, GuardStartMontage);
			}

			if (GuardEndMontage)
			{
				AnimInstance->Montage_Stop(0.05f, GuardEndMontage);
			}

			if (AirGuardStartMontage)
			{
				AnimInstance->Montage_Stop(0.05f, AirGuardStartMontage);
			}

			if (AirGuardEndMontage)
			{
				AnimInstance->Montage_Stop(0.05f, AirGuardEndMontage);
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
		if (bParrySuccessHeavyAttackInputHeld && bHeavyAttackInputHeld)
		{
			HeavyAttackInputHoldTime += DeltaTime;
		}

		if (bDefenseButtonHeld)
		{
			ParrySuccessDefenseHoldElapsedTime += DeltaTime;
			if (ParrySuccessDefenseHoldElapsedTime >= ParrySuccessDefenseHoldConfirmTime &&
				BufferedParrySuccessCancelAction == EJunBufferedParrySuccessCancelAction::None)
			{
				BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::Defense);
			}
		}
		else
		{
			ParrySuccessDefenseHoldElapsedTime = 0.f;
		}

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

	UpdateAirHeavyHitReact(DeltaTime);

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
		if (UAnimMontage* HitReactMontage = CurrentHitReactMontage.Get())
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
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = false;
	bAirGuardBreakLandMontagePending = false;
	ResetAirHeavyHitReactState();
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();

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

bool AJunPlayer::TryCancelHitReactIntoJump()
{
	if (CurrentHitState != EJunPlayerHitState::HitReact ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	if (AnimInstance)
	{
		if (UAnimMontage* HitReactMontage = CurrentHitReactMontage.Get())
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
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = false;
	bAirGuardBreakLandMontagePending = false;
	ResetAirHeavyHitReactState();
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();

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

	return true;
}

void AJunPlayer::FinishPlayerHitState()
{
	const bool bWasGuardBlock = CurrentHitState == EJunPlayerHitState::GuardBlock;
	const bool bWasGuardBreak = CurrentHitState == EJunPlayerHitState::GuardBreak;
	const bool bShouldEndGuardAfterGuardBlock = bWasGuardBlock && !bDefenseButtonHeld;

	if (bWasGuardBreak)
	{
		CurrentPosture = 0.f;
	}

	CurrentHitState = EJunPlayerHitState::None;
	PlayerHitStateRemainTime = 0.f;
	PlayerHitControlLockRemainTime = 0.f;
	ChainParryWindowRemainTime = 0.f;
	ParrySuccessElapsedTime = 0.f;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
	CurrentHitReactType = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	CurrentHitReactMontage = nullptr;
	bCurrentHitReactUsesAirMontage = false;
	bAirGuardBreakLandMontagePending = false;
	ResetAirHeavyHitReactState();
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();
	BufferedParrySuccessCancelAction = EJunBufferedParrySuccessCancelAction::None;
	bParrySuccessHeavyAttackInputHeld = false;
	CurrentParrySuccessMontage = nullptr;
	bGuardBlockReleasePending = false;

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

	if (bWasGuardBlock)
	{
		if (bDefenseButtonHeld && DefenseState == EJunDefenseState::Ending)
		{
			EnterGuardLoop();
		}
		else if (bShouldEndGuardAfterGuardBlock && DefenseState == EJunDefenseState::Guarding)
		{
			BeginGuardEnd();
		}
	}
}

void AJunPlayer::OpenParryWindow(bool bCountAsParryAttempt)
{
	if (bCountAsParryAttempt)
	{
		if (ParrySpamPenaltyResetRemainTime <= 0.f)
		{
			ParrySpamPenaltyStack = 0;
		}
		else
		{
			++ParrySpamPenaltyStack;
		}

		ParrySpamPenaltyResetRemainTime = ParrySpamPenaltyResetDuration;
	}

	const float PenaltyScale = FMath::Clamp(
		1.f - static_cast<float>(ParrySpamPenaltyStack) * ParrySpamPenaltyPerStack,
		0.f,
		1.f);

	CurrentParryWindowDuration = FMath::Max(MinParryWindowDuration, DefaultParryWindowDuration * PenaltyScale);
	CurrentPerfectParryWindowDuration = FMath::Min(
		CurrentParryWindowDuration,
		FMath::Max(MinPerfectParryWindowDuration, PerfectParryWindowDuration * PenaltyScale));

	bParryWindowOpen = CurrentParryWindowDuration > KINDA_SMALL_NUMBER;
	ParryWindowRemainTime = CurrentParryWindowDuration;
}

void AJunPlayer::ResetParrySpamPenalty()
{
	ParrySpamPenaltyStack = 0;
	ParrySpamPenaltyResetRemainTime = 0.f;
}

void AJunPlayer::DrawDefenseTimingDebug() const
{
	if (!bShowDefenseTimingDebug || !GEngine)
	{
		return;
	}

	const float ParryRemainFrames = ParryWindowRemainTime * 60.f;
	const float ParryWindowFrames = CurrentParryWindowDuration * 60.f;
	const float PerfectWindowFrames = CurrentPerfectParryWindowDuration * 60.f;
	const float DodgeTimelineRate = GetCurrentDodgeTimelineRate();
	const float DodgeInvincibleRealTime = DodgeInvincibleRemainTime / FMath::Max(DodgeTimelineRate, KINDA_SMALL_NUMBER);
	const float DodgeInvincibleFrames = DodgeInvincibleRealTime * 60.f;
	const bool bDodgeInvincibleActive = HasGameplayTag(JunGameplayTags::State_Condition_Invincible);

	const FString DebugText = FString::Printf(
		TEXT("Defense Timing Debug\n")
		TEXT("Parry: %s | Remain %.3fs (%.1ff) | Window %.1ff | Perfect %.1ff | SpamStack %d\n")
		TEXT("Dodge IFrame: %s | Remain %.3fs (%.1ff) | PlayRate %.2f"),
		bParryWindowOpen ? TEXT("OPEN") : TEXT("CLOSED"),
		ParryWindowRemainTime,
		ParryRemainFrames,
		ParryWindowFrames,
		PerfectWindowFrames,
		ParrySpamPenaltyStack,
		bDodgeInvincibleActive ? TEXT("ON") : TEXT("OFF"),
		DodgeInvincibleRealTime,
		DodgeInvincibleFrames,
		CurrentDodgePlayRate);

	const FColor DebugColor = (bParryWindowOpen || bDodgeInvincibleActive) ? FColor::Green : FColor::White;
	GEngine->AddOnScreenDebugMessage(61001, 0.f, DebugColor, DebugText);
}

void AJunPlayer::StartDefenseSequence()
{
	// Every tap/press starts a fresh deflect attempt from frame 0 of GuardStart.
	const bool bIsImmediateRestartCycle =
		DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Ending;

	CurrentDefensePlayRate = FMath::Max(GuardStartPlayRate, KINDA_SMALL_NUMBER);
	OpenParryWindow(true);
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
	bAirParrySequenceActive = false;
	ResetGuardRestartAnchor();
	bDeferGuardMoveInput = true;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;

	ApplyDefenseStartBlockTags();

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

		if (GuardEndMontage)
		{
			AnimInstance->Montage_Stop(0.05f, GuardEndMontage);
		}

		if (AirGuardStartMontage)
		{
			AnimInstance->Montage_Stop(0.02f, AirGuardStartMontage);
		}

		if (AirGuardEndMontage)
		{
			AnimInstance->Montage_Stop(0.05f, AirGuardEndMontage);
		}
	}

	DefenseState = EJunDefenseState::Starting;
	++GuardStartRestartSerial;

	// Ground guard visuals are driven by the player ABP state machine.
	// The timer above still resolves deflect tap vs. guard hold.
}

void AJunPlayer::StartAirParrySequence()
{
	CurrentDefensePlayRate = FMath::Max(AirGuardStartPlayRate, KINDA_SMALL_NUMBER);
	OpenParryWindow(true);
	BufferedDefenseTransitionCancelAction = EJunBufferedDefenseCancelAction::None;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime =
		(AirGuardStartMontage ? AirGuardStartMontage->GetPlayLength() : DefaultDeflectResolveTime);
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	bKeepGuardBaseWhileEnding = false;
	GuardExitMoveReleaseRemainTime = 0.f;
	GuardMoveBlendRemainTime = 0.f;
	bDeferGuardMoveInput = false;
	bAirParrySequenceActive = true;
	if (!AnimInstance || !AirGuardStartMontage || !AnimInstance->Montage_IsPlaying(AirGuardStartMontage))
	{
		ResetGuardRestartAnchor();
	}
	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;

	ApplyAirDefenseStartBlockTags();

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);

		if (GuardStartMontage)
		{
			AnimInstance->Montage_Stop(0.02f, GuardStartMontage);
		}

		if (GuardEndMontage)
		{
			AnimInstance->Montage_Stop(0.05f, GuardEndMontage);
		}

		if (AirGuardEndMontage)
		{
			AnimInstance->Montage_Stop(0.05f, AirGuardEndMontage);
		}
	}

	DefenseState = EJunDefenseState::Starting;

	if (!AnimInstance || !AirGuardStartMontage)
	{
		BeginAirGuardEnd();
		return;
	}

	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
	if (AnimInstance->Montage_IsPlaying(AirGuardStartMontage))
	{
		AnimInstance->Montage_SetPlayRate(AirGuardStartMontage, CurrentDefensePlayRate);
		if (bGuardRestartAnchorReached && GuardRestartAnchorMontage == AirGuardStartMontage)
		{
			FinishGuardRestartBlend(false);
			AnimInstance->Montage_SetPosition(AirGuardStartMontage, GuardRestartAnchorPosition);
		}
	}
	else
	{
		AnimInstance->Montage_Play(AirGuardStartMontage, CurrentDefensePlayRate);
	}
}

void AJunPlayer::BeginDefenseFromBufferedInput()
{
	if (GetCharacterMovement()->IsFalling())
	{
		StartAirParrySequence();
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

	if (bAirParrySequenceActive)
	{
		BeginAirGuardEnd();
		return;
	}

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

void AJunPlayer::ApplyAirDefenseStartBlockTags()
{
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
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
			: FMath::Max(0.f, GuardEndStateDuration);
	GuardEndBaseReleaseRemainTime =
		GuardEndMontage
			? FMath::Max(0.f, GuardEndMontage->GetPlayLength() - GuardEndBaseReleaseTimeOffset)
			: FMath::Max(0.f, GuardEndStateDuration);
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	DefenseTransitionElapsedTime = 0.f;
	bKeepGuardBaseWhileEnding = true;
	bDeferGuardMoveInput = true;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	GuardMoveBlendRemainTime = GuardMoveBlendDuration;
	ApplyDefenseStartBlockTags();

	if (GuardEndFinishRemainTime <= 0.f)
	{
		FinishDefense();
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
	}

	// Ground guard end visuals are driven by the player ABP state machine.
	if (BufferedDefenseTransitionCancelAction != EJunBufferedDefenseCancelAction::None)
	{
		TryExecuteBufferedDefenseTransitionCancelAction();
	}
}

void AJunPlayer::BeginAirGuardEnd()
{
	if (DefenseState == EJunDefenseState::None || DefenseState == EJunDefenseState::Ending)
	{
		return;
	}

	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	bDeferGuardMoveInput = false;
	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);
	}

	FinishDefense();
}

void AJunPlayer::FinishDefense()
{
	const bool bWasAirParrySequenceActive = bAirParrySequenceActive;
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
	bAirParrySequenceActive = false;

	if (bWasAirParrySequenceActive)
	{
		bDeferGuardMoveInput = false;
		GuardMoveBlendRemainTime = 0.f;
		GuardExitMoveReleaseRemainTime = 0.f;
		PostDefenseTransitionCancelBufferRemainTime = 0.f;
		ClearDefenseBlockTags();
		DesiredMoveForward = PendingMoveForward;
		DesiredMoveRight = PendingMoveRight;
		return;
	}

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

void AJunPlayer::OnAirGuardStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != AirGuardStartMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
	}

	if (DefenseState != EJunDefenseState::Starting || !bAirParrySequenceActive)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	ResolveCurrentDeflectAttempt();
}

void AJunPlayer::OnAirGuardEndMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != AirGuardEndMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);
	}

	if (DefenseState == EJunDefenseState::Ending && bAirParrySequenceActive)
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

		if (!Candidate->CanBeLockOnTarget())
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

	if (!CanUseHitReactFacingWindow() || !HitReactFacingTarget)
	{
		EndHitReactFacingWindow();
		return;
	}

	FVector Direction = HitReactFacingTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const float ResolvedInterpSpeed = HitReactFacingWindowInterpSpeed > 0.f ? HitReactFacingWindowInterpSpeed : 30.f;
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		Direction.Rotation(),
		DeltaTime,
		ResolvedInterpSpeed
	));
}

bool AJunPlayer::CanUseHitReactFacingWindow() const
{
	return CurrentHitState == EJunPlayerHitState::HitReact ||
		CurrentHitState == EJunPlayerHitState::GuardBreak;
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
			: (bLockOnActive && Cast<AJunMonster>(LockOnTarget.Get()) && Cast<AJunMonster>(LockOnTarget.Get())->IsExecutionReady()
				? ExecutionReadyLockOnArmLength
				: GetPitchAdjustedSpringArmLength());
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
		const AJunMonster* LockOnMonster = Cast<AJunMonster>(LockOnTarget.Get());
		TargetFOV = LockOnMonster && LockOnMonster->IsExecutionReady()
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
	CachedLockOnRangeAlpha = -1.f;
	SmoothedLockOnDistance2D = -1.f;
	SmoothedLockOnPitchOffset = LockOnPitchOffset;
	bLockOnClosePitchModeActive = false;
	bLockOnFarPitchModeActive = false;
	SmoothedLockOnOcclusionLateralOffset = 0.f;
	CachedLockOnAimDirection2D = FVector::ZeroVector;
	bLockOnCloseAimStabilizationActive = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
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

	const AJunCharacter* LockOnCharacter = LockOnTarget.Get();
	return LockOnCharacter && LockOnCharacter->IsJumpCounterThreatActive();
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
		PendingCameraLookInput = FVector2D::ZeroVector;
		return;
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

	const AJunMonster* LockOnMonster = Cast<AJunMonster>(LockOnTarget.Get());
	const bool bTargetExecutionReady = LockOnMonster && LockOnMonster->IsExecutionReady();
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

	const FRotator NewYawRot = FMath::RInterpTo(
		FRotator(0.f, CurrentRot.Yaw, 0.f),
		FRotator(0.f, DesiredRot.Yaw, 0.f),
		DeltaTime,
		DynamicLockOnRotationInterpSpeed);
	const float NewPitch = FMath::FInterpTo(
		CurrentRot.Pitch,
		DesiredRot.Pitch,
		DeltaTime,
		DynamicLockOnPitchRotationInterpSpeed);
	const FRotator NewRot(NewPitch, NewYawRot.Yaw, 0.f);

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
		const FVector TargetLockOnPoint = LockOnTarget->GetLockOnTargetPoint();
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




