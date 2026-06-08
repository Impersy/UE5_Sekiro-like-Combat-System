#include "Character/JunTutorialNPC.h"

#include "AI/JunAIController.h"
#include "Animation/AnimInstance.h"
#include "Character/JunPlayer.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Level/JunTutorialNPCPlacementPoint.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "UI/JunMonsterHUDWidget.h"

AJunTutorialNPC::AJunTutorialNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AJunAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bStartWithPatrol = false;
	bStartWithCutsceneWait = false;
	Hp = 100;
	MaxHp = 100;
	MaxPosture = 100.f;

	DialogueRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DialogueRangeSphere"));
	DialogueRangeSphere->SetupAttachment(RootComponent);
	DialogueRangeSphere->InitSphereRadius(DialogueRangeRadius);
	DialogueRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DialogueRangeSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DialogueRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DialogueRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DialogueRangeSphere->SetGenerateOverlapEvents(true);

	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(RootComponent);
	InteractionPromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	InteractionPromptWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPromptWidget->SetHiddenInGame(true);
	ApplyInteractionPromptSettings();

	TutorialTasks =
	{
		EJunTutorialTaskType::LockOn,
		EJunTutorialTaskType::BasicAttackComboFinalHit,
		EJunTutorialTaskType::JumpAttackHit,
		EJunTutorialTaskType::DashAttackHit,
		EJunTutorialTaskType::HeavyAttackComboHit,
		EJunTutorialTaskType::HeavyChargeHit,
		EJunTutorialTaskType::JigenSecondHit,
		EJunTutorialTaskType::DrinkPotion,
		EJunTutorialTaskType::GuardPostureRecovery,
		EJunTutorialTaskType::ParryThreeTimes,
		EJunTutorialTaskType::AirParryOnce,
		EJunTutorialTaskType::MikiriCounter,
		EJunTutorialTaskType::JumpCounterStomp,
		EJunTutorialTaskType::Execution
	};
}

void AJunTutorialNPC::BeginPlay()
{
	Super::BeginPlay();

	InitialPlacementTransform = GetActorTransform();
	ApplyInteractionPromptSettings();

	if (DialogueRangeSphere)
	{
		DialogueRangeSphere->SetSphereRadius(FMath::Max(0.1f, DialogueRangeRadius), true);
		DialogueRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AJunTutorialNPC::OnDialogueRangeBeginOverlap);
		DialogueRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AJunTutorialNPC::OnDialogueRangeEndOverlap);
	}

	StopAIMovement();
	StopCombatBGM();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	bRunLocomotionRequested = false;
	CurrentState = EMonsterState::Idle;
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->bOrientRotationToMovement = false;
	}

	if (bHideWeaponWhileWaitingForDialogue)
	{
		SetWeaponVisible(false);
	}

	SetInteractionPromptVisible(false);
	PlayIdleWaitMontage();
}

void AJunTutorialNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateInteractionPromptBillboard();
	UpdateTutorialNPCHealthRestore(DeltaSeconds);
	UpdateTutorialNPCPostureDrain(DeltaSeconds);
	UpdateTutorialTaskTransition(DeltaSeconds);
	UpdateTutorialTask(DeltaSeconds);
	DrawTutorialTaskDebug();
	UpdateTutorialHomeReturn();
}

bool AJunTutorialNPC::CanUpdateBehavior() const
{
	return bTutorialCombatEnabled && Super::CanUpdateBehavior();
}

void AJunTutorialNPC::UpdateCombat(float DeltaTime)
{
	if (TutorialMode == EJunTutorialNPCMode::PassiveDummy ||
		TutorialMode == EJunTutorialNPCMode::AttackTraining ||
		TutorialMode == EJunTutorialNPCMode::ExecutionTraining)
	{
		if (!CurrentTarget)
		{
			SetMonsterState(EMonsterState::Idle);
			return;
		}

		if (TryHandleDeadCurrentTarget(EMonsterState::Idle))
		{
			CombatMoveInput = FVector2D::ZeroVector;
			return;
		}

		if (!CanRemainInCombat(CurrentTarget))
		{
			ClearCurrentTarget();
			CombatMoveInput = FVector2D::ZeroVector;
			SetMonsterState(EMonsterState::Idle);
			return;
		}

		if (bTutorialReturningHome)
		{
			StopTutorialTrainingAttackRangeAdjustment();
			return;
		}

		if (bIsAttacking)
		{
			StopTutorialTrainingAttackRangeAdjustment();
			SetDesiredMoveAxes(0.f, 0.f);
			CombatMoveInput = FVector2D::ZeroVector;
			return;
		}

		if (bTutorialTrainingAttackRangeAdjusting)
		{
			UpdateTutorialHomeReturnFacing(DeltaTime);
			return;
		}

		StopAIMovement();
		SetDesiredMoveAxes(0.f, 0.f);
		CombatMoveInput = FVector2D::ZeroVector;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		UpdateCombatFacing(DeltaTime);
		return;
	}

	Super::UpdateCombat(DeltaTime);
}

bool AJunTutorialNPC::CanBeLockOnTarget() const
{
	return bTutorialCombatEnabled && Super::CanBeLockOnTarget();
}

bool AJunTutorialNPC::ShouldShowBossCombatHUD() const
{
	return false;
}

bool AJunTutorialNPC::IsInCombat()
{
	if (bTutorialCombatEnabled &&
		(TutorialMode == EJunTutorialNPCMode::PassiveDummy ||
		 TutorialMode == EJunTutorialNPCMode::AttackTraining ||
		 TutorialMode == EJunTutorialNPCMode::ExecutionTraining))
	{
		return true;
	}

	return Super::IsInCombat();
}

bool AJunTutorialNPC::IsFinalExecution() const
{
	return false;
}

bool AJunTutorialNPC::TryBeginExecutionBy(AJunPlayer* Player)
{
	const bool bStarted = Super::TryBeginExecutionBy(Player);
	if (bStarted)
	{
		NotifyTutorialExecutionStarted(Player);
	}
	return bStarted;
}

void AJunTutorialNPC::TriggerPendingExecutionMontage(bool bApplyResultImmediately)
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	UAnimMontage* MontageToPlay = ExecutedMontage.Get();
	if (MontageToPlay)
	{
		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunTutorialNPC::OnTutorialExecutionMontageEnded);
			MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunTutorialNPC::OnTutorialExecutionMontageEnded);
			CurrentExecutionMontage = MontageToPlay;
		}

		PlayAnimMontage(MontageToPlay);
	}

	if (bApplyResultImmediately)
	{
		ApplyPendingExecutionResult();
	}

	if (!MontageToPlay)
	{
		FinishExecutionRecovery();
	}
}

void AJunTutorialNPC::ApplyPendingExecutionResult()
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	bExecutionResultApplied = true;
	CurrentLifeCount = FMath::Max(0, CurrentLifeCount - 1);
	CurrentPosture = 0.f;
	SetMonsterWorldHUDRevealed(true);
	UpdateMonsterWorldHUD();
	RestoreTutorialNPCHealth();
}

void AJunTutorialNPC::OnTutorialExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentExecutionMontage)
	{
		return;
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunTutorialNPC::OnTutorialExecutionMontageEnded);
	}

	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();

	if (!bBeingExecuted || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	FinishExecutionRecovery();
	RestoreTutorialNPCHealth();
}

void AJunTutorialNPC::EnterCombatState()
{
	bIsHasTarget = (CurrentTarget != nullptr);
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	ResetCombatTurnState();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = false;
	}
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
}

