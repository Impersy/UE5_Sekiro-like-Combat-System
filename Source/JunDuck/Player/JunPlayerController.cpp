#include "JunPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "System/JunAssetManager.h"
#include "Data/JunInputData.h"
#include "JunGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Character/JunCharacter.h"
#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
#include "EngineUtils.h"
#include "UI/JunCombatHUDWidget.h"
#include "UI/JunDangerMarkerWidget.h"
#include "UI/JunLockOnMarkerWidget.h"

AJunPlayerController::AJunPlayerController(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	bShowMouseCursor = false;
}

void AJunPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (const UJunInputData* InputData = UJunAssetManager::GetAssetByName<UJunInputData>("InputData"))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputData->IMC_Default, 0);
		}
	}

	JunPlayer = Cast<AJunPlayer>(GetPawn());
	InitializeCombatWidgets();

}

void AJunPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (const UJunInputData* InputData = UJunAssetManager::GetAssetByName<UJunInputData>("InputData")) // PDA 에 등록된 이름
	{
		if (auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			auto MoveAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::Input_MoveReleased);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::Input_MoveReleased);

			auto TurnAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Turn);
			EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Turn);

			auto JumpAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Input_Jump);

			auto DodgeAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Dodge);
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ThisClass::Input_Dodge);
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Completed, this, &ThisClass::Input_DodgeReleased);
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Canceled, this, &ThisClass::Input_DodgeReleased);

			auto BasicAttackAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_BasicAttack);
			EnhancedInputComponent->BindAction(BasicAttackAction, ETriggerEvent::Started, this, &ThisClass::Input_BasicAttack);

			auto HeavyAttackAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_HeavyAttack);
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &ThisClass::Input_HeavyAttackStarted);
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Completed, this, &ThisClass::Input_HeavyAttackReleased);
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Canceled, this, &ThisClass::Input_HeavyAttackReleased);

			auto LockOnAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_LockOn);
			EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::Input_LockOn);

			auto DefenseAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Defense);
			EnhancedInputComponent->BindAction(DefenseAction, ETriggerEvent::Started, this, &ThisClass::Input_Defense);
			EnhancedInputComponent->BindAction(DefenseAction, ETriggerEvent::Completed, this, &ThisClass::Input_DefenseReleased);
			EnhancedInputComponent->BindAction(DefenseAction, ETriggerEvent::Canceled, this, &ThisClass::Input_DefenseReleased);

			auto RunAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Run);
			EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Started, this, &ThisClass::Input_RunStarted);
			EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &ThisClass::Input_RunReleased);
			EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Canceled, this, &ThisClass::Input_RunReleased);

			auto WalkToggleAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_WalkToggle);
			EnhancedInputComponent->BindAction(WalkToggleAction, ETriggerEvent::Started, this, &ThisClass::Input_WalkToggle);
		}
	}
	
}

void AJunPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateCombatWidgets();
}

void AJunPlayerController::InitializeCombatWidgets()
{
	if (CombatHUDWidgetClass && !CombatHUDWidget)
	{
		CombatHUDWidget = CreateWidget<UJunCombatHUDWidget>(this, CombatHUDWidgetClass);
		if (CombatHUDWidget)
		{
			CombatHUDWidget->AddToViewport(0);
			CombatHUDWidget->SetBossHealthVisible(false);
		}
	}

	if (LockOnMarkerWidgetClass && !LockOnMarkerWidget)
	{
		LockOnMarkerWidget = CreateWidget<UJunLockOnMarkerWidget>(this, LockOnMarkerWidgetClass);
		if (LockOnMarkerWidget)
		{
			LockOnMarkerWidget->AddToViewport(1);
			LockOnMarkerWidget->SetLockOnMarkerVisible(false);
		}
	}

	if (DangerMarkerWidgetClass && !DangerMarkerWidget)
	{
		DangerMarkerWidget = CreateWidget<UJunDangerMarkerWidget>(this, DangerMarkerWidgetClass);
		if (DangerMarkerWidget)
		{
			DangerMarkerWidget->AddToViewport(2);
		}
	}
}

