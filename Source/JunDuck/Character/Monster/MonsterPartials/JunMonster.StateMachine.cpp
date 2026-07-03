#include "Character/Monster/JunMonster.h"

#include "AI/JunAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

namespace
{
	const TCHAR* GetMonsterStateDebugText(EMonsterState State)
	{
		switch (State)
		{
		case EMonsterState::CutsceneWait:
			return TEXT("CutsceneWait");
		case EMonsterState::Idle:
			return TEXT("Idle");
		case EMonsterState::Patrol:
			return TEXT("Patrol");
		case EMonsterState::Chase:
			return TEXT("Chase");
		case EMonsterState::Return:
			return TEXT("Return");
		case EMonsterState::BattleStart:
			return TEXT("BattleStart");
		case EMonsterState::Combat:
			return TEXT("Combat");
		case EMonsterState::Dead:
			return TEXT("Dead");
		default:
			return TEXT("Unknown");
		}
	}
}

void AJunMonster::SetMonsterState(EMonsterState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Monster State Change: %d -> %d"),
		(int32)CurrentState, (int32)NewState);

	CurrentState = NewState;
	StateTime = 0.f;

	switch (CurrentState)
	{
	case EMonsterState::CutsceneWait:
		EnterCutsceneWaitState();
		break;
	case EMonsterState::Idle:
		EnterIdleState();
		break;
	case EMonsterState::Patrol:
		EnterPatrolState();
		break;
	case EMonsterState::Chase:
		EnterChaseState();
		break;
	case EMonsterState::BattleStart:
		EnterBattleStartState();
		break;
	case EMonsterState::Combat:
		EnterCombatState();
		break;
	case EMonsterState::Dead:
		EnterDeadState();
		break;
	case EMonsterState::Return:
		EnterReturnState();
		break;
	default:
		break;
	}
}

void AJunMonster::EnterPlayerDeathWait()
{
	StopAIMovement();
	StopAllAttackTraces();
	CancelCombatTurn();
	ResetCombatTurnState();
	ResetCurrentAttackRuntimeState();
	EndHitReact();
	bIsAttacking = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	if (CurrentState != EMonsterState::Combat)
	{
		SetMonsterState(EMonsterState::Combat);
	}
}

void AJunMonster::ResumeAfterPlayerFakeDeath()
{
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	if (CurrentState != EMonsterState::Combat)
	{
		SetMonsterState(EMonsterState::Combat);
	}
}

void AJunMonster::ResetAfterPlayerRealDeath()
{
	StopAIMovement();
	StopAllAttackTraces();
	CancelCombatTurn();
	ResetCombatTurnState();
	ResetCurrentAttackRuntimeState();
	ResetExecutionRuntimeState();
	StopCombatBGM();
	EndHitReact();
	RemoveGameplayTag(JunGameplayTags::State_Condition_Dead);
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	bIsAttacking = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	ClearCurrentTarget();
	Hp = MaxHp;
	CurrentLifeCount = FMath::Max(1, MaxLifeCount);
	CurrentPosture = 0.f;
	SetActorLocationAndRotation(HomeLocation, HomeRotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	SetMonsterState(EMonsterState::CutsceneWait);
}

void AJunMonster::EnterCutsceneWaitState()
{
	bIsHasTarget = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	bCutsceneTriggered = false;
	bCutsceneWaitEquipMontageActive = false;
	AttachWeaponToSheathedSocket();
	SetWeaponVisible(false);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!MonsterAnimInstance || !CutsceneWaitMontage)
	{
		return;
	}

	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCutsceneWaitMontageEnded);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCutsceneWaitMontageEnded);
	PlayAnimMontage(CutsceneWaitMontage);
	if (!CutsceneWaitIdleSectionName.IsNone() && CutsceneWaitMontage->IsValidSectionName(CutsceneWaitIdleSectionName))
	{
		MonsterAnimInstance->Montage_JumpToSection(CutsceneWaitIdleSectionName, CutsceneWaitMontage);
	}
}

void AJunMonster::EnterIdleState()
{
	bIsHasTarget = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
}

void AJunMonster::EnterPatrolState()
{
	bIsHasTarget = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	bHasPendingStateAfterCombatTurn = false;
	PendingStateAfterCombatTurn = EMonsterState::Idle;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	ChooseNextPatrolLocation();
	MoveToLocation(PatrolTargetLocation);
}