bool AJunTutorialNPC::TryHandleIncomingHitBeforeDamage(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!bTutorialCombatEnabled)
	{
		return true;
	}

	if (AJunPlayer* Player = Cast<AJunPlayer>(DamageCauser))
	{
		NotifyTutorialAttackHit(Player, DefenseRuleData.TutorialAttackType);
	}

	if (TutorialMode == EJunTutorialNPCMode::PassiveDummy)
	{
		return false;
	}

	return Super::TryHandleIncomingHitBeforeDamage(
		HitType,
		DamageAmount,
		DamageCauser,
		SwingDirection,
		DefenseKnockbackData,
		DefenseRuleData);
}

void AJunTutorialNPC::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	if (!bTutorialCombatEnabled || Damage <= 0)
	{
		return;
	}

	const int32 PreviousHp = Hp;
	Hp = FMath::Clamp(Hp - Damage, 1, FMath::Max(1, MaxHp));
	const int32 AppliedDamage = FMath::Max(0, PreviousHp - Hp);

	if (AppliedDamage > 0)
	{
		PlayHitDamageSound();
		SetMonsterWorldHUDRevealed(true);
	}

	if (Hp <= 1)
	{
		RestoreTutorialNPCHealth();
	}

	if (CurrentState == EMonsterState::Dead)
	{
		SetMonsterState(EMonsterState::Combat);
	}
	RemoveGameplayTag(JunGameplayTags::State_Condition_Dead);
	bExecutionReady = false;
	bBeingExecuted = false;
	bExecutionResultApplied = false;
	ExecutionReadyRemainTime = 0.f;

	if (!IsExecutionTrainingEnabled())
	{
		ClampTutorialPostureForCurrentMode();
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
	}
}

void AJunTutorialNPC::NotifyAttackParriedBy(
	AJunPlayer* Parrier,
	float PostureScale,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!bTutorialCombatEnabled)
	{
		return;
	}

	Super::NotifyAttackParriedBy(Parrier, PostureScale, DefenseRuleData);
	if (!IsExecutionTrainingEnabled())
	{
		ClampTutorialPostureForCurrentMode();
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
	}
}

bool AJunTutorialNPC::NotifyMikiriCounteredBy(AJunPlayer* CounterPlayer)
{
	if (!bTutorialCombatEnabled)
	{
		return false;
	}

	UAnimInstance* CurrentAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* InterruptedMontage = CurrentAttackMontage.Get();
	UAnimMontage* CounterMontage = TutorialMikiriCounteredMontage.Get();

	StopAllAttackTraces();
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	EndAttackFacingWindow();
	AttackFacingRemainTime = 0.f;

	if (CurrentAnimInstance && InterruptedMontage)
	{
		CurrentAnimInstance->Montage_Stop(FMath::Max(0.f, TutorialMikiriCounteredBlendOutTime), InterruptedMontage);
	}

	if (CurrentAnimInstance && CounterMontage)
	{
		CurrentAttackMontage = CounterMontage;
		CurrentAttackSelection.Montage = CounterMontage;
		CurrentAttackSelection.PlayRate = TutorialMikiriCounteredPlayRate;
		CurrentAttackSelection.bFaceTargetDuringAttack = true;
		CurrentAttackSelection.FacingDuration = 0.f;
		AttackTime = CounterMontage->GetPlayLength() / FMath::Max(TutorialMikiriCounteredPlayRate, KINDA_SMALL_NUMBER);
		bIsAttacking = true;

		const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, TutorialMikiriCounteredBlendInTime));
		if (CurrentAnimInstance->Montage_PlayWithBlendSettings(CounterMontage, BlendInSettings, TutorialMikiriCounteredPlayRate) <= 0.f)
		{
			CurrentAttackMontage = nullptr;
			AttackTime = 0.f;
			bIsAttacking = false;
			return false;
		}

		AddPosture(TutorialMikiriCounterPostureGain);
	}
	else
	{
		FinishAttack();
	}

	const bool bHandled = true;
	if (!IsExecutionTrainingEnabled())
	{
		ClampTutorialPostureForCurrentMode();
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
	}
	return bHandled;
}

void AJunTutorialNPC::AddPosture(float Amount)
{
	if (!bTutorialCombatEnabled || MaxPosture <= 0.f || Amount <= 0.f)
	{
		return;
	}

	if (!IsExecutionTrainingEnabled())
	{
		const float NonExecutionPostureCap = FMath::Max(0.f, MaxPosture - 1.f);
		CurrentPosture = FMath::Clamp(CurrentPosture + Amount, 0.f, NonExecutionPostureCap);
		PostureRecoveryDelayRemainTime = PostureRecoveryDelay;
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
		return;
	}

	const bool bWasBelowMaxPosture = CurrentPosture < MaxPosture;
	Super::AddPosture(Amount);
	if (bWasBelowMaxPosture && CurrentPosture >= MaxPosture)
	{
		SetMonsterWorldHUDRevealed(true);
		UpdateMonsterWorldHUD();
		if (MonsterHUDWidgetComponent)
		{
			if (UJunMonsterHUDWidget* MonsterHUDWidget = Cast<UJunMonsterHUDWidget>(MonsterHUDWidgetComponent->GetWidget()))
			{
				MonsterHUDWidget->PlayMonsterPostureBreakGlow();
			}
		}
	}
}

bool AJunTutorialNPC::CanStartDialogue(const AJunPlayer* Player) const
{
	return Player &&
		TutorialMode == EJunTutorialNPCMode::DialogueOnly &&
		!bTutorialCombatEnabled &&
		bPlayerInDialogueRange &&
		NearbyPlayer == Player &&
		!bDialogueActive &&
		CurrentState != EMonsterState::Dead &&
		!bExecutionReady &&
		!bBeingExecuted;
}

bool AJunTutorialNPC::HasCurrentDialogueLine() const
{
	return Dialogues.IsValidIndex(CurrentDialogueIndex);
}

FJunTutorialNPCDialogueLine AJunTutorialNPC::GetCurrentDialogueLine() const
{
	return HasCurrentDialogueLine()
		? Dialogues[CurrentDialogueIndex]
		: FJunTutorialNPCDialogueLine();
}

void AJunTutorialNPC::ResetDialogue()
{
	CurrentDialogueIndex = Dialogues.Num() > 0 ? 0 : INDEX_NONE;
	if (HasCurrentDialogueLine())
	{
		OnDialogueLineChanged(Dialogues[CurrentDialogueIndex], CurrentDialogueIndex);
	}
}

bool AJunTutorialNPC::AdvanceDialogue()
{
	if (!bDialogueActive)
	{
		return false;
	}

	if (Dialogues.Num() <= 0)
	{
		FinishDialogue();
		return false;
	}

	const int32 NextDialogueIndex = CurrentDialogueIndex < 0 ? 0 : CurrentDialogueIndex + 1;
	if (!Dialogues.IsValidIndex(NextDialogueIndex))
	{
		FinishDialogue();
		return false;
	}

	CurrentDialogueIndex = NextDialogueIndex;
	OnDialogueLineChanged(Dialogues[CurrentDialogueIndex], CurrentDialogueIndex);
	return true;
}

bool AJunTutorialNPC::TryStartDialogue(AJunPlayer* Player)
{
	if (!CanStartDialogue(Player))
	{
		return false;
	}

	bDialogueActive = true;
	SetInteractionPromptVisible(false);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	ResetDialogue();
	OnDialogueStarted(Player);
	return true;
}

