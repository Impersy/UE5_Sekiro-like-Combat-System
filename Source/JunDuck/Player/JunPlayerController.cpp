#include "JunPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "System/JunAssetManager.h"
#include "Data/JunInputData.h"
#include "JunGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Character/JunCharacter.h"
#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
#include "Character/JunTutorialNPC.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Level/JunPlayerRespawnPoint.h"
#include "Level/JunTutorialNPCPlacementPoint.h"
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

			auto DrinkAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Drink);
			EnhancedInputComponent->BindAction(DrinkAction, ETriggerEvent::Started, this, &ThisClass::Input_Drink);

			auto ControlsToggleAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_ControlsToggle);
			EnhancedInputComponent->BindAction(ControlsToggleAction, ETriggerEvent::Started, this, &ThisClass::Input_ControlsToggle);

			auto InteractAction = InputData->FindInputActionByTag(JunGameplayTags::Input_Action_Interact);
			if (UInputAction* MutableInteractAction = const_cast<UInputAction*>(InteractAction))
			{
				MutableInteractAction->bTriggerWhenPaused = true;
			}
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::Input_Interact);
		}
	}
	
}

void AJunPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateCombatWidgets();
	UpdateTutorialTransition(DeltaTime);
}

void AJunPlayerController::InitializeCombatWidgets()
{
	if (CombatHUDWidgetClass && !CombatHUDWidget)
	{
		CombatHUDWidget = CreateWidget<UJunCombatHUDWidget>(this, CombatHUDWidgetClass);
		if (CombatHUDWidget)
		{
			CombatHUDWidget->AddToViewport(3);
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
		CombatHUDWidget->SetPotionCount(JunPlayer->GetCurrentDrinkPotionCharges());
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
	UpdateLockOnMarkerWidget(CurrentLockOnTarget, ActiveCombatBoss);
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

void AJunPlayerController::StartPlayerPostureBreakHidePresentation()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->StartPlayerPostureBreakHidePresentation();
	}
}

void AJunPlayerController::HideBossPostureImmediately()
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->HideBossPostureImmediately();
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
		if (Monster &&
			Monster->ShouldShowBossCombatHUD() &&
			(Monster->GetCurrentState() == EMonsterState::Combat || Monster->Is_Dead()))
		{
			return Monster;
		}
	}

	return nullptr;
}

FVector AJunPlayerController::GetLockOnMarkerWorldPointForTarget(const AJunCharacter* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	USkeletalMeshComponent* TargetMesh = Target->GetMesh();
	static const FName LockOnBoneName(TEXT("spine_03"));
	if (TargetMesh && TargetMesh->GetBoneIndex(LockOnBoneName) != INDEX_NONE)
	{
		return TargetMesh->GetBoneLocation(LockOnBoneName);
	}

	return Target->GetLockOnTargetPoint();
}

