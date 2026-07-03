#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerPotionComponent.h"
#include "Components/AudioComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Interface/JunLockOnTargetInterface.h"
#include "Interface/JunPlayerCombatTargetInterface.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Level/JunPlayerRespawnPoint.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunBGMManager.h"
#include "System/JunTimeEffectSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#pragma region Death / Respawn
// ============================================================================
// Death / Respawn
// ============================================================================

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
	if (!TryApplyForcedActionRequest(MakeForcedActionRequest(
		EJunPlayerActionState::Death,
		EJunActionTransitionReason::ForcedByDeath)))
	{
		return;
	}

	DeathState = EJunPlayerDeathState::FakeDeath;
	Hp = 0;
	bDeathPresentationStarted = false;
	bPendingDeathPresentationIsRealDeath = false;
	ApplyDeathControlLock();
	NotifyCombatTargetsPlayerFakeDeathStarted();

	if (!TryStartAirDeathSequence() && FakeDeathMontage)
	{
		PlayAnimMontage(FakeDeathMontage);
	}
}

void AJunPlayer::EnterRealDeath()
{
	if (!TryApplyForcedActionRequest(MakeForcedActionRequest(
		EJunPlayerActionState::Death,
		EJunActionTransitionReason::ForcedByDeath)))
	{
		return;
	}

	DeathState = EJunPlayerDeathState::RealDeath;
	Hp = 0;
	RealDeathHoldRemainTime = 0.f;
	RealDeathFullBlackHoldRemainTime = -1.f;
	bRealDeathBlackFadeStarted = false;
	bDeathPresentationStarted = false;
	AddGameplayTag(JunGameplayTags::State_Condition_Dead);
	ApplyDeathControlLock();
	NotifyCombatTargetsPlayerRealDeathStarted();
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
	if (!TryApplyForcedActionRequest(MakeForcedActionRequest(
		EJunPlayerActionState::Death,
		EJunActionTransitionReason::ForcedByDeath)))
	{
		return;
	}

	DeathState = EJunPlayerDeathState::RealDeath;
	Hp = 0;
	RealDeathHoldRemainTime = 0.f;
	RealDeathFullBlackHoldRemainTime = -1.f;
	bRealDeathBlackFadeStarted = false;
	bPendingDeathPresentationIsRealDeath = true;
	bDeathPresentationStarted = false;
	AddGameplayTag(JunGameplayTags::State_Condition_Dead);
	ApplyDeathControlLock();
	NotifyCombatTargetsPlayerRealDeathStarted();
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
	ClearActionStateIf(EJunPlayerActionState::Death, EJunActionTransitionReason::System);
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
	NotifyCombatTargetsPlayerRevivedFromFakeDeath();

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
		ResetCombatTargetsAfterPlayerRealDeath();
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
	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
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
	PendingBasicAttackRecoveryActionRequest.Reset();
	PostBasicAttackJumpDodgeBufferRemainTime = 0.f;
	PostBasicAttackDefenseBufferRemainTime = 0.f;

	bBufferedHeavyAttackInput = false;
	bBufferedHeavyAttackComboInput = false;
	bCanBufferHeavyAttackComboInput = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;
	PendingHeavyAttackRecoveryActionRequest.Reset();
	ResetHeavyAttackChargeInput();

	bBufferedJigenInput = false;
	bBufferedJigenComboInput = false;
	bCanBufferJigenComboInput = false;
	bJigenComboAdvanceStateActive = false;
	PendingJigenRecoveryActionRequest.Reset();

	if (DefenseComponent)
	{
		DefenseComponent->ClearDefenseInputBuffers();
	}
	PendingParrySuccessActionRequest.Reset();
	bParrySuccessHeavyAttackInputHeld = false;
	ParrySuccessDefenseHoldElapsedTime = 0.f;
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

void AJunPlayer::NotifyCombatTargetsPlayerFakeDeathStarted()
{
	for (TActorIterator<AJunCharacter> It(GetWorld()); It; ++It)
	{
		if (IJunPlayerCombatTargetInterface* CombatTarget = Cast<IJunPlayerCombatTargetInterface>(*It))
		{
			CombatTarget->OnPlayerFakeDeathStarted();
		}
	}
}

void AJunPlayer::NotifyCombatTargetsPlayerRevivedFromFakeDeath()
{
	for (TActorIterator<AJunCharacter> It(GetWorld()); It; ++It)
	{
		if (IJunPlayerCombatTargetInterface* CombatTarget = Cast<IJunPlayerCombatTargetInterface>(*It))
		{
			CombatTarget->OnPlayerFakeDeathRevived();
		}
	}
}

void AJunPlayer::NotifyCombatTargetsPlayerRealDeathStarted()
{
	for (TActorIterator<AJunCharacter> It(GetWorld()); It; ++It)
	{
		if (IJunPlayerCombatTargetInterface* CombatTarget = Cast<IJunPlayerCombatTargetInterface>(*It))
		{
			CombatTarget->OnPlayerRealDeathStarted();
		}
	}
}

void AJunPlayer::ResetCombatTargetsAfterPlayerRealDeath()
{
	for (TActorIterator<AJunCharacter> It(GetWorld()); It; ++It)
	{
		if (IJunPlayerCombatTargetInterface* CombatTarget = Cast<IJunPlayerCombatTargetInterface>(*It))
		{
			CombatTarget->OnPlayerRealDeathRespawned();
		}
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
	ClearActionStateIf(EJunPlayerActionState::Death, EJunActionTransitionReason::System);
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
	ResetCameraToSpawnView(SpawnViewRotation, RespawnCameraPitch);
	StartCameraHardSnap(3);
	ClearDeathControlLock();

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(GetController()))
	{
		JunPlayerController->SetLockOnMarkerSuppressed(false);
		JunPlayerController->ResetPlayerPostureVisibilityState();
		JunPlayerController->StartRespawnFullBlackFadeOut();
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
		const IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(LockOnTarget.Get());
		const FVector TargetLockOnPoint = LockOnTargetInterface
			? LockOnTargetInterface->GetLockOnCameraTargetPoint()
			: LockOnTarget->GetLockOnTargetPoint();
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
	const IJunLockOnTargetInterface* LockOnTargetInterface = Cast<IJunLockOnTargetInterface>(LockOnTarget.Get());
	const bool bTargetExecutionReady = LockOnTargetInterface && LockOnTargetInterface->IsLockOnExecutionReady();
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
	GetWorldTimerManager().ClearTimer(GuardBlockReleaseIntoGuardEndTimerHandle);
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
	PendingParrySuccessActionRequest.Reset();
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

#pragma endregion