void AJunTutorialNPC::SetTutorialCombatEnabled(bool bEnabled)
{
	if (bTutorialCombatEnabled == bEnabled)
	{
		return;
	}

	bTutorialCombatEnabled = bEnabled;
	if (bTutorialCombatEnabled)
	{
		bDialogueActive = false;
		SetInteractionPromptVisible(false);
		if (!bHideWeaponWhileWaitingForDialogue)
		{
			SetWeaponVisible(true);
		}
		return;
	}

	StopAIMovement();
	StopCombatBGM();
	ClearCurrentTarget();
	bDialogueActive = false;
	SetMonsterState(EMonsterState::Idle);
	SetInteractionPromptVisible(bPlayerInDialogueRange);
	if (bHideWeaponWhileWaitingForDialogue)
	{
		SetWeaponVisible(false);
	}
	PlayIdleWaitMontage();
}

void AJunTutorialNPC::SetTutorialMode(EJunTutorialNPCMode NewMode)
{
	TutorialMode = NewMode;
	bTutorialCombatEnabled = TutorialMode != EJunTutorialNPCMode::DialogueOnly;
	if (TutorialMode != EJunTutorialNPCMode::DialogueOnly)
	{
		bDialogueActive = false;
		CurrentDialogueIndex = INDEX_NONE;
	}
	bDisablePostureGain = TutorialMode == EJunTutorialNPCMode::DialogueOnly;
	if (!IsExecutionTrainingEnabled())
	{
		ClampTutorialPostureForCurrentMode();
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
	}

	if (TutorialMode == EJunTutorialNPCMode::DialogueOnly)
	{
		SetTutorialCombatEnabled(false);
	}
	else
	{
		SetInteractionPromptVisible(false);
	}
}

void AJunTutorialNPC::BeginTutorialCutsceneWait(AJunPlayer* Player)
{
	bTutorialStarted = true;
	TutorialHomeLocation = GetActorLocation();
	RefreshTutorialHomeReturnGoalLocation();
	bTutorialReturningHome = false;
	bTutorialHomeMoveRequested = false;
	TutorialHomeReturnMoveDelayRemainTime = 0.f;
	CurrentTarget = Player;
	bIsHasTarget = Player != nullptr;
	SetTutorialMode(EJunTutorialNPCMode::DialogueOnly);
	SetMonsterState(EMonsterState::CutsceneWait);
}

void AJunTutorialNPC::StartTutorialEquip()
{
	bTutorialStarted = true;
	TutorialHomeLocation = GetActorLocation();
	RefreshTutorialHomeReturnGoalLocation();
	bTutorialReturningHome = false;
	bTutorialHomeMoveRequested = false;
	TutorialHomeReturnMoveDelayRemainTime = 0.f;
	SetTutorialMode(EJunTutorialNPCMode::PassiveDummy);
	bCutsceneTriggered = true;
	if (AJunPlayer* Player = Cast<AJunPlayer>(CurrentTarget.Get()))
	{
		StartTutorialTasks(Player);
	}

	if (PlayCutsceneWaitEquipMontage(CutsceneWaitEquipBlendInTime))
	{
		return;
	}

	AttachWeaponToHandSocket();
	SetMonsterState(EMonsterState::BattleStart);
}

void AJunTutorialNPC::StartTutorialTasks(AJunPlayer* Player)
{
	if (!Player || TutorialTasks.Num() <= 0)
	{
		return;
	}

	bTutorialStarted = true;
	bTutorialTasksCompleted = false;
	bTutorialTaskTransitionPending = false;
	bTutorialTaskTransitionWaitingAtHome = false;
	bTutorialTaskTransitionWaitedForHitReact = false;
	TutorialTaskTransitionSourceMode = EJunTutorialNPCMode::DialogueOnly;
	PendingTutorialTaskIndex = INDEX_NONE;
	TutorialTaskTransitionWaitRemainTime = 0.f;
	CurrentTarget = Player;
	bIsHasTarget = true;
	CurrentTutorialTaskIndex = 0;
	ResetTutorialTaskRuntime();
	ApplyTutorialTaskMode();
	OnTutorialTaskChanged(GetCurrentTutorialTask(), CurrentTutorialTaskIndex);
}

EJunTutorialTaskType AJunTutorialNPC::GetCurrentTutorialTask() const
{
	return TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex)
		? TutorialTasks[CurrentTutorialTaskIndex]
		: EJunTutorialTaskType::LockOn;
}

bool AJunTutorialNPC::IsCurrentTutorialTask(EJunTutorialTaskType TaskType) const
{
	return !bTutorialTasksCompleted &&
		TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex) &&
		TutorialTasks[CurrentTutorialTaskIndex] == TaskType;
}

void AJunTutorialNPC::CompleteCurrentTutorialTask()
{
	if (bTutorialTasksCompleted ||
		bTutorialTaskTransitionPending ||
		!TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex))
	{
		return;
	}

	BeginTutorialTaskTransition(CurrentTutorialTaskIndex + 1);
}

void AJunTutorialNPC::BeginTutorialTaskTransition(int32 NextTaskIndex)
{
	bTutorialTaskTransitionPending = true;
	bTutorialTaskTransitionWaitingAtHome = false;
	TutorialTaskTransitionSourceMode = TutorialMode;
	bTutorialTaskTransitionWaitedForHitReact =
		TutorialMode == EJunTutorialNPCMode::PassiveDummy ||
		IsInHitReact();
	PendingTutorialTaskIndex = NextTaskIndex;
	TutorialTaskTransitionWaitRemainTime = 0.f;

	if (bIsAttacking || IsInHitReact())
	{
		return;
	}

	BeginTutorialTaskTransitionHomeReturn();
}

void AJunTutorialNPC::BeginTutorialTaskTransitionHomeReturn()
{
	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

	StopAllAttackTraces();
	StopMonsterCodeMove(true);
	StopTutorialTrainingAttackRangeAdjustment();
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	TutorialTrainingAttackCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	SetTutorialMode(EJunTutorialNPCMode::PassiveDummy);
	RefreshTutorialHomeReturnGoalLocation();
	bTutorialHomeMoveRequested = false;
	const float DistanceToHome = FVector::Dist2D(GetActorLocation(), TutorialHomeReturnGoalLocation);
	if (DistanceToHome <= GetTutorialHomeReturnEffectiveAcceptRadius())
	{
		bTutorialReturningHome = false;
		bTutorialTaskTransitionWaitingAtHome = true;
		TutorialTaskTransitionWaitRemainTime = FMath::Max(0.f, TutorialTaskTransitionWaitDuration);
		return;
	}

	bTutorialReturningHome = true;
	switch (TutorialTaskTransitionSourceMode)
	{
	case EJunTutorialNPCMode::PassiveDummy:
		TutorialHomeReturnMoveDelayRemainTime = FMath::Max(0.f, TutorialDummyHomeReturnMoveStartDelay);
		break;
	case EJunTutorialNPCMode::AttackTraining:
		TutorialHomeReturnMoveDelayRemainTime = FMath::Max(0.f, TutorialAttackHomeReturnMoveStartDelay);
		break;
	default:
		TutorialHomeReturnMoveDelayRemainTime = bTutorialTaskTransitionWaitedForHitReact
			? FMath::Max(0.f, TutorialDummyHomeReturnMoveStartDelay)
			: 0.f;
		break;
	}
	bTutorialTaskTransitionWaitedForHitReact = false;
	UpdateTutorialHomeReturnMoveAxes();
}

