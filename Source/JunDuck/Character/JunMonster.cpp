#include "Character/JunMonster.h"
#include "AI/JunAIController.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Weapon/WeaponActor.h"

// Engine lifecycle / external API

namespace
{
	bool IsHeavyHitType(const EHitReactType HitType)
	{
		return HitType == EHitReactType::HeavyHit_A
			|| HitType == EHitReactType::HeavyHit_B
			|| HitType == EHitReactType::HeavyHit_C;
	}

	bool IsLargeHitType(const EHitReactType HitType)
	{
		return HitType == EHitReactType::LargeHit_Short
			|| HitType == EHitReactType::LargeHit_Long;
	}

	ECharacterHitReactDirection ResolveHitReactDirectionFromSwing(const AActor& CharacterActor, const FVector& SwingDirection)
	{
		const FVector SafeSwingDirection = SwingDirection.GetSafeNormal();
		if (SafeSwingDirection.IsNearlyZero())
		{
			return ECharacterHitReactDirection::Front_F;
		}

		const FVector LocalSwingDirection = CharacterActor.GetActorTransform().InverseTransformVectorNoScale(SafeSwingDirection);
		const float YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalSwingDirection.Y, LocalSwingDirection.X));

		if (YawDegrees >= -25.f && YawDegrees <= 25.f)
		{
			return ECharacterHitReactDirection::Front_F;
		}
		if (YawDegrees > 25.f && YawDegrees <= 70.f)
		{
			return ECharacterHitReactDirection::Front_FR;
		}
		if (YawDegrees > 70.f && YawDegrees <= 135.f)
		{
			return ECharacterHitReactDirection::Right_R;
		}
		if (YawDegrees < -25.f && YawDegrees >= -70.f)
		{
			return ECharacterHitReactDirection::Front_FL;
		}
		if (YawDegrees < -70.f && YawDegrees >= -135.f)
		{
			return ECharacterHitReactDirection::Left_L;
		}

		return ECharacterHitReactDirection::Back_B;
	}

	UAnimMontage* ResolveDirectionalHitReactMontage(
		const ECharacterHitReactDirection HitDirection,
		UAnimMontage* BackMontage,
		UAnimMontage* FrontMontage,
		UAnimMontage* FrontLeftMontage,
		UAnimMontage* FrontRightMontage,
		UAnimMontage* LeftMontage,
		UAnimMontage* RightMontage)
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
	}

	const TCHAR* GetMonsterStateDebugText(EMonsterState State)
	{
		switch (State)
		{
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

AJunMonster::AJunMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 캡슐 충돌 프리셋
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("EnemyCapsule"));

	// 메쉬 기본 위치 및 회전 조정
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));

	AIControllerClass = AJunAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 컨트롤러에 기록된 회전을 따라서 캐릭터가 같이 회전되는 옵션 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 몸체가 이동 방향에 맞춰서 회전하게끔함
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 350.f, 0.f);
}

void AJunMonster::BeginPlay()
{
	Super::BeginPlay();

	// 팀 설정
	SetTeamTag(JunGameplayTags::Team_Enemy);

	SpawnAndAttachWeapon();

	HomeLocation = GetActorLocation();

	if (bStartWithPatrol)
	{
		SetMonsterState(EMonsterState::Patrol);
	}
	else
	{
		SetMonsterState(EMonsterState::Idle);
	}
}

void AJunMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GEngine)
	{
		FString DebugText = FString::Printf(
			TEXT("Monster State: %s\nHp: %d / %d"),
			GetMonsterStateDebugText(CurrentState),
			Hp,
			MaxHp
		);
		const FString ExtraDebugText = GetMonsterDebugExtraText();
		if (!ExtraDebugText.IsEmpty())
		{
			DebugText += TEXT("\n");
			DebugText += ExtraDebugText;
		}

		GEngine->AddOnScreenDebugMessage(
			static_cast<uint64>(reinterpret_cast<UPTRINT>(this)),
			0.f,
			FColor::Yellow,
			DebugText
		);
	}

	if (bDebugFreezeMovement)
	{
		StopAIMovement();
		GetCharacterMovement()->StopMovementImmediately();
		SetDesiredMoveAxes(0.f, 0.f);
		CombatMoveInput = FVector2D::ZeroVector;
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = GetDesiredMaxWalkSpeed();
	bIsRunning = ShouldUseRunLocomotion();
	UpdateAttack(DeltaTime);

	// 몬스터는 "상위 상태 업데이트"와 "피격 리액션 타이머"를 매 프레임 병렬로 관리한다.
	StateTime += DeltaTime;
	Update_State(DeltaTime);
	UpdateHitReact(DeltaTime);
}