void AJunMonster::EnterChaseState()
{
	bIsHasTarget = false;
	CombatMoveInput = FVector2D::ZeroVector;
	bHasPendingStateAfterCombatTurn = false;
	PendingStateAfterCombatTurn = EMonsterState::Idle;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bRunLocomotionRequested = false;

	if (CurrentTarget)
	{
		MoveToTarget(CurrentTarget);
	}
}

void AJunMonster::EnterBattleStartState()
{
	bIsHasTarget = (CurrentTarget != nullptr);
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	ResetCombatTurnState();
	GetCharacterMovement()->bOrientRotationToMovement = false;

	const FVector Velocity2D = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
	if (!Velocity2D.IsNearlyZero())
	{
		const FVector Forward = GetActorForwardVector();
		const FVector Right = GetActorRightVector();
		BattleStartInitialMoveForward = FVector::DotProduct(Velocity2D.GetSafeNormal(), Forward);
		BattleStartInitialMoveRight = FVector::DotProduct(Velocity2D.GetSafeNormal(), Right);
	}
	else
	{
		BattleStartInitialMoveForward = DesiredMoveForward;
		BattleStartInitialMoveRight = DesiredMoveRight;
	}

	BattleStartMoveBlendRemainTime = BattleStartMoveBlendDuration;
	StopAIMovement();
}

void AJunMonster::EnterReturnState()
{
	bIsHasTarget = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	ResetCombatTurnState();
	GetCharacterMovement()->bOrientRotationToMovement = true;
	if (!TryResolveReachableLocationToward(LastKnownTargetLocation, ReturnTargetLocation))
	{
		ReturnTargetLocation = HomeLocation;
	}

	MoveToLocation(ReturnTargetLocation, ReturnAcceptRadius);
}

void AJunMonster::EnterCombatState()
{
	bIsHasTarget = (CurrentTarget != nullptr);
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	ResetCombatTurnState();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	StopAIMovement();
	StartCombatBGM();
}

void AJunMonster::EnterDeadState()
{
	bIsHasTarget = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	CancelCombatTurn();
	ResetCombatTurnState();
	StopMonsterCodeMove(true);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	AddGameplayTag(JunGameplayTags::State_Condition_Dead);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);
	StopAllAttackTraces();

	// 필요하면 죽음 몽타주 / collision 조정 / AI 비활성화
}

void AJunMonster::Update_State(float DeltaTime)
{
	// ControlLocked/Dead 같은 상위 조건이 걸리면 일반 AI 상태 갱신은 멈춘다.
	if (Is_Dead() && CurrentState != EMonsterState::Dead)
	{
		SetMonsterState(EMonsterState::Dead);
		return;
	}

	if (!CanUpdateBehavior())
	{
		return;
	}

	switch (CurrentState)
	{
	case EMonsterState::CutsceneWait:
		UpdateCutsceneWait(DeltaTime);
		break;
	case EMonsterState::Idle:
		UpdateIdle(DeltaTime);
		break;
	case EMonsterState::Patrol:
		UpdatePatrol(DeltaTime);
		break;
	case EMonsterState::Chase:
		UpdateChase(DeltaTime);
		break;
	case EMonsterState::BattleStart:
		UpdateBattleStart(DeltaTime);
		break;
	case EMonsterState::Combat:
		UpdateCombat(DeltaTime);
		break;
	case EMonsterState::Return:
		UpdateReturn(DeltaTime);
		break;
	case EMonsterState::Dead:
	default:
		break;
	}
}

void AJunMonster::UpdateCutsceneWait(float DeltaTime)
{
	if (bCutsceneTriggered)
	{
		return;
	}

	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);
	AJunCharacter* TargetCharacter = Cast<AJunCharacter>(PlayerActor);
	if (!TargetCharacter || TargetCharacter->Is_Dead() || !IsEnemyTo(TargetCharacter))
	{
		return;
	}

	const float DistSq = FVector::DistSquared2D(GetActorLocation(), TargetCharacter->GetActorLocation());
	if (DistSq > FMath::Square(CutsceneTriggerRange))
	{
		return;
	}

	bCutsceneTriggered = true;
	CurrentTarget = TargetCharacter;
	bIsHasTarget = true;
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (PlayCutsceneWaitEquipMontage(CutsceneWaitEquipBlendInTime))
	{
		return;
	}

	AttachWeaponToHandSocket();
	SetMonsterState(EMonsterState::BattleStart);
}