void AJunTutorialNPC::FinishTutorialTaskTransition()
{
	if (!bTutorialTaskTransitionPending)
	{
		return;
	}

	CurrentTutorialTaskIndex = PendingTutorialTaskIndex;
	PendingTutorialTaskIndex = INDEX_NONE;
	bTutorialTaskTransitionPending = false;
	bTutorialTaskTransitionWaitingAtHome = false;
	bTutorialTaskTransitionWaitedForHitReact = false;
	TutorialTaskTransitionSourceMode = EJunTutorialNPCMode::DialogueOnly;
	TutorialTaskTransitionWaitRemainTime = 0.f;
	ResetTutorialTaskRuntime();

	if (!TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex))
	{
		bTutorialTasksCompleted = true;
		SetTutorialMode(EJunTutorialNPCMode::PassiveDummy);
		RestoreTutorialNPCHealth();
		OnTutorialCompleted();
		return;
	}

	ApplyTutorialTaskMode();
	OnTutorialTaskChanged(GetCurrentTutorialTask(), CurrentTutorialTaskIndex);
}

void AJunTutorialNPC::UpdateTutorialTaskTransition(float DeltaSeconds)
{
	if (!bTutorialTaskTransitionPending)
	{
		return;
	}

	if (!bTutorialTaskTransitionWaitingAtHome)
	{
		if (!bTutorialReturningHome && !bIsAttacking && !IsInHitReact())
		{
			BeginTutorialTaskTransitionHomeReturn();
		}
		return;
	}

	if (bTutorialTaskTransitionWaitingAtHome)
	{
		TutorialTaskTransitionWaitRemainTime = FMath::Max(0.f, TutorialTaskTransitionWaitRemainTime - DeltaSeconds);
		if (TutorialTaskTransitionWaitRemainTime > 0.f)
		{
			return;
		}

		FinishTutorialTaskTransition();
	}
}

void AJunTutorialNPC::NotifyTutorialLockOn(AJunPlayer* Player)
{
	if (Player && Player == CurrentTarget && IsCurrentTutorialTask(EJunTutorialTaskType::LockOn))
	{
		CompleteCurrentTutorialTask();
	}
}

void AJunTutorialNPC::NotifyTutorialAttackHit(AJunPlayer* Player, EJunTutorialAttackType AttackType)
{
	if (!Player || Player != CurrentTarget)
	{
		return;
	}

	switch (AttackType)
	{
	case EJunTutorialAttackType::BasicComboFinal:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::BasicAttackComboFinalHit))
		{
			CompleteCurrentTutorialTask();
		}
		break;
	case EJunTutorialAttackType::JumpAttack:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::JumpAttackHit))
		{
			CompleteCurrentTutorialTask();
		}
		break;
	case EJunTutorialAttackType::DashAttack:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::DashAttackHit))
		{
			CompleteCurrentTutorialTask();
		}
		break;
	case EJunTutorialAttackType::HeavyAttack1:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::HeavyAttackComboHit))
		{
			TutorialHeavyHitMask |= 1;
		}
		break;
	case EJunTutorialAttackType::HeavyAttack2:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::HeavyAttackComboHit))
		{
			TutorialHeavyHitMask |= 2;
			if ((TutorialHeavyHitMask & 3) == 3)
			{
				CompleteCurrentTutorialTask();
			}
		}
		break;
	case EJunTutorialAttackType::HeavyCharge:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::HeavyChargeHit))
		{
			CompleteCurrentTutorialTask();
		}
		break;
	case EJunTutorialAttackType::Jigen2:
		if (IsCurrentTutorialTask(EJunTutorialTaskType::JigenSecondHit))
		{
			CompleteCurrentTutorialTask();
		}
		break;
	default:
		break;
	}
}

void AJunTutorialNPC::NotifyTutorialDrinkPotion(AJunPlayer* Player)
{
	if (Player && Player == CurrentTarget && IsCurrentTutorialTask(EJunTutorialTaskType::DrinkPotion))
	{
		CompleteCurrentTutorialTask();
	}
}

void AJunTutorialNPC::NotifyTutorialParrySuccess(AJunPlayer* Player, bool bAirParry)
{
	if (!Player || Player != CurrentTarget)
	{
		return;
	}

	if (bAirParry)
	{
		if (IsCurrentTutorialTask(EJunTutorialTaskType::AirParryOnce))
		{
			CompleteCurrentTutorialTask();
		}
		return;
	}

	if (IsCurrentTutorialTask(EJunTutorialTaskType::ParryThreeTimes))
	{
		++TutorialParrySuccessCount;
		if (TutorialParrySuccessCount >= 3)
		{
			CompleteCurrentTutorialTask();
		}
	}
}

void AJunTutorialNPC::NotifyTutorialMikiriSuccess(AJunPlayer* Player)
{
	if (Player && Player == CurrentTarget && IsCurrentTutorialTask(EJunTutorialTaskType::MikiriCounter))
	{
		CompleteCurrentTutorialTask();
	}
}

void AJunTutorialNPC::NotifyTutorialJumpCounterStompSuccess(AJunPlayer* Player)
{
	if (Player && Player == CurrentTarget && IsCurrentTutorialTask(EJunTutorialTaskType::JumpCounterStomp))
	{
		CompleteCurrentTutorialTask();
	}
}

void AJunTutorialNPC::NotifyTutorialExecutionStarted(AJunPlayer* Player)
{
	if (Player && Player == CurrentTarget && IsCurrentTutorialTask(EJunTutorialTaskType::Execution))
	{
		CompleteCurrentTutorialTask();
	}
}

bool AJunTutorialNPC::MoveToTutorialPlacementPoint(AJunTutorialNPCPlacementPoint* PlacementPoint)
{
	if (!PlacementPoint)
	{
		return false;
	}

	StopAIMovement();
	StopCombatBGM();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	bRunLocomotionRequested = false;
	bDialogueActive = false;
	CurrentDialogueIndex = INDEX_NONE;
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	const bool bMoved = PlacementPoint->MoveNPCToPlacement(this);
	TutorialHomeLocation = GetActorLocation();
	RefreshTutorialHomeReturnGoalLocation();
	bTutorialReturningHome = false;
	bTutorialHomeMoveRequested = false;
	TutorialHomeReturnMoveDelayRemainTime = 0.f;
	bTutorialTaskTransitionPending = false;
	bTutorialTaskTransitionWaitingAtHome = false;
	bTutorialTaskTransitionWaitedForHitReact = false;
	TutorialTaskTransitionSourceMode = EJunTutorialNPCMode::DialogueOnly;
	PendingTutorialTaskIndex = INDEX_NONE;
	TutorialTaskTransitionWaitRemainTime = 0.f;
	if (bHideWeaponWhileWaitingForDialogue && !bTutorialCombatEnabled)
	{
		SetWeaponVisible(false);
	}

	SetMonsterState(EMonsterState::Idle);
	PlayIdleWaitMontage();
	return bMoved;
}