void AJunMonster::HandlePatrolMoveCompleted(bool bSuccess)
{
	if (CurrentState == EMonsterState::Return)
	{
		if (HasReachedReturnTarget())
		{
			bIsSearching = true;

			// 수색 시간 시작
			SetMonsterState(EMonsterState::Idle);
		}

		return;
	}

	if (CurrentState != EMonsterState::Patrol)
	{
		return;
	}

	// 플레이어 발견 우선
	TryFindTarget();

	if (CurrentTarget)
	{
		SetMonsterState(EMonsterState::Chase);
		return;
	}

	// 성공/실패 상관없이 다음 순찰점 갱신
	ChooseNextPatrolLocation();
	MoveToLocation(PatrolTargetLocation);
}

void AJunMonster::ReceiveHit(EHitReactType HitType, float DamageAmount, AActor* DamageCauser)
{
	// 기존 호출부 호환용 경로.
	// 방향 정보가 없으면 공격자 위치 기반의 대체 방향을 만들어 아래 오버로드로 넘긴다.
	const FVector FallbackSwingDirection = DamageCauser
		? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal()
		: GetActorForwardVector();

	ReceiveHit(HitType, DamageAmount, DamageCauser, FallbackSwingDirection);
}

void AJunMonster::ReceiveHit(EHitReactType HitType, float DamageAmount, AActor* DamageCauser, const FVector& SwingDirection)
{
	if (CurrentState == EMonsterState::Dead)
	{
		return;
	}

	// 데미지는 먼저 적용하고, 그 다음 "새 피격 리액션으로 갈아탈 수 있는지"를 판단한다.
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);

	// 데미지 적용 (기존 시스템 활용)
	OnDamaged(FMath::RoundToInt(DamageAmount), AttackerCharacter);

	// 사망 체크
	if (Is_Dead())
	{
		CurrentHitReact = EHitReactType::Dead;
		SetMonsterState(EMonsterState::Dead);
		return;
	}

	// 인터럽트 가능 여부
	if (!CanBeInterruptedBy(HitType))
	{
		return;
	}

	// 이미 HitReact 중일 때 우선순위 간단 처리
	if (IsInHitReact())
	{
		// Heavy react 중에는 Light/Large가 현재 모션을 덮지 못하게 막는다.
		if (IsHeavyHitType(CurrentHitReact) && HitType == EHitReactType::LightHit)
		{
			return;
		}

		if (IsHeavyHitType(CurrentHitReact) && IsLargeHitType(HitType))
		{
			return;
		}
	}

	if (!ShouldStartHitReact(HitType))
	{
		return;
	}

	// 먼저 공격자 위치로 F/L/R/B를 나누고,
	// 정면 계열일 때만 스윙 방향으로 F/FL/FR를 세분화한다.
	StartHitReact(HitType, DetermineHitReactDirection(DamageCauser, SwingDirection));
}

bool AJunMonster::HasCombatTarget()
{
	return bIsHasTarget;
}

bool AJunMonster::IsGuardOn() const
{
	return bIsGuardOn;
}

bool AJunMonster::IsInCombat()
{
	return CurrentState == EMonsterState::BattleStart || CurrentState == EMonsterState::Combat;
}

bool AJunMonster::IsAttacking()
{
	return bIsAttacking;
}

bool AJunMonster::IsRunning() const
{
	return bIsRunning;
}

