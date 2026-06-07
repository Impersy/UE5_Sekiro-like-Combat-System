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

AJunTutorialNPC::AJunTutorialNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AJunAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bStartWithPatrol = false;
	bStartWithCutsceneWait = false;

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
	UpdateTutorialHomeReturn();
}

bool AJunTutorialNPC::CanUpdateBehavior() const
{
	return bTutorialCombatEnabled && Super::CanUpdateBehavior();
}

void AJunTutorialNPC::UpdateCombat(float DeltaTime)
{
	if (TutorialMode == EJunTutorialNPCMode::PassiveDummy)
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
	if (!bTutorialCombatEnabled)
	{
		return;
	}

	const bool bPreventDeath = !IsExecutionTrainingEnabled();
	const int32 SafeDamage = bPreventDeath
		? FMath::Min(Damage, FMath::Max(0, Hp - 1))
		: Damage;

	Super::OnDamaged(SafeDamage, Attacker);

	if (bPreventDeath)
	{
		Hp = FMath::Max(1, Hp);
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
		CurrentPosture = 0.f;
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

	const bool bHandled = Super::NotifyMikiriCounteredBy(CounterPlayer);
	if (!IsExecutionTrainingEnabled())
	{
		CurrentPosture = 0.f;
		bExecutionReady = false;
		ExecutionReadyRemainTime = 0.f;
	}
	return bHandled;
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
	bDisablePostureGain = !IsExecutionTrainingEnabled();
	if (!IsExecutionTrainingEnabled())
	{
		CurrentPosture = 0.f;
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

	if (PlayCutsceneWaitEquipMontage(CutsceneWaitEquipBlendInTime))
	{
		return;
	}

	AttachWeaponToHandSocket();
	SetMonsterState(EMonsterState::BattleStart);
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

	if (!CanUpdateTutorialHomeReturn())
	{
		if (bTutorialReturningHome)
		{
			FinishTutorialHomeReturn();
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
	TutorialHomeReturnMoveDelayRemainTime = FMath::Max(0.f, TutorialHomeReturnMoveStartDelay);
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