bool AJunMonster::PlayCutsceneWaitEquipMontage(float BlendInTime)
{
	UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!MonsterAnimInstance || !CutsceneWaitEquipMontage)
	{
		return false;
	}

	if (CutsceneWaitMontage)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCutsceneWaitMontageEnded);
	}

	bCutsceneWaitEquipMontageActive = true;
	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCutsceneWaitEquipMontageEnded);
	const float MontagePlayResult = MonsterAnimInstance->Montage_PlayWithBlendIn(
		CutsceneWaitEquipMontage,
		FAlphaBlendArgs(FMath::Max(0.f, BlendInTime)),
		1.f,
		EMontagePlayReturnType::MontageLength,
		0.f,
		true
	);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCutsceneWaitEquipMontageEnded);

	if (MontagePlayResult <= 0.f)
	{
		bCutsceneWaitEquipMontageActive = false;
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCutsceneWaitEquipMontageEnded);
		return false;
	}

	return true;
}

void AJunMonster::UpdateIdle(float DeltaTime)
{
	TryFindTarget();

	if (CurrentTarget)
	{
		bIsSearching = false;

		if (TryStartTurnTowardsTargetThenState(EMonsterState::Chase))
		{
			return;
		}

		SetMonsterState(EMonsterState::Chase);
		return;
	}

	if (bIsSearching)
	{
		if (StateTime >= SearchWaitTime)
		{
			bIsSearching = false;
			SetMonsterState(EMonsterState::Patrol);
		}
	}
	else
	{
		if (bStartWithPatrol && StateTime >= IdleToPatrolDelay)
		{
			SetMonsterState(EMonsterState::Patrol);
		}
	}
}

void AJunMonster::UpdatePatrol(float DeltaTime)
{
	TryFindTarget();

	if (CurrentTarget)
	{
		if (TryStartTurnTowardsTargetThenState(EMonsterState::Chase))
		{
			return;
		}

		SetMonsterState(EMonsterState::Chase);
		return;
	}
}

void AJunMonster::UpdateChase(float DeltaTime)
{
	if (!CurrentTarget)
	{
		SetMonsterState(EMonsterState::Patrol);
		return;
	}

	if (TryHandleDeadCurrentTarget(EMonsterState::Idle))
	{
		return;
	}

	// 유지 범위 체크 (DetectRange 아님)
	if (!CanKeepTarget(CurrentTarget))
	{
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		ClearCurrentTarget();
		SetMonsterState(EMonsterState::Return);
		return;
	}

	const float DistToTarget = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

	// Chase는 BattleStart 범위까지 확실히 붙는 역할만 담당한다.
	// 전투 시작 연출이나 전투 이동은 여기서 하지 않고, BattleStart / Combat으로 넘긴다.
	if (DistToTarget <= AIData.BattleStartRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter BattleStart Range"));
		GetCharacterMovement()->bOrientRotationToMovement = false;
		StopAIMovement();
		SetMonsterState(EMonsterState::BattleStart);
		return;
	}

	// 추적 유지 (너무 자주 호출 방지용 조건 추가)
	const float MoveThreshold = 50.f;

	if (FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation()) > MoveThreshold)
	{
		MoveToTarget(CurrentTarget);
	}
}

void AJunMonster::UpdateBattleStart(float DeltaTime)
{
	if (!CurrentTarget)
	{
		SetMonsterState(EMonsterState::Patrol);
		return;
	}

	if (TryHandleDeadCurrentTarget(EMonsterState::Idle))
	{
		return;
	}

	if (!CanRemainInCombat(CurrentTarget))
	{
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		ClearCurrentTarget();
		SetMonsterState(EMonsterState::Return);
		return;
	}

	// BattleStart는 전투 시작 연출/준비용 짧은 중간 상태다.
	// 실제 공격/전투 이동은 아직 하지 않고, 필요한 경우 Turn만 허용한다.
	UpdateBattleStartMovementBlend(DeltaTime);
	TryStartCombatTurn();

	if (IsCombatTurnPlaying())
	{
		return;
	}

	if (StateTime >= BattleStartDuration)
	{
		SetMonsterState(EMonsterState::Combat);
	}
}