bool AJunMonster::ShouldUseRunLocomotion() const
{
	const EMonsterMoveState MoveState = GetMoveState();
	return MoveState == EMonsterMoveState::Run;
}

EMonsterState AJunMonster::GetCurrentState() const
{
	return CurrentState;
}

EMonsterMoveState AJunMonster::GetMoveState() const
{
	if (bIsGuardOn)
	{
		return EMonsterMoveState::Guard;
	}

	if (bRunLocomotionRequested)
	{
		return EMonsterMoveState::Run;
	}

	return EMonsterMoveState::Walk;
}

FVector2D AJunMonster::GetCombatMoveInput() const
{
	return CombatMoveInput;
}

float AJunMonster::GetDesiredMoveForward() const
{
	return DesiredMoveForward;
}

float AJunMonster::GetDesiredMoveRight() const
{
	return DesiredMoveRight;
}

float AJunMonster::GetDesiredMaxWalkSpeed() const
{
	switch (GetMoveState())
	{
	case EMonsterMoveState::Guard:
		return GuardMoveSpeed;
	case EMonsterMoveState::Run:
		return RunMoveSpeed;
	case EMonsterMoveState::Walk:
	default:
		return WalkMoveSpeed;
	}
}

void AJunMonster::SetDesiredMoveAxes(float NewForward, float NewRight)
{
	DesiredMoveForward = FMath::Clamp(NewForward, -1.f, 1.f);
	DesiredMoveRight = FMath::Clamp(NewRight, -1.f, 1.f);
}

void AJunMonster::BeginAttackTraceWindow(EHitReactType HitReactType)
{
	if (!EquippedWeapon)
	{
		return;
	}

	EquippedWeapon->SetAttackHitReactType(HitReactType);
	EquippedWeapon->StartAttackTrace();
}

void AJunMonster::EndAttackTraceWindow()
{
	if (!EquippedWeapon)
	{
		return;
	}

	EquippedWeapon->EndAttackTrace();
}

void AJunMonster::BeginKickAttackTraceWindow(EHitReactType HitReactType)
{
	if (EquippedKickWeapon)
	{
		EquippedKickWeapon->SetAttackHitReactType(HitReactType);
		EquippedKickWeapon->StartAttackTrace();
	}

	if (EquippedKickWeaponRight)
	{
		EquippedKickWeaponRight->SetAttackHitReactType(HitReactType);
		EquippedKickWeaponRight->StartAttackTrace();
	}
}

void AJunMonster::EndKickAttackTraceWindow()
{
	if (EquippedKickWeapon)
	{
		EquippedKickWeapon->EndAttackTrace();
	}

	if (EquippedKickWeaponRight)
	{
		EquippedKickWeaponRight->EndAttackTrace();
	}
}

// Top-level state machine

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
}

void AJunMonster::EnterDeadState()
{
	bIsHasTarget = false;
	bRunLocomotionRequested = false;
	CombatMoveInput = FVector2D::ZeroVector;
	CancelCombatTurn();
	ResetCombatTurnState();
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

	UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!MonsterAnimInstance)
	{
		return false;
	}

	UAnimMontage* TurnMontage = ChooseCombatTurnMontage(YawDelta);
	if (!TurnMontage)
	{
		return false;
	}

	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);

	if (MonsterAnimInstance->Montage_Play(TurnMontage) <= 0.f)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
		return false;
	}

	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	CurrentCombatTurnMontage = TurnMontage;
	PendingStateAfterCombatTurn = NextState;
	bHasPendingStateAfterCombatTurn = true;
	bCombatTurnInProgress = true;
	return true;
}

bool AJunMonster::CanStartGenericTurnTowardsTarget() const
{
	if (!CurrentTarget || bCombatTurnInProgress)
	{
		return false;
	}

	if (bIsAttacking || IsInHitReact() || CurrentState == EMonsterState::Dead)
	{
		return false;
	}

	return true;
}