void AJunPlayerController::UpdateCombatWidgets()
{
	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	if (!JunPlayer)
	{
		return;
	}

	AJunCharacter* CurrentLockOnTarget = JunPlayer->IsLockOn() ? JunPlayer->GetLockOnTarget() : nullptr;
	AJunMonster* ActiveCombatBoss = FindActiveCombatBoss();

	if (CombatHUDWidget)
	{
		CombatHUDWidget->SetPlayerHealth(JunPlayer->GetHp(), JunPlayer->GetMaxHp());
		CombatHUDWidget->SetPlayerPosture(JunPlayer->GetCurrentPosture(), JunPlayer->GetMaxPosture());
		CombatHUDWidget->SetBossHealthVisible(ActiveCombatBoss != nullptr);

		if (ActiveCombatBoss)
		{
			CombatHUDWidget->SetBossHealth(ActiveCombatBoss->GetHp(), ActiveCombatBoss->GetMaxHp());
			CombatHUDWidget->SetBossPosture(ActiveCombatBoss->GetCurrentPosture(), ActiveCombatBoss->GetMaxPosture());
			CombatHUDWidget->SetBossLifeState(ActiveCombatBoss->GetCurrentLifeCount(), ActiveCombatBoss->GetMaxLifeCount());
		}
		else
		{
			CombatHUDWidget->SetBossPosture(0.f, 1.f);
			CombatHUDWidget->SetBossLifeState(0, 0);
		}
	}

	UpdateDangerMarkerWidget();
	UpdateLockOnMarkerWidget(CurrentLockOnTarget);
}

void AJunPlayerController::PlayPlayerPostureBreakGlow()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->PlayPlayerPostureBreakGlow();
	}
}

void AJunPlayerController::PlayBossPostureBreakGlow()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->PlayBossPostureBreakGlow();
	}
}

void AJunPlayerController::ShowBossClearUI()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->ShowBossClearUI();
	}
}

void AJunPlayerController::ShowFakeDeathUI()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->ShowFakeDeathUI();
	}
}

void AJunPlayerController::HideFakeDeathUI()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->HideFakeDeathUI();
	}
}

void AJunPlayerController::ShowRealDeathUI()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->ShowRealDeathUI();
	}
}

void AJunPlayerController::StartDeathFullBlackFadeIn()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->StartFullBlackFadeIn();
	}
}

void AJunPlayerController::HideDeathUI()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->HideDeathUI();
	}
}

bool AJunPlayerController::IsDeathFullBlackOpaque() const
{
	return CombatHUDWidget && CombatHUDWidget->IsFullBlackOpaque();
}

void AJunPlayerController::SetLockOnMarkerSuppressed(bool bSuppressed)
{
	bLockOnMarkerSuppressed = bSuppressed;

	if (bLockOnMarkerSuppressed && LockOnMarkerWidget)
	{
		LockOnMarkerWidget->SetExecutionReadyMarkerVisible(false);
		LockOnMarkerWidget->SetLockOnMarkerVisible(false);
	}
}

void AJunPlayerController::ResetPlayerPostureVisibilityState()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->ResetPlayerPostureVisibilityState();
	}
}

void AJunPlayerController::PlayDangerMarkerOnPlayer()
{
	if (!DangerMarkerWidget)
	{
		return;
	}

	DangerMarkerWidget->PlayDangerMarker();
	UpdateDangerMarkerWidget();
}

void AJunPlayerController::UpdateDangerMarkerWidget()
{
	if (!DangerMarkerWidget || !DangerMarkerWidget->IsDangerMarkerActive())
	{
		return;
	}

	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	if (!JunPlayer)
	{
		return;
	}

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const FVector MarkerWorldLocation =
		JunPlayer->GetActorLocation() +
		JunPlayer->GetActorForwardVector() * DangerMarkerPlayerWorldOffset.X +
		JunPlayer->GetActorRightVector() * DangerMarkerPlayerWorldOffset.Y +
		FVector::UpVector * DangerMarkerPlayerWorldOffset.Z;
	if (!ProjectWorldLocationToScreen(MarkerWorldLocation, ScreenPosition, true))
	{
		return;
	}

	DangerMarkerWidget->SetPositionInViewport(ScreenPosition, true);
}

AJunMonster* AJunPlayerController::FindActiveCombatBoss() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AJunMonster> It(World); It; ++It)
	{
		AJunMonster* Monster = *It;
		if (Monster && (Monster->GetCurrentState() == EMonsterState::Combat || Monster->Is_Dead()))
		{
			return Monster;
		}
	}

	return nullptr;
}