void AJunMonster::UpdateBattleStartMovementBlend(float DeltaTime)
{
	if (BattleStartMoveBlendRemainTime <= 0.f)
	{
		SetDesiredMoveAxes(0.f, 0.f);
		return;
	}

	BattleStartMoveBlendRemainTime = FMath::Max(0.f, BattleStartMoveBlendRemainTime - DeltaTime);

	const float BlendAlpha = BattleStartMoveBlendDuration > 0.f
		? (BattleStartMoveBlendRemainTime / BattleStartMoveBlendDuration)
		: 0.f;

	SetDesiredMoveAxes(
		BattleStartInitialMoveForward * BlendAlpha,
		BattleStartInitialMoveRight * BlendAlpha
	);
}

void AJunMonster::UpdateReturn(float DeltaTime)
{
	// 이동 중에도 타겟 발견 가능
	TryFindTarget();

	if (CurrentTarget)
	{
		SetMonsterState(EMonsterState::Chase);
		return;
	}

	if (HasReachedReturnTarget())
	{
		bIsSearching = true;
		SetMonsterState(EMonsterState::Idle);
		return;
	}

	MoveToLocation(ReturnTargetLocation, ReturnAcceptRadius);
}

void AJunMonster::UpdateCombat(float DeltaTime)
{
	if (!CurrentTarget)
	{
		SetMonsterState(EMonsterState::Patrol);
		return;
	}

	if (TryHandleDeadCurrentTarget(EMonsterState::Idle))
	{
		CombatMoveInput = FVector2D::ZeroVector;
		return;
	}

	if (!CanRemainInCombat(CurrentTarget))
	{
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		ClearCurrentTarget();
		CombatMoveInput = FVector2D::ZeroVector;
		SetMonsterState(EMonsterState::Return);
		return;
	}

	// Combat는 타겟 유지, 전투용 회전, 전투용 이동만 담당한다.
	// 공격 여부 결정은 TryAttack 같은 별도 단계로 분리해서 붙이기 쉽게 둔다.
	UpdateCombatFacing(DeltaTime);

	if (CanAttackTarget())
	{
		TryAttack();
	}
}

// Combat helpers

void AJunMonster::TryStartCombatTurn()
{
	TryStartCombatTurnWithThreshold(CombatTurnStartAngle);
}

void AJunMonster::UpdateCombatTurn(float DeltaTime)
{
	if (!IsCombatTurnPlaying())
	{
		return;
	}

	const bool bPendingMovementState =
		bHasPendingStateAfterCombatTurn &&
		(PendingStateAfterCombatTurn == EMonsterState::Patrol ||
			PendingStateAfterCombatTurn == EMonsterState::Chase ||
			PendingStateAfterCombatTurn == EMonsterState::Return);

	if (bEarlyBlendOutCombatTurnOnTargetYaw &&
		bPendingMovementState &&
		FMath::Abs(GetCombatTargetYawDelta()) <= CombatTurnEarlyBlendOutYawTolerance)
	{
		FinishCombatTurnEarly(CombatTurnEarlyBlendOutTime);
	}
}

bool AJunMonster::TryStartTurnTowardsTargetThenState(EMonsterState NextState)
{
	if (!CanStartGenericTurnTowardsTarget())
	{
		return false;
	}

	const float YawDelta = GetCombatTargetYawDelta();
	if (FMath::Abs(YawDelta) < CombatTurnStartAngle)
	{
		return false;
	}

	return StartCombatTurnMontage(YawDelta, true, NextState);
}

bool AJunMonster::CanStartGenericTurnTowardsTarget() const
{
	return CanStartTurnInternal(false);
}

bool AJunMonster::CanStartCombatTurn() const
{
	return CanStartTurnInternal(true);
}

bool AJunMonster::TryStartCombatTurnWithThreshold(float MinimumTurnAngle)
{
	if (!CanStartCombatTurn())
	{
		return false;
	}

	const float YawDelta = GetCombatTargetYawDelta();
	if (FMath::Abs(YawDelta) < MinimumTurnAngle)
	{
		return false;
	}

	return StartCombatTurnMontage(YawDelta, false, EMonsterState::Idle);
}