bool AJunMonster::CanStartCombatTurn() const
{
	if (!CurrentTarget || bCombatTurnInProgress)
	{
		return false;
	}

	if (CurrentState != EMonsterState::BattleStart &&
		CurrentState != EMonsterState::Combat)
	{
		return false;
	}

	if (bIsAttacking || IsInHitReact() || CurrentState == EMonsterState::Dead)
	{
		return false;
	}

	return GetVelocity().Size2D() <= CombatTurnMaxGroundSpeed;
}

bool AJunMonster::TryStartCombatTurnWithThreshold(float MinimumTurnAngle)
{
	if (!CanStartCombatTurn())
	{
		return false;
	}

	UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!MonsterAnimInstance)
	{
		return false;
	}

	const float YawDelta = GetCombatTargetYawDelta();
	if (FMath::Abs(YawDelta) < MinimumTurnAngle)
	{
		return false;
	}

	UAnimMontage* TurnMontage = ChooseCombatTurnMontage(YawDelta);
	if (!TurnMontage)
	{
		return false;
	}

	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);

	if (MonsterAnimInstance->Montage_Play(TurnMontage) <= 0.f)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
		return false;
	}

	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	CurrentCombatTurnMontage = TurnMontage;
	bCombatTurnInProgress = true;
	return true;
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

void AJunMonster::UpdateCombatFacing(float DeltaTime)
{
	if (!CurrentTarget)
	{
		return;
	}

	TryStartCombatTurn();

	if (IsCombatTurnPlaying())
	{
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

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = ToTarget.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		CombatFacingInterpSpeed
	);

	SetActorRotation(NewRotation);
}

// AI / target helpers

void AJunMonster::TryFindTarget()
{
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);

	if (!PlayerActor)
	{
		return;
	}

	AJunCharacter* TargetCharacter = Cast<AJunCharacter>(PlayerActor);

	if (!TargetCharacter)
	{
		return;
	}

	const bool bEnemy = IsEnemyTo(TargetCharacter);
	if (!bEnemy)
	{
		return;
	}

	const bool bDetect = CanDetectTarget(TargetCharacter);
	if (!bDetect)
	{
		return;
	}

	CurrentTarget = TargetCharacter;
}

bool AJunMonster::CanDetectTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(Target);
	if (TargetCharacter && TargetCharacter->Is_Dead())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.DetectRange);
}

bool AJunMonster::CanKeepTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(Target);
	if (TargetCharacter && TargetCharacter->Is_Dead())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.LoseTargetRange);
}

bool AJunMonster::CanRemainInCombat(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(Target);
	if (TargetCharacter && TargetCharacter->Is_Dead())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.CombatBreakRange);
}

bool AJunMonster::CanAttackTarget() const
{
	if (CurrentState != EMonsterState::Combat)
	{
		return false;
	}

	const FMonsterAttackSelection AttackSelection = ChooseNextAttackSelection();
	if (!CurrentTarget || !AttackSelection.Montage)
	{
		return false;
	}

	if (bIsAttacking || IsInHitReact() || Is_Dead() || IsCombatTurnPlaying())
	{
		return false;
	}

	const float DistSq = FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return DistSq <= FMath::Square(AttackSelection.AttackRange);
}

void AJunMonster::ChooseNextPatrolLocation()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	APawn* ControlledPawn = AICon->GetPawn();
	if (!ControlledPawn)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(ControlledPawn->GetWorld());
	if (!NavSystem)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	FNavLocation RandomLocation;
	const bool bFound = NavSystem->GetRandomReachablePointInRadius(
		HomeLocation,
		AIData.PatrolRadius,
		RandomLocation
	);

	if (!bFound)
	{
		PatrolTargetLocation = HomeLocation;
		return;
	}

	FNavLocation ProjectedLocation;
	const bool bProjected = NavSystem->ProjectPointToNavigation(
		RandomLocation.Location,
		ProjectedLocation,
		FVector(100.f, 100.f, 300.f)
	);

	PatrolTargetLocation = bProjected ? ProjectedLocation.Location : RandomLocation.Location;
}