void AJunPlayerController::UpdateLockOnMarkerWidget(AJunCharacter* CurrentLockOnTarget)
{
	if (!LockOnMarkerWidget)
	{
		return;
	}

	if (bLockOnMarkerSuppressed)
	{
		LockOnMarkerWidget->SetExecutionReadyMarkerVisible(false);
		LockOnMarkerWidget->SetLockOnMarkerVisible(false);
		return;
	}

	const bool bTargetChanged = CurrentLockOnTarget != PreviousLockOnMarkerTarget;
	if (bTargetChanged)
	{
		PreviousLockOnMarkerTarget = CurrentLockOnTarget;
		LockOnMarkerShowDelayRemainTime = CurrentLockOnTarget
			? FMath::Max(0.f, LockOnMarkerShowDelay)
			: 0.f;
	}

	if (!CurrentLockOnTarget)
	{
		LockOnMarkerWidget->SetExecutionReadyMarkerVisible(false);
		LockOnMarkerWidget->SetLockOnMarkerVisible(false);
		return;
	}

	LockOnMarkerShowDelayRemainTime = FMath::Max(0.f, LockOnMarkerShowDelayRemainTime - GetWorld()->GetDeltaSeconds());

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const bool bProjected = ProjectWorldLocationToScreen(JunPlayer->GetLockOnMarkerWorldPoint(), ScreenPosition, true);
	if (!bProjected)
	{
		LockOnMarkerWidget->SetExecutionReadyMarkerVisible(false);
		LockOnMarkerWidget->SetLockOnMarkerVisible(false);
		return;
	}

	LockOnMarkerWidget->SetPositionInViewport(ScreenPosition, true);

	const bool bShowLockOnMarker = LockOnMarkerShowDelayRemainTime <= 0.f;
	LockOnMarkerWidget->SetLockOnMarkerVisible(bShowLockOnMarker);

	const AJunMonster* LockOnMonster = Cast<AJunMonster>(CurrentLockOnTarget);
	LockOnMarkerWidget->SetExecutionReadyMarkerVisible(bShowLockOnMarker && LockOnMonster && LockOnMonster->IsExecutionReady());
}

void AJunPlayerController::Input_Move(const FInputActionValue& InputValue)
{
	if (!JunPlayer || !GetPawn())
	{
		return;
	}

	FVector2D MoveVec = InputValue.Get<FVector2D>();

	if (JunPlayer)
	{
		JunPlayer->SetDesiredMoveAxes(MoveVec.X, MoveVec.Y);
	}

	if (JunPlayer->GetDefenseState() == EJunDefenseState::Ending &&
		(MoveVec.X != 0.f || MoveVec.Y != 0.f))
	{
		JunPlayer->BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::Move);
	}

	if (JunPlayer->IsBasicAttacking() &&
		(MoveVec.X != 0.f || MoveVec.Y != 0.f))
	{
		JunPlayer->TryCancelBasicAttackIntoMove();
	}

	if (JunPlayer->IsInParrySuccess() &&
		(MoveVec.X != 0.f || MoveVec.Y != 0.f))
	{
		JunPlayer->BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::Move);
	}

	if (JunPlayer->IsJumpAttacking() &&
		(MoveVec.X != 0.f || MoveVec.Y != 0.f) &&
		JunPlayer->TryCancelJumpAttackEndIntoMove())
	{
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Block_Move) || 
		JunPlayer->GetPlayerIsFalling())
	{
		return;
	}

	if (MoveVec.X != 0)
	{
		FRotator Rotator = GetControlRotation();
		FVector Direction = UKismetMathLibrary::GetForwardVector(FRotator(0, Rotator.Yaw, 0));
		GetPawn()->AddMovementInput(Direction, MoveVec.X); // 입력에 따른 방향만 설정
	}

	if (MoveVec.Y != 0)
	{
		FRotator Rotator = GetControlRotation();
		FVector Direction = UKismetMathLibrary::GetRightVector(FRotator(0, Rotator.Yaw, 0));
		GetPawn()->AddMovementInput(Direction, MoveVec.Y); // 입력에 따른 방향만 설정
	}
}

void AJunPlayerController::Input_MoveReleased(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	JunPlayer->OnMoveInputReleased();
	JunPlayer->SetDesiredMoveAxes(0.f, 0.f);
}

void AJunPlayerController::Input_Turn(const FInputActionValue& InputValue)
{

	if (!JunPlayer)
	{
		return;
	}

	const FVector2D LookAxis = InputValue.Get<FVector2D>();
	JunPlayer->AddCameraLookInput(LookAxis);
}