void AJunTutorialNPC::ReturnToInitialPlacement()
{
	StopAIMovement();
	StopCombatBGM();
	ClearCurrentTarget();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	bRunLocomotionRequested = false;
	bDialogueActive = false;
	CurrentDialogueIndex = INDEX_NONE;
	bTutorialStarted = false;
	bTutorialReturningHome = false;
	bTutorialHomeMoveRequested = false;
	TutorialHomeReturnMoveDelayRemainTime = 0.f;
	bTutorialTaskTransitionPending = false;
	bTutorialTaskTransitionWaitingAtHome = false;
	bTutorialTaskTransitionWaitedForHitReact = false;
	TutorialTaskTransitionSourceMode = EJunTutorialNPCMode::DialogueOnly;
	PendingTutorialTaskIndex = INDEX_NONE;
	TutorialTaskTransitionWaitRemainTime = 0.f;
	SetTutorialCombatEnabled(false);

	SetActorLocationAndRotation(
		InitialPlacementTransform.GetLocation(),
		InitialPlacementTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	SetMonsterState(EMonsterState::Idle);
	PlayIdleWaitMontage();
}

void AJunTutorialNPC::FinishDialogue()
{
	if (!bDialogueActive)
	{
		return;
	}

	bDialogueActive = false;
	CurrentDialogueIndex = INDEX_NONE;
	OnDialogueFinished();
	SetInteractionPromptVisible(bPlayerInDialogueRange);
	PlayIdleWaitMontage();
}

void AJunTutorialNPC::UpdateTutorialTask(float DeltaSeconds)
{
	if (bTutorialTasksCompleted ||
		bTutorialTaskTransitionPending ||
		!TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex))
	{
		return;
	}

	if (TutorialMode == EJunTutorialNPCMode::AttackTraining)
	{
		TryStartTutorialTrainingAttack(DeltaSeconds);
	}

	if (IsCurrentTutorialTask(EJunTutorialTaskType::GuardPostureRecovery))
	{
		if (AJunPlayer* Player = Cast<AJunPlayer>(CurrentTarget.Get()))
		{
			const float PlayerMaxPosture = Player->GetMaxPosture();
			const float StartPosture = PlayerMaxPosture * FMath::Clamp(TutorialGuardPostureRecoveryStartRatio, 0.f, 1.f);
			const float CompletePosture = PlayerMaxPosture * FMath::Clamp(TutorialGuardPostureRecoveryCompleteRatio, 0.f, 1.f);
			const bool bGuardRequirementMet =
				!bTutorialGuardPostureRecoveryRequiresGuard ||
				Player->IsGuardOn() ||
				Player->IsGuardPoseActive();
			if (PlayerMaxPosture > 0.f && !bGuardRequirementMet)
			{
				Player->SetPostureForTutorial(StartPosture);
				return;
			}

			if (PlayerMaxPosture > 0.f &&
				bGuardRequirementMet &&
				Player->GetCurrentPosture() <= CompletePosture)
			{
				CompleteCurrentTutorialTask();
			}
		}
	}
}

void AJunTutorialNPC::DrawTutorialTaskDebug() const
{
	if (!bDebugTutorialTaskProgress || !GEngine || !bTutorialStarted)
	{
		return;
	}

	const UEnum* TaskEnum = StaticEnum<EJunTutorialTaskType>();
	const UEnum* ModeEnum = StaticEnum<EJunTutorialNPCMode>();
	const FString TaskName = TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex) && TaskEnum
		? TaskEnum->GetNameStringByValue(static_cast<int64>(TutorialTasks[CurrentTutorialTaskIndex]))
		: (bTutorialTasksCompleted ? TEXT("Completed") : TEXT("None"));
	const FString ModeName = ModeEnum
		? ModeEnum->GetNameStringByValue(static_cast<int64>(TutorialMode))
		: FString::FromInt(static_cast<int32>(TutorialMode));

	const AJunPlayer* Player = Cast<AJunPlayer>(CurrentTarget.Get());
	const float PlayerPosture = Player ? Player->GetCurrentPosture() : 0.f;
	const float PlayerMaxPosture = Player ? Player->GetMaxPosture() : 0.f;

	const FString DebugText = FString::Printf(
		TEXT("Tutorial Task %d/%d : %s\nMode=%s  Done=%d\nHeavyMask=%d  Parry=%d/3  AttackCD=%.2f\nHP=%d/%d  Posture=%.1f/%.1f\nPlayerPosture=%.1f/%.1f"),
		TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex) ? CurrentTutorialTaskIndex + 1 : 0,
		TutorialTasks.Num(),
		*TaskName,
		*ModeName,
		bTutorialTasksCompleted ? 1 : 0,
		TutorialHeavyHitMask,
		TutorialParrySuccessCount,
		TutorialTrainingAttackCooldownRemainTime,
		Hp,
		MaxHp,
		CurrentPosture,
		MaxPosture,
		PlayerPosture,
		PlayerMaxPosture);

	GEngine->AddOnScreenDebugMessage(
		static_cast<uint64>(reinterpret_cast<UPTRINT>(this)) ^ 0x5455544Fu,
		0.f,
		bTutorialTasksCompleted ? FColor::Green : FColor::Yellow,
		DebugText);
}

void AJunTutorialNPC::ApplyTutorialTaskMode()
{
	if (!TutorialTasks.IsValidIndex(CurrentTutorialTaskIndex))
	{
		return;
	}

	const EJunTutorialTaskType Task = TutorialTasks[CurrentTutorialTaskIndex];
	switch (Task)
	{
	case EJunTutorialTaskType::ParryThreeTimes:
	case EJunTutorialTaskType::AirParryOnce:
	case EJunTutorialTaskType::MikiriCounter:
	case EJunTutorialTaskType::JumpCounterStomp:
		SetTutorialMode(EJunTutorialNPCMode::AttackTraining);
		break;
	case EJunTutorialTaskType::Execution:
		SetTutorialMode(EJunTutorialNPCMode::ExecutionTraining);
		StartTutorialNPCPostureDrainToZero();
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
		break;
	default:
		SetTutorialMode(EJunTutorialNPCMode::PassiveDummy);
		break;
	}

	if (Task == EJunTutorialTaskType::DrinkPotion)
	{
		if (AJunPlayer* Player = Cast<AJunPlayer>(CurrentTarget.Get()))
		{
			const float TargetHpRatio = FMath::Clamp(TutorialDrinkTaskTargetHpRatio, 0.f, 1.f);
			const int32 TargetHp = TargetHpRatio > 0.f
				? FMath::RoundToInt(static_cast<float>(Player->GetMaxHp()) * TargetHpRatio)
				: TutorialDrinkTaskTargetHp;
			Player->SetHpForTutorial(TargetHp);
		}
	}
	else if (Task == EJunTutorialTaskType::GuardPostureRecovery)
	{
		if (AJunPlayer* Player = Cast<AJunPlayer>(CurrentTarget.Get()))
		{
			const float TargetPosture = Player->GetMaxPosture() * FMath::Clamp(TutorialGuardPostureRecoveryStartRatio, 0.f, 1.f);
			Player->SetPostureForTutorial(TargetPosture);
		}
	}
}

void AJunTutorialNPC::TryStartTutorialTrainingAttack(float DeltaSeconds)
{
	if (bIsAttacking || IsInHitReact() || bTutorialReturningHome || bBeingExecuted || bExecutionReady)
	{
		return;
	}

	if (!CurrentTarget)
	{
		return;
	}

	TutorialTrainingAttackCooldownRemainTime = FMath::Max(0.f, TutorialTrainingAttackCooldownRemainTime - DeltaSeconds);
	if (TutorialTrainingAttackCooldownRemainTime > 0.f)
	{
		return;
	}

	UAnimMontage* Montage = GetTutorialAttackMontageForCurrentTask();
	if (!Montage)
	{
		StopTutorialTrainingAttackRangeAdjustment();
		return;
	}

	if (!UpdateTutorialTrainingAttackRange(DeltaSeconds))
	{
		return;
	}

	StartTutorialTrainingAttack(Montage, TutorialTrainingAttackPlayRate);
	TutorialTrainingAttackCooldownRemainTime = FMath::Max(0.f, TutorialTrainingAttackCooldown);
}