void AJunMonster::MoveToLocation(const FVector& Dest, float AcceptanceRadius)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return;
	}

	FNavLocation ProjectedLocation;
	const bool bProjected = NavigationSystem->ProjectPointToNavigation(
		Dest,
		ProjectedLocation,
		FVector(200.f, 200.f, 500.f)
	);

	if (!bProjected)
	{
		return;
	}

	const float ResolvedAcceptanceRadius =
		AcceptanceRadius >= 0.f ? AcceptanceRadius : AIData.PatrolAcceptRadius;

	AICon->MoveToLocation(ProjectedLocation.Location, ResolvedAcceptanceRadius);
}

bool AJunMonster::TryResolveReachableLocationToward(const FVector& DesiredLocation, FVector& OutReachableLocation) const
{
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return false;
	}

	const FVector StartLocation = GetActorLocation();
	const FVector QueryExtent(100.f, 100.f, 300.f);
	constexpr int32 NumSamples = 12;

	for (int32 SampleIndex = NumSamples; SampleIndex >= 0; --SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(NumSamples);
		const FVector SampleLocation = FMath::Lerp(StartLocation, DesiredLocation, Alpha);

		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(SampleLocation, ProjectedLocation, QueryExtent))
		{
			OutReachableLocation = ProjectedLocation.Location;
			return true;
		}
	}

	return false;
}

float AJunMonster::GetEffectiveReturnReachedDistance() const
{
	const float CapsuleRadius = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.f;

	return ReturnAcceptRadius + CapsuleRadius + ReturnReachedTolerance;
}

bool AJunMonster::HasReachedReturnTarget() const
{
	return FVector::Dist2D(GetActorLocation(), ReturnTargetLocation) <= GetEffectiveReturnReachedDistance();
}

void AJunMonster::ResetCombatTurnState()
{
	bCombatTurnInProgress = false;
	bHasPendingStateAfterCombatTurn = false;
	CurrentCombatTurnMontage = nullptr;
	PendingStateAfterCombatTurn = EMonsterState::Idle;
}

void AJunMonster::MoveToTarget(AActor* Target)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon || !Target)
	{
		return;
	}

	AICon->MoveToActor(Target, AIData.ChaseMoveAcceptanceRadius);
}

void AJunMonster::StartChase(AActor* NewTarget)
{
	if (!NewTarget)
	{
		return;
	}

	// 이미 같은 타겟을 상위 전투 상태에서 추적/전투 중이면 상태를 되감지 않는다.
	if (CurrentTarget == NewTarget &&
		(CurrentState == EMonsterState::Chase ||
		 CurrentState == EMonsterState::BattleStart ||
		 CurrentState == EMonsterState::Combat))
	{
		return;
	}

	// 전투 중에 새 타겟 갱신이 와도, 전투 유지 가능 범위라면 현재 상위 상태를 유지한다.
	if (CurrentState == EMonsterState::BattleStart || CurrentState == EMonsterState::Combat)
	{
		CurrentTarget = NewTarget;
		bIsHasTarget = true;
		return;
	}

	CurrentTarget = NewTarget;
	bIsHasTarget = false;
	SetMonsterState(EMonsterState::Chase);
}

void AJunMonster::StopAIMovement()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	AICon->StopMovement();
}

void AJunMonster::ClearCurrentTarget()
{
	CurrentTarget = nullptr;
	bIsHasTarget = false;
}

bool AJunMonster::TryHandleDeadCurrentTarget(EMonsterState NextStateIfDead)
{
	const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(CurrentTarget);
	if (!TargetCharacter || !TargetCharacter->Is_Dead())
	{
		return false;
	}

	ClearCurrentTarget();
	SetMonsterState(NextStateIfDead);
	return true;
}

void AJunMonster::StopAllAttackTraces()
{
	EndAttackTraceWindow();
	EndKickAttackTraceWindow();

	if (EquippedScabbard)
	{
		EquippedScabbard->StopAttackTrace();
	}
}