void AJunPlayerController::UpdateLockOnMarkerWidget(AJunCharacter* CurrentLockOnTarget, AJunMonster* ActiveCombatBoss)
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

	AJunMonster* ExecutionReadyMonster = nullptr;
	if (AJunMonster* LockOnMonster = Cast<AJunMonster>(CurrentLockOnTarget))
	{
		ExecutionReadyMonster = LockOnMonster->IsExecutionReady() ? LockOnMonster : nullptr;
	}
	if (!ExecutionReadyMonster && ActiveCombatBoss && ActiveCombatBoss->IsExecutionReady())
	{
		ExecutionReadyMonster = ActiveCombatBoss;
	}

	AJunCharacter* MarkerTarget = CurrentLockOnTarget ? CurrentLockOnTarget : ExecutionReadyMonster;

	const bool bTargetChanged = CurrentLockOnTarget != PreviousLockOnMarkerTarget;
	if (bTargetChanged)
	{
		PreviousLockOnMarkerTarget = CurrentLockOnTarget;
		LockOnMarkerShowDelayRemainTime = CurrentLockOnTarget
			? FMath::Max(0.f, LockOnMarkerShowDelay)
			: 0.f;
	}

	if (!MarkerTarget)
	{
		LockOnMarkerWidget->SetExecutionReadyMarkerVisible(false);
		LockOnMarkerWidget->SetLockOnMarkerVisible(false);
		return;
	}

	LockOnMarkerShowDelayRemainTime = FMath::Max(0.f, LockOnMarkerShowDelayRemainTime - GetWorld()->GetDeltaSeconds());

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const FVector MarkerWorldPoint = CurrentLockOnTarget
		? JunPlayer->GetLockOnMarkerWorldPoint()
		: GetLockOnMarkerWorldPointForTarget(MarkerTarget);
	const bool bProjected = ProjectWorldLocationToScreen(MarkerWorldPoint, ScreenPosition, true);
	if (!bProjected)
	{
		LockOnMarkerWidget->SetExecutionReadyMarkerVisible(false);
		LockOnMarkerWidget->SetLockOnMarkerVisible(false);
		return;
	}

	LockOnMarkerWidget->SetPositionInViewport(ScreenPosition, true);

	const bool bShowLockOnMarker = CurrentLockOnTarget && LockOnMarkerShowDelayRemainTime <= 0.f;
	LockOnMarkerWidget->SetLockOnMarkerVisible(bShowLockOnMarker);
	LockOnMarkerWidget->SetExecutionReadyMarkerVisible(ExecutionReadyMonster != nullptr);
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	if (JunPlayer->IsDrinkingPotion())
	{
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		JunPlayer->TryCancelDodgeIntoJump();
		return;
	}

	if (JunPlayer->TryStartJumpCounterStompFollowUp())
	{
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

	if (JunPlayer->GetDefenseState() == EJunDefenseState::Starting)
	{
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	if (JunPlayer->IsDrinkingPotion())
	{
		return;
	}

	bDodgeInputHeld = true;
	if (UWorld* World = GetWorld())
	{
		LastDodgeInputTime = World->GetTimeSeconds();
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
	bDodgeInputHeld = false;

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

	if (JunPlayer->TryChooseFakeDeathDie())
	{
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	if (JunPlayer->IsDrinkingPotion())
	{
		return;
	}

	if (JunPlayer->IsInDeathSequence())
	{
		return;
	}

	if (JunPlayer->TryCancelJumpCounterStompFollowUpIntoJumpAttack())
	{
		return;
	}

	if (JunPlayer->TryStartJigen())
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	if (JunPlayer->IsDrinkingPotion())
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	if (JunPlayer->IsInParrySuccess())
	{
		JunPlayer->OnHeavyAttackReleased();
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	if (JunPlayer->IsDrinkingPotion())
	{
		return;
	}

	const UWorld* World = GetWorld();
	const bool bRecentlyPressedDodge = World &&
		World->GetTimeSeconds() - LastDodgeInputTime <= MikiriCounterDodgeDefenseInputGraceTime;
	if (bRecentlyPressedDodge && JunPlayer->TryOpenMikiriCounterWindow(false))
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

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
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

void AJunPlayerController::Input_Drink(const FInputActionValue& InputValue)
{
	if (!JunPlayer)
	{
		return;
	}

	if (JunPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	JunPlayer->TryStartDrinkPotion();
}

void AJunPlayerController::Input_ControlsToggle(const FInputActionValue& InputValue)
{
	if (CombatHUDWidget)
	{
		CombatHUDWidget->ToggleControlsVisibility();
	}
}

void AJunPlayerController::Input_Interact(const FInputActionValue& InputValue)
{
	if (bPlayerPostureTutorialGuideActive)
	{
		HidePlayerPostureTutorialGuide();
		return;
	}

	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	if (!JunPlayer || !CombatHUDWidget)
	{
		return;
	}

	if (IsTutorialTransitionActive())
	{
		return;
	}

	if (AJunTutorialNPC* DialogueNPC = ActiveDialogueNPC.Get())
	{
		if (DialogueNPC->IsDialogueActive())
		{
			const bool bWasTutorialEndDialogue = DialogueNPC->IsTutorialEndDialogueActive();
			if (DialogueNPC->AdvanceDialogue())
			{
				ShowDialogueLineFromNPC(DialogueNPC);
				return;
			}

			CombatHUDWidget->HideDialogue();
			JunPlayer->EndDialogueCamera();
			if (bWasTutorialEndDialogue)
			{
				JunPlayer->EndTutorialControlLock();
				ActiveDialogueNPC = nullptr;
				return;
			}

			StartTutorialTransition(DialogueNPC);
			ActiveDialogueNPC = nullptr;
			return;
		}
	}

	AJunTutorialNPC* DialogueNPC = FindAvailableDialogueNPC();
	if (!DialogueNPC || !DialogueNPC->TryStartDialogue(JunPlayer))
	{
		return;
	}

	ActiveDialogueNPC = DialogueNPC;
	JunPlayer->BeginTutorialControlLock();
	JunPlayer->BeginDialogueCamera();
	ShowDialogueLineFromNPC(DialogueNPC);
}

void AJunPlayerController::ShowPlayerPostureTutorialGuide()
{
	if (!BeginTutorialGuidePause())
	{
		return;
	}

	CombatHUDWidget->ShowPlayerPostureGuide();
}

void AJunPlayerController::ShowMonsterPostureTutorialGuide()
{
	if (!BeginTutorialGuidePause())
	{
		return;
	}

	CombatHUDWidget->ShowMonsterPostureGuide();
}

void AJunPlayerController::ShowDangerAttackTutorialGuide()
{
	if (!BeginTutorialGuidePause())
	{
		return;
	}

	CombatHUDWidget->ShowDangerAttackGuide();
}

bool AJunPlayerController::BeginTutorialGuidePause()
{
	if (bPlayerPostureTutorialGuideActive || !CombatHUDWidget)
	{
		return false;
	}

	bPlayerPostureTutorialGuideActive = true;
	bGamePausedBeforeTutorialGuide = IsPaused();
	SetPause(true);
	return true;
}

void AJunPlayerController::HidePlayerPostureTutorialGuide()
{
	if (!bPlayerPostureTutorialGuideActive)
	{
		return;
	}

	bPlayerPostureTutorialGuideActive = false;
	if (CombatHUDWidget)
	{
		CombatHUDWidget->HideTutorialGuides();
	}
	if (!bGamePausedBeforeTutorialGuide)
	{
		SetPause(false);
	}

	if (PendingTutorialTaskUIType != INDEX_NONE && CombatHUDWidget)
	{
		CombatHUDWidget->ShowTutorialTask(PendingTutorialTaskUIType);
		PendingTutorialTaskUIType = INDEX_NONE;
	}
}

void AJunPlayerController::ShowTutorialTaskUI(EJunTutorialTaskType TaskType)
{
	const int32 TaskTypeValue = static_cast<int32>(TaskType);
	if (bPlayerPostureTutorialGuideActive)
	{
		PendingTutorialTaskUIType = TaskTypeValue;
		return;
	}

	PendingTutorialTaskUIType = INDEX_NONE;
	if (CombatHUDWidget)
	{
		CombatHUDWidget->ShowTutorialTask(TaskTypeValue);
	}
}

void AJunPlayerController::HideTutorialTaskUI()
{
	PendingTutorialTaskUIType = INDEX_NONE;
	if (CombatHUDWidget)
	{
		CombatHUDWidget->HideTutorialTask();
	}
}

void AJunPlayerController::StartTutorialTransition(AJunTutorialNPC* NPC)
{
	if (!JunPlayer || !CombatHUDWidget || !NPC)
	{
		if (JunPlayer)
		{
			JunPlayer->EndTutorialControlLock();
		}
		return;
	}

	PendingTutorialNPC = NPC;
	PreTutorialPlayerTransform = JunPlayer->GetActorTransform();
	bHasPreTutorialPlayerTransform = true;
	bTutorialEndTransition = false;
	TutorialTransitionState = EJunTutorialTransitionState::FadingIn;
	TutorialFullBlackHoldRemainTime = 0.f;
	CombatHUDWidget->StartTutorialDimBlackFadeIn();
}

void AJunPlayerController::StartTutorialEndTransition(AJunTutorialNPC* NPC)
{
	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	if (!JunPlayer || !CombatHUDWidget || !NPC || TutorialTransitionState != EJunTutorialTransitionState::None)
	{
		return;
	}

	PendingTutorialNPC = NPC;
	bTutorialEndTransition = true;
	TutorialFullBlackHoldRemainTime = 0.f;
	JunPlayer->BeginTutorialControlLock();
	CombatHUDWidget->HideTutorialTask();
	CombatHUDWidget->HideDialogue();
	CombatHUDWidget->StartTutorialDimBlackFadeIn();
	TutorialTransitionState = EJunTutorialTransitionState::FadingIn;
}

void AJunPlayerController::UpdateTutorialTransition(float DeltaTime)
{
	if (TutorialTransitionState == EJunTutorialTransitionState::None || !CombatHUDWidget)
	{
		return;
	}

	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	switch (TutorialTransitionState)
	{
	case EJunTutorialTransitionState::FadingIn:
		if (CombatHUDWidget->IsTutorialDimBlackOpaque())
		{
			if (bTutorialEndTransition)
			{
				FinishTutorialEndTransitionMove();
			}
			else
			{
				FinishTutorialTransitionMove();
			}
			TutorialFullBlackHoldRemainTime = FMath::Max(0.f, TutorialFullBlackHoldDuration);
			TutorialTransitionState = EJunTutorialTransitionState::FullBlackHold;
		}
		break;
	case EJunTutorialTransitionState::FullBlackHold:
		TutorialFullBlackHoldRemainTime = FMath::Max(0.f, TutorialFullBlackHoldRemainTime - DeltaTime);
		if (TutorialFullBlackHoldRemainTime <= 0.f)
		{
			CombatHUDWidget->StartTutorialDimBlackFadeOut();
			TutorialTransitionState = EJunTutorialTransitionState::FadingOut;
		}
		break;
	case EJunTutorialTransitionState::FadingOut:
		if (CombatHUDWidget->IsTutorialDimBlackHidden())
		{
			if (bTutorialEndTransition)
			{
				bTutorialEndTransition = false;
				PendingTutorialNPC = nullptr;
				TutorialTransitionState = EJunTutorialTransitionState::None;
				break;
			}

			if (JunPlayer)
			{
				JunPlayer->EndTutorialControlLock();
			}
			if (AJunTutorialNPC* NPC = PendingTutorialNPC.Get())
			{
				NPC->StartTutorialEquip();
			}
			PendingTutorialNPC = nullptr;
			TutorialTransitionState = EJunTutorialTransitionState::None;
		}
		break;
	case EJunTutorialTransitionState::None:
	default:
		break;
	}
}

void AJunPlayerController::FinishTutorialEndTransitionMove()
{
	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	if (JunPlayer)
	{
		JunPlayer->RestoreResourcesAfterTutorial();

		if (JunPlayer->IsLockOn())
		{
			JunPlayer->ToggleLockOn();
		}

		if (bHasPreTutorialPlayerTransform)
		{
			JunPlayer->SetActorLocationAndRotation(
				PreTutorialPlayerTransform.GetLocation(),
				PreTutorialPlayerTransform.GetRotation().Rotator(),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}

		JunPlayer->SetDesiredMoveAxes(0.f, 0.f);
		JunPlayer->OnMoveInputReleased();
		if (UCharacterMovementComponent* MovementComponent = JunPlayer->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	if (AJunTutorialNPC* NPC = PendingTutorialNPC.Get())
	{
		NPC->ReturnToInitialPlacement();
		if (JunPlayer && NPC->StartTutorialEndDialogue(JunPlayer))
		{
			ActiveDialogueNPC = NPC;
			JunPlayer->BeginDialogueCamera();
			ShowDialogueLineFromNPC(NPC);
			JunPlayer->SnapCameraToLookAt(NPC->GetActorLocation(), -12.f);
		}
		else if (JunPlayer)
		{
			JunPlayer->EndTutorialControlLock();
		}
	}
}

void AJunPlayerController::FinishTutorialTransitionMove()
{
	if (!JunPlayer)
	{
		JunPlayer = Cast<AJunPlayer>(GetPawn());
	}

	if (JunPlayer)
	{
		if (AJunPlayerRespawnPoint* TutorialStartPoint = AJunPlayerRespawnPoint::FindPlayerRespawnPoint(
			this,
			EJunPlayerRespawnPointType::TutorialStart))
		{
			TutorialStartPoint->MoveActorToRespawnPoint(JunPlayer);
		}

		JunPlayer->SetDesiredMoveAxes(0.f, 0.f);
		JunPlayer->OnMoveInputReleased();
	}

	if (AJunTutorialNPC* NPC = PendingTutorialNPC.Get())
	{
		if (AJunTutorialNPCPlacementPoint* PlacementPoint = AJunTutorialNPCPlacementPoint::FindTutorialNPCPlacementPoint(this))
		{
			NPC->MoveToTutorialPlacementPoint(PlacementPoint);
		}

		NPC->BeginTutorialCutsceneWait(JunPlayer);

		if (JunPlayer)
		{
			JunPlayer->SnapCameraToLookAt(NPC->GetActorLocation(), -24.f);
		}
	}
}

AJunTutorialNPC* AJunPlayerController::FindAvailableDialogueNPC() const
{
	const UWorld* World = GetWorld();
	if (!World || !JunPlayer)
	{
		return nullptr;
	}

	AJunTutorialNPC* BestNPC = nullptr;
	float BestDistanceSq = FLT_MAX;

	for (TActorIterator<AJunTutorialNPC> It(World); It; ++It)
	{
		AJunTutorialNPC* NPC = *It;
		if (!NPC || !NPC->CanStartDialogue(JunPlayer))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(JunPlayer->GetActorLocation(), NPC->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestNPC = NPC;
		}
	}

	return BestNPC;
}

void AJunPlayerController::ShowDialogueLineFromNPC(AJunTutorialNPC* NPC)
{
	if (!NPC || !CombatHUDWidget || !NPC->HasCurrentDialogueLine())
	{
		if (CombatHUDWidget)
		{
			CombatHUDWidget->HideDialogue();
		}
		return;
	}

	const FJunTutorialNPCDialogueLine DialogueLine = NPC->GetCurrentDialogueLine();
	CombatHUDWidget->ShowDialogue(DialogueLine.DialogueText, DialogueLine.SpeakerName);
}