bool AJunTutorialNPC::UpdateTutorialTrainingAttackRange(float DeltaSeconds)
{
	if (!bUseTutorialTrainingAttackRange || !CurrentTarget)
	{
		StopTutorialTrainingAttackRangeAdjustment();
		return true;
	}

	const float MinRange = FMath::Max(0.f, TutorialTrainingAttackMinRange);
	const float MaxRange = FMath::Max(MinRange, TutorialTrainingAttackMaxRange);
	const float Tolerance = FMath::Max(0.f, TutorialTrainingAttackRangeTolerance);
	const float Distance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (Distance >= MinRange && Distance <= MaxRange)
	{
		StopTutorialTrainingAttackRangeAdjustment();
		return true;
	}

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		StopTutorialTrainingAttackRangeAdjustment();
		return true;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		StopTutorialTrainingAttackRangeAdjustment();
		return true;
	}

	bTutorialTrainingAttackRangeAdjusting = true;
	UpdateTutorialHomeReturnFacing(DeltaSeconds);

	FVector MoveDirection = ToTarget.GetSafeNormal();
	float AcceptanceRadius = Tolerance;
	if (Distance > MaxRange)
	{
		AcceptanceRadius = FMath::Max(5.f, MaxRange - Tolerance);
		AICon->MoveToActor(CurrentTarget.Get(), AcceptanceRadius, false);
	}
	else
	{
		MoveDirection *= -1.f;
		const FVector DesiredLocation = GetActorLocation() + MoveDirection * FMath::Max(MinRange - Distance + Tolerance, Tolerance);
		AICon->MoveToLocation(DesiredLocation, FMath::Max(5.f, Tolerance), false);
	}

	const float ForwardAxis = FVector::DotProduct(GetActorForwardVector(), MoveDirection);
	const float RightAxis = FVector::DotProduct(GetActorRightVector(), MoveDirection);
	SetDesiredMoveAxes(ForwardAxis, RightAxis);
	CombatMoveInput = FVector2D(ForwardAxis, RightAxis);
	return false;
}

void AJunTutorialNPC::StopTutorialTrainingAttackRangeAdjustment()
{
	if (!bTutorialTrainingAttackRangeAdjusting)
	{
		return;
	}

	bTutorialTrainingAttackRangeAdjusting = false;
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
}

bool AJunTutorialNPC::ShouldSuspendTutorialHomeReturnForAttackTraining() const
{
	return TutorialMode == EJunTutorialNPCMode::AttackTraining &&
		!bTutorialTaskTransitionPending &&
		!bTutorialTasksCompleted &&
		CurrentTarget != nullptr &&
		GetTutorialAttackMontageForCurrentTask() != nullptr;
}

void AJunTutorialNPC::StartTutorialTrainingAttack(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage)
	{
		return;
	}

	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	StopTutorialTrainingAttackRangeAdjustment();
	CurrentAttackSelection = FMonsterAttackSelection();
	CurrentAttackSelection.Montage = Montage;
	CurrentAttackSelection.AttackRange = DefaultAttackRange;
	CurrentAttackSelection.bFaceTargetDuringAttack = true;
	CurrentAttackSelection.FacingDuration = 0.f;
	CurrentAttackSelection.FacingInterpSpeed = AttackFacingInterpSpeed;
	CurrentAttackSelection.PlayRate = FMath::Max(PlayRate, KINDA_SMALL_NUMBER);
	CurrentAttackMontage = Montage;
	AttackTime = Montage->GetPlayLength() / CurrentAttackSelection.PlayRate;
	AttackFacingRemainTime = CurrentAttackSelection.FacingDuration;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	PlayAnimMontage(Montage, CurrentAttackSelection.PlayRate);
}

UAnimMontage* AJunTutorialNPC::GetTutorialAttackMontageForCurrentTask() const
{
	switch (GetCurrentTutorialTask())
	{
	case EJunTutorialTaskType::ParryThreeTimes:
		return TutorialParryTrainingAttackMontage.Get();
	case EJunTutorialTaskType::AirParryOnce:
		return TutorialAirParryTrainingAttackMontage.Get();
	case EJunTutorialTaskType::MikiriCounter:
		return TutorialMikiriTrainingAttackMontage.Get();
	case EJunTutorialTaskType::JumpCounterStomp:
		return TutorialJumpCounterTrainingAttackMontage.Get();
	default:
		return nullptr;
	}
}

void AJunTutorialNPC::ResetTutorialTaskRuntime()
{
	TutorialHeavyHitMask = 0;
	TutorialParrySuccessCount = 0;
	TutorialTrainingAttackCooldownRemainTime = 0.f;
	TutorialPostureDrainRemainTime = 0.f;
	TutorialPostureDrainStartValue = 0.f;
	StopTutorialTrainingAttackRangeAdjustment();
}

void AJunTutorialNPC::RestoreTutorialNPCHealth()
{
	const int32 TargetHp = FMath::Max(1, MaxHp);
	if (Hp >= TargetHp)
	{
		TutorialHealthRestoreRemainTime = 0.f;
		TutorialHealthRestorePendingAmount = 0.f;
		TutorialHealthRestoreAccumulator = 0.f;
		return;
	}

	if (Hp <= 0)
	{
		Hp = 1;
	}

	const int32 MissingHp = TargetHp - Hp;
	if (TutorialHealthRestoreDuration <= 0.f)
	{
		RestoreTutorialNPCHealthImmediately();
		return;
	}

	TutorialHealthRestorePendingAmount = static_cast<float>(MissingHp);
	TutorialHealthRestoreRemainTime = FMath::Max(TutorialHealthRestoreDuration, KINDA_SMALL_NUMBER);
	TutorialHealthRestoreAccumulator = 0.f;
}

void AJunTutorialNPC::RestoreTutorialNPCHealthImmediately()
{
	Hp = FMath::Max(1, MaxHp);
	TutorialHealthRestoreRemainTime = 0.f;
	TutorialHealthRestorePendingAmount = 0.f;
	TutorialHealthRestoreAccumulator = 0.f;
}

void AJunTutorialNPC::UpdateTutorialNPCHealthRestore(float DeltaSeconds)
{
	if (TutorialHealthRestoreRemainTime <= 0.f || TutorialHealthRestorePendingAmount <= 0.f)
	{
		return;
	}

	if (Hp >= MaxHp)
	{
		RestoreTutorialNPCHealthImmediately();
		return;
	}

	const float StepRatio = TutorialHealthRestoreRemainTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(DeltaSeconds / TutorialHealthRestoreRemainTime, 0.f, 1.f)
		: 1.f;
	const float StepHeal = TutorialHealthRestorePendingAmount * StepRatio;
	TutorialHealthRestorePendingAmount = FMath::Max(0.f, TutorialHealthRestorePendingAmount - StepHeal);
	TutorialHealthRestoreRemainTime = FMath::Max(0.f, TutorialHealthRestoreRemainTime - DeltaSeconds);

	TutorialHealthRestoreAccumulator += StepHeal;
	const int32 HealToApply = FMath::FloorToInt(TutorialHealthRestoreAccumulator);
	if (HealToApply > 0)
	{
		const int32 PreviousHp = Hp;
		Hp = FMath::Clamp(Hp + HealToApply, 1, FMath::Max(1, MaxHp));
		TutorialHealthRestoreAccumulator -= static_cast<float>(Hp - PreviousHp);
	}

	if (TutorialHealthRestoreRemainTime <= 0.f && TutorialHealthRestorePendingAmount > 0.f)
	{
		const int32 FinalHealToApply = FMath::CeilToInt(TutorialHealthRestorePendingAmount + TutorialHealthRestoreAccumulator);
		if (FinalHealToApply > 0)
		{
			Hp = FMath::Clamp(Hp + FinalHealToApply, 1, FMath::Max(1, MaxHp));
		}

		TutorialHealthRestoreRemainTime = 0.f;
		TutorialHealthRestorePendingAmount = 0.f;
		TutorialHealthRestoreAccumulator = 0.f;
	}
}