void AJunMonster::ResetCurrentAttackRuntimeState()
{
	CurrentAttackMontage = nullptr;
	CurrentAttackSelection = FMonsterAttackSelection();
	bAttackFacingWindowActive = false;
	AttackFacingWindowInterpSpeed = 0.f;
}

void AJunMonster::BeginAttackFacingWindow(float InterpSpeed)
{
	bAttackFacingWindowActive = true;
	AttackFacingWindowInterpSpeed = FMath::Max(InterpSpeed, 0.f);
}

void AJunMonster::EndAttackFacingWindow()
{
	bAttackFacingWindowActive = false;
	AttackFacingWindowInterpSpeed = 0.f;
}

void AJunMonster::TryAttack()
{
	if (!CanAttackTarget())
	{
		return;
	}

	const FMonsterAttackSelection AttackSelection = ChooseNextAttackSelection();
	UAnimMontage* AttackMontageToPlay = AttackSelection.Montage.Get();
	bIsAttacking = true;
	bAttackInterruptedByHitReact = false;
	CurrentAttackSelection = AttackSelection;
	CurrentAttackMontage = AttackMontageToPlay;
	const float AttackPlayRate = FMath::Max(AttackSelection.PlayRate, KINDA_SMALL_NUMBER);
	AttackTime = AttackMontageToPlay ? (AttackMontageToPlay->GetPlayLength() / AttackPlayRate) : DefaultAttackDuration;
	AttackFacingRemainTime = AttackSelection.FacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (AttackMontageToPlay)
	{
		PlayAnimMontage(AttackMontageToPlay, AttackPlayRate);
	}
}

void AJunMonster::UpdateAttack(float DeltaTime)
{
	if (!bIsAttacking)
	{
		return;
	}

	if (IsInHitReact())
	{
		return;
	}

	if (IsCombatTurnPlaying())
	{
		return;
	}

	if (AttackFacingRemainTime > 0.f)
	{
		AttackFacingRemainTime = FMath::Max(0.f, AttackFacingRemainTime - DeltaTime);
	}

	const bool bCanFaceDuringAttack =
		CurrentAttackSelection.bFaceTargetDuringAttack &&
		(bAttackFacingWindowActive || AttackFacingRemainTime > 0.f || CurrentAttackSelection.FacingDuration < 0.f);

	if (bCanFaceDuringAttack && CurrentTarget)
	{
		FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator CurrentRotation = GetActorRotation();
			const FRotator TargetRotation = ToTarget.Rotation();
			const float FacingInterpSpeed = bAttackFacingWindowActive && AttackFacingWindowInterpSpeed > 0.f
				? AttackFacingWindowInterpSpeed
				: CurrentAttackSelection.FacingInterpSpeed;
			const FRotator NewRotation = FMath::RInterpTo(
				CurrentRotation,
				TargetRotation,
				DeltaTime,
				FacingInterpSpeed
			);

			SetActorRotation(NewRotation);
		}
	}

	OnAttackTick(DeltaTime);

	AttackTime -= DeltaTime;
	if (AttackTime <= 0.f)
	{
		FinishAttack();
	}
}

void AJunMonster::FinishAttack()
{
	if (!bIsAttacking)
	{
		return;
	}

	bIsAttacking = false;
	AttackTime = 0.f;
	AttackFacingRemainTime = 0.f;

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	if (CurrentAttackSelection.bTryTurnAfterAttack)
	{
		TryStartPostAttackTurn();
	}

	ResetCurrentAttackRuntimeState();
}

FMonsterAttackSelection AJunMonster::ChooseNextAttackSelection() const
{
	FMonsterAttackSelection Selection;
	Selection.Montage = AttackMontage;
	Selection.AttackRange = DefaultAttackRange;
	Selection.bFaceTargetDuringAttack = true;
	Selection.FacingDuration = -1.f;
	Selection.FacingInterpSpeed = AttackFacingInterpSpeed;
	Selection.PlayRate = 1.f;
	Selection.bTryTurnAfterAttack = false;
	Selection.PostAttackTurnStartAngle = CombatTurnStartAngle;
	return Selection;
}