void AJunPlayerController::Input_Jump(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		JunPlayer->TryCancelDodgeIntoJump();
		return;
	}

	if (JunPlayer->IsInParrySuccess())
	{
		JunPlayer->BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::Jump);
		return;
	}

	if (JunPlayer->IsBasicAttacking())
	{
		if (JunPlayer->CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Jump))
		{
			JunPlayer->BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction::Jump);
		}
		return;
	}

	if (JunPlayer->IsHeavyAttacking())
	{
		JunPlayer->Jump();
		return;
	}

	if (JunPlayer->CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Jump))
	{
		JunPlayer->BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction::Jump);
		return;
	}

	if (JunPlayer->CanBufferDefenseTransitionCancel())
	{
		JunPlayer->BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::Jump);
		return;
	}
	
	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Block_Jump) ||
		JunPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return;
	}

	JunPlayer->Jump();
	
}

void AJunPlayerController::Input_Dodge(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->IsInParrySuccess())
	{
		JunPlayer->BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::Dodge);
		return;
	}

	if (JunPlayer->IsBasicAttacking())
	{
		if (JunPlayer->CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Dodge))
		{
			JunPlayer->BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction::Dodge);
		}
		return;
	}

	if (JunPlayer->IsHeavyAttacking())
	{
		JunPlayer->StartDodge();
		return;
	}

	if (JunPlayer->IsJumpAttacking())
	{
		JunPlayer->TryCancelJumpAttackEndIntoDodge();
		return;
	}

	if (JunPlayer->CanBufferDefenseTransitionCancel())
	{
		JunPlayer->BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::Dodge);
		return;
	}

	if (JunPlayer->CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Dodge))
	{
		JunPlayer->BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction::Dodge);
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		JunPlayer->StartDodge();
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Block_Dodge))
	{
		return;
	}

	JunPlayer->StartDodge();
}

void AJunPlayerController::Input_DodgeReleased(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	JunPlayer->OnDodgeInputReleased();
}

void AJunPlayerController::Input_BasicAttack(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->TryStartJigen())
	{
		return;
	}

	if (JunPlayer->TryChooseFakeDeathDie())
	{
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (!JunPlayer->TryStartDodgeAttack())
		{
			JunPlayer->TryCancelDodgeIntoBasicAttack();
		}
		return;
	}

	if (JunPlayer->TryStartExecution())
	{
		return;
	}

	if (JunPlayer->IsInParrySuccess())
	{
		JunPlayer->BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::BasicAttack);
		return;
	}

	if (JunPlayer->IsJumpAttacking())
	{
		JunPlayer->TryCancelJumpAttackEndIntoBasicAttack();
		return;
	}

	if (JunPlayer->IsDodgeAttacking())
	{
		JunPlayer->TryCancelDodgeAttackIntoBasicAttack();
		return;
	}

	if (JunPlayer->IsHeavyAttacking())
	{
		JunPlayer->TryCancelHeavyAttackIntoBasicAttack();
		return;
	}

	if (JunPlayer->CanBufferDefenseTransitionCancel())
	{
		JunPlayer->BufferDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction::BasicAttack);
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Block_Attack) ||
		JunPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return;
	}

	JunPlayer->BasicAttack();
}

void AJunPlayerController::Input_HeavyAttackStarted(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->IsInParrySuccess())
	{
		JunPlayer->BufferParrySuccessCancelAction(EJunBufferedParrySuccessCancelAction::HeavyAttack);
		return;
	}

	JunPlayer->OnHeavyAttackStarted();
}

void AJunPlayerController::Input_HeavyAttackReleased(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->IsInParrySuccess())
	{
		return;
	}

	JunPlayer->OnHeavyAttackReleased();
}

void AJunPlayerController::Input_LockOn(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	JunPlayer->ToggleLockOn();
}

void AJunPlayerController::Input_Defense(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->TryChooseFakeDeathRevive())
	{
		return;
	}

	JunPlayer->OnDefenseStarted();
}

void AJunPlayerController::Input_DefenseReleased(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	JunPlayer->OnDefenseReleased();
}

void AJunPlayerController::Input_RunStarted(const FInputActionValue& InputValue)
{
	// Walk is controlled by Input_WalkToggle. Keep Run input inert so legacy mappings do not force hold-walk.
}

void AJunPlayerController::Input_RunReleased(const FInputActionValue& InputValue)
{
	// Walk is controlled by Input_WalkToggle. Keep Run input inert so legacy mappings do not force hold-walk.
}

void AJunPlayerController::Input_WalkToggle(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	JunPlayer->ToggleWalkingState();
}