void AJunTutorialNPC::StartTutorialNPCPostureDrainToZero()
{
	if (CurrentPosture <= 0.f)
	{
		CurrentPosture = 0.f;
		TutorialPostureDrainRemainTime = 0.f;
		TutorialPostureDrainStartValue = 0.f;
		UpdateMonsterWorldHUD();
		return;
	}

	SetMonsterWorldHUDRevealed(true);

	if (TutorialPostureDrainToZeroDuration <= 0.f)
	{
		CurrentPosture = 0.f;
		TutorialPostureDrainRemainTime = 0.f;
		TutorialPostureDrainStartValue = 0.f;
		UpdateMonsterWorldHUD();
		return;
	}

	TutorialPostureDrainStartValue = CurrentPosture;
	TutorialPostureDrainRemainTime = FMath::Max(TutorialPostureDrainToZeroDuration, KINDA_SMALL_NUMBER);
}

void AJunTutorialNPC::UpdateTutorialNPCPostureDrain(float DeltaSeconds)
{
	if (TutorialPostureDrainRemainTime <= 0.f || TutorialPostureDrainStartValue <= 0.f)
	{
		return;
	}

	const float DrainSpeed = TutorialPostureDrainStartValue / FMath::Max(TutorialPostureDrainToZeroDuration, KINDA_SMALL_NUMBER);
	CurrentPosture = FMath::Max(0.f, CurrentPosture - DrainSpeed * DeltaSeconds);
	TutorialPostureDrainRemainTime = FMath::Max(0.f, TutorialPostureDrainRemainTime - DeltaSeconds);

	if (TutorialPostureDrainRemainTime <= 0.f || CurrentPosture <= KINDA_SMALL_NUMBER)
	{
		CurrentPosture = 0.f;
		TutorialPostureDrainRemainTime = 0.f;
		TutorialPostureDrainStartValue = 0.f;
	}

	UpdateMonsterWorldHUD();
}

void AJunTutorialNPC::ClampTutorialPostureForCurrentMode()
{
	if (IsExecutionTrainingEnabled())
	{
		CurrentPosture = FMath::Clamp(CurrentPosture, 0.f, MaxPosture);
		return;
	}

	const float NonExecutionPostureCap = FMath::Max(0.f, MaxPosture - 1.f);
	CurrentPosture = FMath::Clamp(CurrentPosture, 0.f, NonExecutionPostureCap);
}

void AJunTutorialNPC::UpdateTutorialHomeReturn()
{
	if (!bTutorialStarted)
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), TutorialHomeReturnGoalLocation);
	const float Distance = FMath::Sqrt(DistanceSq);
	const float EffectiveAcceptRadius = GetTutorialHomeReturnEffectiveAcceptRadius();
	if (bDebugTutorialHomeReturn)
	{
		const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
		const AAIController* AICon = Cast<AAIController>(GetController());
		const UPathFollowingComponent* PathFollowing = AICon ? AICon->GetPathFollowingComponent() : nullptr;
		const float RawHomeDistance = FVector::Dist2D(GetActorLocation(), TutorialHomeLocation);
		const float GoalOffset = FVector::Dist2D(TutorialHomeLocation, TutorialHomeReturnGoalLocation);
		const FString DebugMessage = FString::Printf(
			TEXT("TutorialHome | GoalDist=%.1f RawDist=%.1f Accept=%.1f GoalOffset=%.1f Returning=%d MoveReq=%d PF=%d Delay=%.2f Desired=(%.2f,%.2f) Speed=%.1f Accel=%.1f State=%d Mode=%d Can=%d"),
			Distance,
			RawHomeDistance,
			EffectiveAcceptRadius,
			GoalOffset,
			bTutorialReturningHome ? 1 : 0,
			bTutorialHomeMoveRequested ? 1 : 0,
			PathFollowing ? static_cast<int32>(PathFollowing->GetStatus()) : -1,
			TutorialHomeReturnMoveDelayRemainTime,
			GetDesiredMoveForward(),
			GetDesiredMoveRight(),
			MovementComponent ? MovementComponent->Velocity.Size2D() : 0.f,
			MovementComponent ? MovementComponent->GetCurrentAcceleration().Size2D() : 0.f,
			static_cast<int32>(CurrentState),
			static_cast<int32>(TutorialMode),
			CanUpdateTutorialHomeReturn() ? 1 : 0);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugMessage);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				reinterpret_cast<uint64>(this) & 0x7fffffff,
				0.f,
				FColor::Cyan,
				DebugMessage);
		}
	}

	if (Distance <= EffectiveAcceptRadius)
	{
		if (bTutorialReturningHome)
		{
			FinishTutorialHomeReturn();
		}
		return;
	}

	const bool bCanUpdateHomeReturn = CanUpdateTutorialHomeReturn() ||
		(bTutorialTaskTransitionPending &&
			bTutorialReturningHome &&
			!bIsAttacking &&
			!IsInHitReact() &&
			!bExecutionReady &&
			!bBeingExecuted);
	if (!bCanUpdateHomeReturn)
	{
		SetDesiredMoveAxes(0.f, 0.f);
		CombatMoveInput = FVector2D::ZeroVector;
		return;
	}

	if (ShouldSuspendTutorialHomeReturnForAttackTraining())
	{
		if (bTutorialReturningHome)
		{
			bTutorialReturningHome = false;
			bTutorialHomeMoveRequested = false;
			TutorialHomeReturnMoveDelayRemainTime = 0.f;
			if (!bTutorialTrainingAttackRangeAdjusting)
			{
				StopAIMovement();
				SetDesiredMoveAxes(0.f, 0.f);
				CombatMoveInput = FVector2D::ZeroVector;
			}
		}
		return;
	}

	if (bTutorialReturningHome)
	{
		if (bTutorialHomeMoveRequested)
		{
			const AAIController* AICon = Cast<AAIController>(GetController());
			const UPathFollowingComponent* PathFollowing = AICon ? AICon->GetPathFollowingComponent() : nullptr;
			const EPathFollowingStatus::Type PathStatus = PathFollowing
				? PathFollowing->GetStatus()
				: EPathFollowingStatus::Idle;
			if (PathStatus != EPathFollowingStatus::Moving &&
				PathStatus != EPathFollowingStatus::Waiting)
			{
				if (bDebugTutorialHomeReturn)
				{
					UE_LOG(LogTemp, Warning, TEXT("TutorialHomeMoveRequest | InactivePath ResetMoveReq PFStatus=%d GoalDist=%.1f Accept=%.1f"),
						static_cast<int32>(PathStatus),
						Distance,
						EffectiveAcceptRadius);
				}
				bTutorialHomeMoveRequested = false;
			}
		}

		UpdateTutorialHomeReturnMoveAxes();
		UpdateTutorialHomeReturnFacing(DeltaSeconds);

		if (TutorialHomeReturnMoveDelayRemainTime > 0.f)
		{
			TutorialHomeReturnMoveDelayRemainTime = FMath::Max(
				0.f,
				TutorialHomeReturnMoveDelayRemainTime - DeltaSeconds);
			return;
		}

		if (!bTutorialHomeMoveRequested)
		{
			RequestTutorialHomeReturnMove();
		}
		return;
	}

	if (DistanceSq <= FMath::Square(TutorialHomeReturnStartDistance))
	{
		return;
	}

	bTutorialReturningHome = true;
	bTutorialHomeMoveRequested = false;
	TutorialHomeReturnMoveDelayRemainTime = 0.f;
	bRunLocomotionRequested = false;
	UpdateTutorialHomeReturnMoveAxes();
	UpdateTutorialHomeReturnFacing(DeltaSeconds);
}