bool AJunMonster::IsCombatTurnPlaying() const
{
	return bCombatTurnInProgress && CurrentCombatTurnMontage != nullptr;
}

float AJunMonster::GetCombatTargetYawDelta() const
{
	if (!CurrentTarget)
	{
		return 0.f;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return 0.f;
	}

	const FRotator TargetRotation = ToTarget.Rotation();
	return FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetRotation.Yaw);
}

UAnimMontage* AJunMonster::ChooseCombatTurnMontage(float YawDelta) const
{
	const float AbsYawDelta = FMath::Abs(YawDelta);
	if (AbsYawDelta < CombatTurnStartAngle)
	{
		return nullptr;
	}

	const bool bUse180 = AbsYawDelta >= CombatTurn180Threshold;
	const bool bTurnRight = YawDelta > 0.f;

	if (bUse180)
	{
		return bTurnRight ? TurnRight180Montage.Get() : TurnLeft180Montage.Get();
	}

	return bTurnRight ? TurnRight90Montage.Get() : TurnLeft90Montage.Get();
}

void AJunMonster::CancelCombatTurn(float BlendOutTime)
{
	if (!IsCombatTurnPlaying())
	{
		return;
	}

	UAnimMontage* PlayingTurnMontage = CurrentCombatTurnMontage.Get();
	bCombatTurnInProgress = false;
	bHasPendingStateAfterCombatTurn = false;
	CurrentCombatTurnMontage = nullptr;
	PendingStateAfterCombatTurn = EMonsterState::Idle;

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);

		if (PlayingTurnMontage)
		{
			MonsterAnimInstance->Montage_Stop(BlendOutTime, PlayingTurnMontage);
		}
	}
}

void AJunMonster::FinishCombatTurnEarly(float BlendOutTime)
{
	if (!IsCombatTurnPlaying())
	{
		return;
	}

	UAnimMontage* PlayingTurnMontage = CurrentCombatTurnMontage.Get();
	LastCombatTurnEndYaw = GetActorRotation().Yaw;

	if (bDebugCombatTurnYaw)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][EarlyBlendOut] Montage=%s Start=%.2f AfterPlay=%.2f End=%.2f TotalDelta=%.2f TargetDelta=%.2f"),
			*GetNameSafe(PlayingTurnMontage),
			LastCombatTurnStartYaw,
			LastCombatTurnPostPlayYaw,
			LastCombatTurnEndYaw,
			FMath::FindDeltaAngleDegrees(LastCombatTurnStartYaw, LastCombatTurnEndYaw),
			GetCombatTargetYawDelta());
	}

	bCombatTurnInProgress = false;
	CurrentCombatTurnMontage = nullptr;

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
		if (PlayingTurnMontage)
		{
			MonsterAnimInstance->Montage_Stop(FMath::Max(0.f, BlendOutTime), PlayingTurnMontage);
		}
	}

	if (bHasPendingStateAfterCombatTurn)
	{
		const EMonsterState NextState = PendingStateAfterCombatTurn;
		bHasPendingStateAfterCombatTurn = false;
		PendingStateAfterCombatTurn = EMonsterState::Idle;
		SetMonsterState(NextState);
	}
}

void AJunMonster::UpdateCombatFacing(float DeltaTime)
{
	if (!CurrentTarget)
	{
		return;
	}

	TryStartCombatTurn();

	if (IsCombatTurnPlaying())
	{
		EndAttackFacingWindow();
		AttackFacingRemainTime = 0.f;
		return;
	}

	const bool bHasDesiredMoveInput =
		!FMath::IsNearlyZero(DesiredMoveForward) ||
		!FMath::IsNearlyZero(DesiredMoveRight);
	const bool bHasMovementVelocity = GetVelocity().Size2D() > 3.f;

	if (!bHasDesiredMoveInput && !bHasMovementVelocity)
	{
		return;
	}

	FRotator TargetRotation;
	if (!TryGetCombatFacingTargetRotation(TargetRotation))
	{
		return;
	}

	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		TargetRotation,
		DeltaTime,
		CombatFacingInterpSpeed
	);

	SetActorRotation(NewRotation);
}

// AI / target helpers