bool AJunMonster::TryStartPostAttackTurn()
{
	return TryStartCombatTurnWithThreshold(CurrentAttackSelection.PostAttackTurnStartAngle);
}

void AJunMonster::OnAttackTick(float DeltaTime)
{
}

FString AJunMonster::GetMonsterDebugExtraText() const
{
	return FString();
}

UAnimMontage* AJunMonster::GetCurrentAttackMontage() const
{
	return CurrentAttackMontage.Get();
}

const FMonsterAttackSelection& AJunMonster::GetCurrentAttackSelection() const
{
	return CurrentAttackSelection;
}

void AJunMonster::OnCombatTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentCombatTurnMontage)
	{
		return;
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	}

	bCombatTurnInProgress = false;
	CurrentCombatTurnMontage = nullptr;

	if (bHasPendingStateAfterCombatTurn)
	{
		const EMonsterState NextState = PendingStateAfterCombatTurn;
		bHasPendingStateAfterCombatTurn = false;
		PendingStateAfterCombatTurn = EMonsterState::Idle;
		SetMonsterState(NextState);
	}
}

// Weapon

void AJunMonster::SpawnAndAttachWeapon()
{
	// 몬스터 무기는 BeginPlay에서 한 번 스폰해서 소켓에 붙이고,
	// 공격 trace window 때만 활성화한다.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

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
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster weapon."));
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
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster scabbard."));
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

	if (DefaultKickWeaponClass)
	{
		EquippedKickWeapon = World->SpawnActor<AWeaponActor>(
			DefaultKickWeaponClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedKickWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster kick weapon."));
		}
		else
		{
			EquippedKickWeapon->StopAttackTrace();
			EquippedKickWeapon->SetTraceSampleCount(KickWeaponTraceSampleCount);
			EquippedKickWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				KickWeaponSocketName
			);
		}
	}

	if (DefaultKickWeaponRightClass)
	{
		EquippedKickWeaponRight = World->SpawnActor<AWeaponActor>(
			DefaultKickWeaponRightClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedKickWeaponRight)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster right kick weapon."));
		}
		else
		{
			EquippedKickWeaponRight->StopAttackTrace();
			EquippedKickWeaponRight->SetTraceSampleCount(KickWeaponTraceSampleCount);
			EquippedKickWeaponRight->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				KickWeaponRightSocketName
			);
		}
	}
}

// Hit react

void AJunMonster::StartHitReact(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection)
{
	// 피격 리액션은 "상태 태그 잠금 + AI 이동 정지 + 방향별 몽타주 재생"으로 시작한다.
	// 이 동안 일반 상태머신은 멈추고, Duration이 끝나면 다시 원래 AI 상태 갱신으로 돌아간다.
	CurrentHitReact = NewHitReact;
	CurrentHitReactDirection = NewHitDirection;
	HitReactTime = 0.f;
	CurrentHitReactDuration = GetHitReactDuration(NewHitReact);
	CurrentHitReactControlLockRemainTime = GetHitReactControlLockDuration(NewHitReact);

	if (bIsAttacking)
	{
		bAttackInterruptedByHitReact = true;
		bIsAttacking = false;
		AttackTime = 0.f;
		AttackFacingRemainTime = 0.f;
		ResetCurrentAttackRuntimeState();
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
	AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);

	CancelCombatTurn();
	StopAIMovement();
	StopAllAttackTraces();
	OnHitReactStarted(NewHitReact, NewHitDirection);

	if (UAnimMontage* HitReactMontage = GetHitReactMontage(NewHitReact, NewHitDirection))
	{
		if (IsHeavyHitType(NewHitReact))
		{
			CurrentHitReactDuration = HitReactMontage->GetPlayLength();
		}

		PlayAnimMontage(HitReactMontage);
	}
}