void AJunTutorialNPC::UpdateTutorialHomeReturnFacing(float DeltaSeconds)
{
	if (!CurrentTarget || DeltaSeconds <= 0.f)
	{
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation = ToTarget.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		TargetRotation,
		DeltaSeconds,
		CombatFacingInterpSpeed);

	SetActorRotation(NewRotation);
}

bool AJunTutorialNPC::RequestTutorialHomeReturnMove()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	const float EffectiveAcceptRadius = GetTutorialHomeReturnEffectiveAcceptRadius();
	if (!AICon)
	{
		if (bDebugTutorialHomeReturn)
		{
			UE_LOG(LogTemp, Warning, TEXT("TutorialHomeMoveRequest | Failed: NoAIController"));
		}
		return false;
	}

	const EPathFollowingRequestResult::Type Result = AICon->MoveToLocation(
		TutorialHomeReturnGoalLocation,
		EffectiveAcceptRadius,
		false);

	if (bDebugTutorialHomeReturn)
	{
		const UPathFollowingComponent* PathFollowing = AICon->GetPathFollowingComponent();
		UE_LOG(LogTemp, Warning, TEXT("TutorialHomeMoveRequest | Result=%d Goal=%s RawHome=%s GoalDist=%.1f RawDist=%.1f Accept=%.1f PFStatus=%d"),
			static_cast<int32>(Result),
			*TutorialHomeReturnGoalLocation.ToCompactString(),
			*TutorialHomeLocation.ToCompactString(),
			FVector::Dist2D(GetActorLocation(), TutorialHomeReturnGoalLocation),
			FVector::Dist2D(GetActorLocation(), TutorialHomeLocation),
			EffectiveAcceptRadius,
			PathFollowing ? static_cast<int32>(PathFollowing->GetStatus()) : -1);
	}

	if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		FinishTutorialHomeReturn();
		return true;
	}

	if (Result == EPathFollowingRequestResult::RequestSuccessful)
	{
		bTutorialHomeMoveRequested = true;
		return true;
	}

	return false;
}

void AJunTutorialNPC::FinishTutorialHomeReturn()
{
	if (bDebugTutorialHomeReturn)
	{
		const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
		UE_LOG(LogTemp, Warning, TEXT("TutorialHomeFinish | GoalDist=%.1f RawDist=%.1f Accept=%.1f DesiredBefore=(%.2f,%.2f) SpeedBefore=%.1f"),
			FVector::Dist2D(GetActorLocation(), TutorialHomeReturnGoalLocation),
			FVector::Dist2D(GetActorLocation(), TutorialHomeLocation),
			GetTutorialHomeReturnEffectiveAcceptRadius(),
			GetDesiredMoveForward(),
			GetDesiredMoveRight(),
			MovementComponent ? MovementComponent->Velocity.Size2D() : 0.f);
	}

	bTutorialReturningHome = false;
	bTutorialHomeMoveRequested = false;
	TutorialHomeReturnMoveDelayRemainTime = 0.f;
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	bRunLocomotionRequested = false;

	if (bTutorialTaskTransitionPending)
	{
		bTutorialTaskTransitionWaitingAtHome = true;
		TutorialTaskTransitionWaitRemainTime = FMath::Max(0.f, TutorialTaskTransitionWaitDuration);
	}
}

void AJunTutorialNPC::RefreshTutorialHomeReturnGoalLocation()
{
	TutorialHomeReturnGoalLocation = TutorialHomeLocation;

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return;
	}

	FNavLocation ProjectedLocation;
	const bool bProjected = NavigationSystem->ProjectPointToNavigation(
		TutorialHomeLocation,
		ProjectedLocation,
		FVector(200.f, 200.f, 500.f));

	if (bProjected)
	{
		TutorialHomeReturnGoalLocation = ProjectedLocation.Location;
	}
}

float AJunTutorialNPC::GetTutorialHomeReturnEffectiveAcceptRadius() const
{
	return FMath::Max(0.f, TutorialHomeReturnAcceptRadius);
}

void AJunTutorialNPC::UpdateTutorialHomeReturnMoveAxes()
{
	FVector ToHome = TutorialHomeReturnGoalLocation - GetActorLocation();
	ToHome.Z = 0.f;
	if (ToHome.IsNearlyZero())
	{
		SetDesiredMoveAxes(0.f, 0.f);
		CombatMoveInput = FVector2D::ZeroVector;
		return;
	}

	const FVector MoveDirection = ToHome.GetSafeNormal();
	const float ForwardAxis = FVector::DotProduct(GetActorForwardVector(), MoveDirection);
	const float RightAxis = FVector::DotProduct(GetActorRightVector(), MoveDirection);
	SetDesiredMoveAxes(ForwardAxis, RightAxis);
	CombatMoveInput = FVector2D(ForwardAxis, RightAxis);
}

bool AJunTutorialNPC::CanUpdateTutorialHomeReturn() const
{
	if (!bTutorialCombatEnabled ||
		CurrentState == EMonsterState::Dead ||
		bExecutionReady ||
		bBeingExecuted ||
		IsInHitReact() ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	return TutorialMode == EJunTutorialNPCMode::PassiveDummy ||
		TutorialMode == EJunTutorialNPCMode::AttackTraining ||
		TutorialMode == EJunTutorialNPCMode::ExecutionTraining;
}

void AJunTutorialNPC::OnDialogueRangeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AJunPlayer* Player = Cast<AJunPlayer>(OtherActor);
	if (!Player)
	{
		return;
	}

	NearbyPlayer = Player;
	bPlayerInDialogueRange = true;
	SetInteractionPromptVisible(CanStartDialogue(Player));
}

void AJunTutorialNPC::OnDialogueRangeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor != NearbyPlayer)
	{
		return;
	}

	NearbyPlayer = nullptr;
	bPlayerInDialogueRange = false;
	SetInteractionPromptVisible(false);
}

void AJunTutorialNPC::PlayIdleWaitMontage()
{
	if (!IdleWaitMontage || bDialogueActive)
	{
		return;
	}

	PlayAnimMontage(IdleWaitMontage);
}

void AJunTutorialNPC::SetInteractionPromptVisible(bool bVisible)
{
	if (TutorialMode != EJunTutorialNPCMode::DialogueOnly || bTutorialCombatEnabled)
	{
		bVisible = false;
	}

	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetHiddenInGame(!bVisible);
		InteractionPromptWidget->SetVisibility(bVisible, true);
	}
}

void AJunTutorialNPC::ApplyInteractionPromptSettings()
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	InteractionPromptWidget->SetWidgetSpace(InteractionPromptSpace);
	InteractionPromptWidget->SetDrawAtDesiredSize(false);
	InteractionPromptWidget->SetDrawSize(InteractionPromptDrawSize);
	InteractionPromptWidget->SetPivot(FVector2D(0.5f, 0.5f));
	InteractionPromptWidget->SetTickWhenOffscreen(false);
}

void AJunTutorialNPC::UpdateInteractionPromptBillboard()
{
	if (!InteractionPromptWidget ||
		InteractionPromptSpace != EWidgetSpace::World ||
		!bBillboardWorldInteractionPrompt ||
		!InteractionPromptWidget->IsVisible())
	{
		return;
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager)
	{
		return;
	}

	const FVector WidgetLocation = InteractionPromptWidget->GetComponentLocation();
	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const FVector ToCamera = CameraLocation - WidgetLocation;
	if (ToCamera.IsNearlyZero())
	{
		return;
	}

	InteractionPromptWidget->SetWorldRotation(ToCamera.Rotation());
}