void AJunMonster::UpdateHitReact(float DeltaTime)
{
	if (!IsInHitReact())
	{
		return;
	}

	// 현재는 몽타주 종료가 아니라 Duration 기준으로 HitReact 상태를 끝낸다.
	HitReactTime += DeltaTime;

	if (CurrentHitReactControlLockRemainTime > 0.f)
	{
		CurrentHitReactControlLockRemainTime = FMath::Max(0.f, CurrentHitReactControlLockRemainTime - DeltaTime);
		if (CurrentHitReactControlLockRemainTime <= 0.f)
		{
			ReleaseHitReactControlLock();
		}
	}

	if (HitReactTime >= CurrentHitReactDuration)
	{
		EndHitReact();
	}
}

void AJunMonster::EndHitReact()
{
	// HitReact 종료는 태그 해제만 담당하고,
	// 이후 AI 상태 갱신은 CanUpdateBehavior()가 다시 허용한다.
	const EHitReactType EndedHitReact = CurrentHitReact;
	CurrentHitReact = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	HitReactTime = 0.f;
	CurrentHitReactDuration = 0.f;
	CurrentHitReactControlLockRemainTime = 0.f;

	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	ReleaseHitReactControlLock();
	OnHitReactEnded(EndedHitReact);
}

void AJunMonster::ReleaseHitReactControlLock()
{
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
}

void AJunMonster::OnHitReactStarted(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection)
{
}

void AJunMonster::OnHitReactEnded(EHitReactType EndedHitReact)
{
}

bool AJunMonster::IsInHitReact() const
{
	return CurrentHitReact != EHitReactType::None;
}

bool AJunMonster::CanBeInterruptedBy(EHitReactType IncomingHitReact) const
{
	if (CurrentState == EMonsterState::Dead)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_SuperArmor))
	{
		return IsHeavyHitType(IncomingHitReact) || IncomingHitReact == EHitReactType::LargeHit_Long;
	}

	return true;
}

bool AJunMonster::ShouldStartHitReact(EHitReactType IncomingHitReact) const
{
	return IncomingHitReact != EHitReactType::None;
}

float AJunMonster::GetHitReactDuration(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
		return LightHitDuration;
	case EHitReactType::HeavyHit_A:
	case EHitReactType::HeavyHit_B:
	case EHitReactType::HeavyHit_C:
		return HeavyHitDuration;
	case EHitReactType::LargeHit_Short:
		return LargeHitDuration;
	case EHitReactType::LargeHit_Long:
		return LargeHitLongDuration;
	default:
		return 0.f;
	}
}

float AJunMonster::GetHitReactControlLockDuration(EHitReactType HitType) const
{
	return GetHitReactDuration(HitType);
}

ECharacterHitReactDirection AJunMonster::DetermineHitReactDirection(const AActor* DamageCauser, const FVector& SwingDirection) const
{
	if (!DamageCauser)
	{
		return ResolveHitReactDirectionFromSwing(*this, SwingDirection);
	}

	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(DamageCauser->GetActorLocation() - GetActorLocation());
	ToAttackerLocal.Z = 0.f;

	if (ToAttackerLocal.IsNearlyZero())
	{
		return ResolveHitReactDirectionFromSwing(*this, SwingDirection);
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

	const ECharacterHitReactDirection SwingResolvedDirection = ResolveHitReactDirectionFromSwing(*this, SwingDirection);
	if (SwingResolvedDirection == ECharacterHitReactDirection::Front_FL ||
		SwingResolvedDirection == ECharacterHitReactDirection::Front_FR)
	{
		return SwingResolvedDirection;
	}

	return ECharacterHitReactDirection::Front_F;
}

UAnimMontage* AJunMonster::GetHitReactMontage(EHitReactType HitType, ECharacterHitReactDirection HitDirection) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
		return ResolveDirectionalHitReactMontage(
			HitDirection,
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
			return ResolveDirectionalHitReactMontage(
				HitDirection,
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
			return ResolveDirectionalHitReactMontage(
				HitDirection,
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
			return ResolveDirectionalHitReactMontage(
				HitDirection,
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
		return ResolveDirectionalHitReactMontage(
			HitDirection,
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

bool AJunMonster::CanUpdateBehavior() const
{
	if (CurrentState == EMonsterState::Dead)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	return true;
}
