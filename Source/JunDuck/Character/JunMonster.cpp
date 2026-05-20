#include "Character/JunMonster.h"
#include "AI/JunAIController.h"
#include "AlphaBlend.h"
#include "Animation/AnimMontage.h"
#include "Character/JunPlayer.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Player/JunPlayerController.h"
#include "System/JunBGMManager.h"
#include "Weapon/ArrowProjectile.h"
#include "Weapon/WeaponActor.h"

// Engine lifecycle / external API

namespace
{
	bool IsInvalidCombatTarget(const AActor* Target)
	{
		if (!Target)
		{
			return true;
		}

		if (const AJunCharacter* TargetCharacter = Cast<AJunCharacter>(Target))
		{
			if (TargetCharacter->Is_Dead())
			{
				return true;
			}
		}

		if (const AJunPlayer* TargetPlayer = Cast<AJunPlayer>(Target))
		{
			if (TargetPlayer->IsInDeathSequence())
			{
				return true;
			}
		}

		return false;
	}

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

	bool DoesMontageUseRootMotion(const UAnimMontage* Montage)
	{
		return Montage && Montage->HasRootMotion();
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

AJunMonster::AJunMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 캡슐 충돌 프리셋
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("EnemyCapsule"));

	// 메쉬 기본 위치 및 회전 조정
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));

	CombatBGMComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CombatBGMComponent"));
	CombatBGMComponent->SetupAttachment(RootComponent);
	CombatBGMComponent->bAutoActivate = false;

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

	CurrentLifeCount = FMath::Max(1, MaxLifeCount);
	CurrentPosture = 0.f;
	bExecutionReady = false;
	bBeingExecuted = false;
	bExecutionResultApplied = false;
	ExecutionReadyRemainTime = 0.f;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;

	// 팀 설정
	SetTeamTag(JunGameplayTags::Team_Enemy);

	SpawnAndAttachWeapon();

	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		DefaultMonsterBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
		DefaultMonsterGroundFriction = MovementComponent->GroundFriction;
		DefaultMonsterBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;
	}

	if (bStartWithCutsceneWait)
	{
		SetMonsterState(EMonsterState::CutsceneWait);
	}
	else if (bStartWithPatrol)
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
	UpdateHitReactKnockbackBraking(DeltaTime);
	UpdateHitReactFacing(DeltaTime);

	if (bExecutionReady && !bBeingExecuted)
	{
		ExecutionReadyRemainTime = FMath::Max(0.f, ExecutionReadyRemainTime - DeltaTime);
		if (ExecutionReadyRemainTime <= 0.f)
		{
			EndExecutionReady();
		}
	}

	if (CurrentState != EMonsterState::Dead && bBeingExecuted && bAttackFacingWindowActive)
	{
		UpdateExecutionFacing(DeltaTime);
	}

	UpdateAttack(DeltaTime);
	UpdatePostureRecovery(DeltaTime);

	// 몬스터는 "상위 상태 업데이트"와 "피격 리액션 타이머"를 매 프레임 병렬로 관리한다.
	StateTime += DeltaTime;
	Update_State(DeltaTime);
	UpdateHitReact(DeltaTime);
}

void AJunMonster::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	Super::HandleGameplayEventNotify(EventTag);

	if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_StopCombatBGM))
	{
		StopCombatBGM();
		return;
	}

	if (!EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_Clear))
	{
		return;
	}

	if (BossClearSound)
	{
		UGameplayStatics::PlaySound2D(this, BossClearSound, BossClearSoundVolume);
	}

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		JunPlayerController->ShowBossClearUI();
	}
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
	ReceiveHit(HitType, DamageAmount, DamageCauser, SwingDirection, FJunAttackDefenseKnockbackData());
}

void AJunMonster::ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	if (CurrentState == EMonsterState::Dead || bBeingExecuted)
	{
		return;
	}

	if (TryHandleIncomingHitBeforeDamage(HitType, DamageAmount, DamageCauser, SwingDirection, DefenseKnockbackData))
	{
		return;
	}

	HitReactFacingTarget = DamageCauser;

	// 데미지는 먼저 적용하고, 그 다음 "새 피격 리액션으로 갈아탈 수 있는지"를 판단한다.
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);

	// 데미지 적용 (기존 시스템 활용)
	OnDamaged(FMath::RoundToInt(DamageAmount), AttackerCharacter);

	if (bExecutionReady || bBeingExecuted)
	{
		return;
	}

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
	const ECharacterHitReactDirection HitDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	UAnimMontage* HitReactMontage = GetHitReactMontage(HitType, HitDirection);
	StartHitReact(HitType, HitDirection);
	ApplyHitReactKnockback(DamageCauser, DefenseKnockbackData.HitReact, HitReactMontage);
}

bool AJunMonster::TryHandleIncomingHitBeforeDamage(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	return false;
}

void AJunMonster::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	if (CurrentState == EMonsterState::Dead || bExecutionReady || bBeingExecuted || Damage <= 0)
	{
		return;
	}

	const int32 PreviousHp = Hp;
	Hp = FMath::Clamp(Hp - Damage, 0, MaxHp);
	const int32 AppliedDamage = FMath::Max(0, PreviousHp - Hp);

	if (AppliedDamage > 0)
	{
		AddPosture(static_cast<float>(AppliedDamage) * PostureGainPerDamage);
	}

	if (Hp <= 0)
	{
		Hp = 0;
		StartExecutionReady();
	}
}

void AJunMonster::NotifyAttackParriedBy(AJunPlayer* Parrier, float PostureScale)
{
	if (!Parrier || CurrentState == EMonsterState::Dead || bBeingExecuted)
	{
		return;
	}

	AddPosture(ParriedPostureGain * FMath::Max(0.f, PostureScale));
}

bool AJunMonster::IsExecutionReady() const
{
	return bExecutionReady && !bBeingExecuted && CurrentState != EMonsterState::Dead;
}

bool AJunMonster::CanBeExecutedBy(const AJunPlayer* Player) const
{
	if (!Player || !IsExecutionReady())
	{
		return false;
	}

	const float Range = FMath::Max(ExecutionInteractRange, 0.f);
	if (FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(Range))
	{
		return false;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	if (ToPlayer.IsNearlyZero())
	{
		return true;
	}

	const float FrontDot = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToPlayer.GetSafeNormal());
	const float MinFrontDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(ExecutionFrontAngle, 0.f, 180.f)));
	return FrontDot >= MinFrontDot;
}

bool AJunMonster::TryBeginExecutionBy(AJunPlayer* Player)
{
	if (!CanBeExecutedBy(Player))
	{
		return false;
	}

	bExecutionReady = false;
	bBeingExecuted = true;
	bExecutionResultApplied = false;
	CurrentExecutionInstigator = Player;
	CurrentExecutionMontage = nullptr;
	ExecutionReadyRemainTime = 0.f;
	GetWorldTimerManager().ClearTimer(ExecutionRecoveryTimerHandle);

	StopAIMovement();
	StopAllAttackTraces();
	CancelCombatTurn();
	ResetCombatTurnState();
	if (bIsAttacking)
	{
		FinishAttack();
	}
	EndHitReact();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->StopMovementImmediately();
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	return true;
}

void AJunMonster::TriggerPendingExecutionMontage(bool bApplyResultImmediately)
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	const bool bFinalExecution = CurrentLifeCount <= 1;
	UAnimMontage* MontageToPlay = bFinalExecution
		? (ExecutedFinishMontage ? ExecutedFinishMontage.Get() : ExecutedDeathMontage.Get())
		: ExecutedMontage.Get();

	if (MontageToPlay)
	{
		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
			if (!bFinalExecution)
			{
				MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnExecutionMontageEnded);
				CurrentExecutionMontage = MontageToPlay;
			}
		}

		PlayAnimMontage(MontageToPlay);
	}

	if (bApplyResultImmediately)
	{
		ApplyPendingExecutionResult();
	}

	if (!MontageToPlay && !bFinalExecution)
	{
		FinishExecutionRecovery();
	}
}

void AJunMonster::ApplyPendingExecutionResult()
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	bExecutionResultApplied = true;
	CurrentLifeCount = FMath::Max(0, CurrentLifeCount - 1);
	CurrentPosture = 0.f;
	Hp = 0;

	if (CurrentLifeCount <= 0)
	{
		EndAttackFacingWindow();
		AddGameplayTag(JunGameplayTags::State_Condition_Dead);
		SetMonsterState(EMonsterState::Dead);
	}
}

void AJunMonster::CancelPendingExecution()
{
	if (!bBeingExecuted || bExecutionResultApplied || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	RestoreExecutionCapsuleCollisionIgnore();
	RestoreExecutionCapsuleRadius();
	bBeingExecuted = false;
	ExecutionReadyRemainTime = 0.f;
	CurrentPosture = 0.f;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();

	if (Hp <= 0 && CurrentLifeCount > 0)
	{
		Hp = 1;
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
}

void AJunMonster::RestoreExecutionCapsuleCollisionIgnore()
{
	AJunPlayer* ExecutionInstigator = CurrentExecutionInstigator.Get();
	if (!ExecutionInstigator)
	{
		return;
	}

	if (UCapsuleComponent* MonsterCapsule = GetCapsuleComponent())
	{
		MonsterCapsule->IgnoreActorWhenMoving(ExecutionInstigator, false);
	}

	if (UCapsuleComponent* PlayerCapsule = ExecutionInstigator->GetCapsuleComponent())
	{
		PlayerCapsule->IgnoreActorWhenMoving(this, false);
	}
}

void AJunMonster::RestoreExecutionCapsuleRadius()
{
	if (UCapsuleComponent* MonsterCapsule = GetCapsuleComponent())
	{
		if (bExecutionCapsuleRadiusReduced && SavedExecutionCapsuleRadius > 0.f)
		{
			MonsterCapsule->SetCapsuleRadius(SavedExecutionCapsuleRadius, true);
		}
	}

	SavedExecutionCapsuleRadius = 0.f;
	bExecutionCapsuleRadiusReduced = false;
}

bool AJunMonster::HasExecutionResultApplied() const
{
	return bExecutionResultApplied;
}

void AJunMonster::HandleEquipWeaponNotify()
{
	SetWeaponVisible(true);
	AttachWeaponToHandSocket();
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

int32 AJunMonster::GetCurrentLifeCount() const
{
	return CurrentLifeCount;
}

int32 AJunMonster::GetMaxLifeCount() const
{
	return MaxLifeCount;
}

bool AJunMonster::IsFinalExecution() const
{
	return CurrentLifeCount <= 1;
}

float AJunMonster::GetCurrentPosture() const
{
	return CurrentPosture;
}

float AJunMonster::GetMaxPosture() const
{
	return MaxPosture;
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

void AJunMonster::StartCombatBGM()
{
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->PlayCombatBGM(CombatBGM);
		return;
	}

	if (!CombatBGM || !CombatBGMComponent)
	{
		return;
	}

	if (CombatBGMComponent->IsPlaying())
	{
		return;
	}

	CombatBGMComponent->SetSound(CombatBGM);
	if (CombatBGMFadeInTime > 0.f)
	{
		CombatBGMComponent->FadeIn(CombatBGMFadeInTime);
	}
	else
	{
		CombatBGMComponent->Play();
	}
}

void AJunMonster::StopCombatBGM()
{
	if (AJunBGMManager* BGMManager = AJunBGMManager::Get(this))
	{
		BGMManager->ReturnToMapBGM();
		return;
	}

	if (!CombatBGMComponent || !CombatBGMComponent->IsPlaying())
	{
		return;
	}

	if (CombatBGMFadeOutTime > 0.f)
	{
		CombatBGMComponent->FadeOut(CombatBGMFadeOutTime, 0.f);
	}
	else
	{
		CombatBGMComponent->Stop();
	}
}

void AJunMonster::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, EJunWeaponNiagaraComponent NiagaraComponent, const FJunAttackTraceOverrideData& TraceOverrideData)
{
	if (!EquippedWeapon)
	{
		return;
	}

	EquippedWeapon->SetAttackHitReactType(HitReactType);
	EquippedWeapon->SetAttackDamageData(DamageData);
	EquippedWeapon->SetAttackDefenseKnockbackData(DefenseKnockbackData);
	EquippedWeapon->ApplyAttackTraceOverride(TraceOverrideData);
	EquippedWeapon->StartAttackTrace(NiagaraComponent);
}

void AJunMonster::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
	if (!EquippedWeapon)
	{
		return;
	}

	EquippedWeapon->EndAttackTrace(NiagaraComponent);
}

void AJunMonster::BeginKickAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	if (EquippedKickWeapon)
	{
		EquippedKickWeapon->SetAttackHitReactType(HitReactType);
		EquippedKickWeapon->SetAttackDamageData(DamageData);
		EquippedKickWeapon->SetAttackDefenseKnockbackData(DefenseKnockbackData);
		EquippedKickWeapon->StartAttackTrace();
	}

	if (EquippedKickWeaponRight)
	{
		EquippedKickWeaponRight->SetAttackHitReactType(HitReactType);
		EquippedKickWeaponRight->SetAttackDamageData(DamageData);
		EquippedKickWeaponRight->SetAttackDefenseKnockbackData(DefenseKnockbackData);
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

void AJunMonster::BeginWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->ActivateWeaponNiagara(ComponentType);
	}
}

void AJunMonster::EndWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->DeactivateWeaponNiagara(ComponentType);
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

	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	EndAttackFacingWindow();
	AttackFacingRemainTime = 0.f;

	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);

	LastCombatTurnStartYaw = GetActorRotation().Yaw;
	if (MonsterAnimInstance->Montage_Play(TurnMontage, CombatTurnPlayRate) <= 0.f)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
		return false;
	}
	LastCombatTurnPostPlayYaw = GetActorRotation().Yaw;
	if (bDebugCombatTurnYaw)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][StartToState] Play=%s YawDelta=%.2f Before=%.2f AfterPlay=%.2f AfterPlayDelta=%.2f Velocity=%.2f State=%s"),
			*GetNameSafe(TurnMontage),
			YawDelta,
			LastCombatTurnStartYaw,
			LastCombatTurnPostPlayYaw,
			FMath::FindDeltaAngleDegrees(LastCombatTurnStartYaw, LastCombatTurnPostPlayYaw),
			GetVelocity().Size2D(),
			GetMonsterStateDebugText(CurrentState));
	}

	CurrentCombatTurnMontage = TurnMontage;
	bCombatTurnInProgress = true;
	PendingStateAfterCombatTurn = NextState;
	bHasPendingStateAfterCombatTurn = true;
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

	if (const UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (MonsterAnimInstance->GetCurrentActiveMontage() != nullptr)
		{
			return false;
		}
	}

	return GetVelocity().Size2D() <= CombatTurnMaxGroundSpeed;
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

	if (const UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (MonsterAnimInstance->GetCurrentActiveMontage() != nullptr)
		{
			return false;
		}
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

	StopAIMovement();
	CombatMoveInput = FVector2D::ZeroVector;
	SetDesiredMoveAxes(0.f, 0.f);
	EndAttackFacingWindow();
	AttackFacingRemainTime = 0.f;

	MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
	MonsterAnimInstance->OnMontageEnded.AddDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);

	LastCombatTurnStartYaw = GetActorRotation().Yaw;
	if (MonsterAnimInstance->Montage_Play(TurnMontage, CombatTurnPlayRate) <= 0.f)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCombatTurnMontageEnded);
		return false;
	}
	LastCombatTurnPostPlayYaw = GetActorRotation().Yaw;
	if (bDebugCombatTurnYaw)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][Start] Play=%s YawDelta=%.2f Before=%.2f AfterPlay=%.2f AfterPlayDelta=%.2f Velocity=%.2f State=%s"),
			*GetNameSafe(TurnMontage),
			YawDelta,
			LastCombatTurnStartYaw,
			LastCombatTurnPostPlayYaw,
			FMath::FindDeltaAngleDegrees(LastCombatTurnStartYaw, LastCombatTurnPostPlayYaw),
			GetVelocity().Size2D(),
			GetMonsterStateDebugText(CurrentState));
	}

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
	if (IsInvalidCombatTarget(Target))
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(AIData.LoseTargetRange);
}

bool AJunMonster::CanRemainInCombat(AActor* Target) const
{
	if (IsInvalidCombatTarget(Target))
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
	if (IsInvalidCombatTarget(CurrentTarget) || !AttackSelection.Montage)
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

	if (AttachedArrow)
	{
		AttachedArrow->Destroy();
		AttachedArrow = nullptr;
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

void AJunMonster::BeginHitReactFacingWindow(float InterpSpeed)
{
	if (!HitReactFacingTarget)
	{
		bHitReactFacingWindowActive = false;
		HitReactFacingWindowInterpSpeed = 0.f;
		return;
	}

	bHitReactFacingWindowActive = true;
	HitReactFacingWindowInterpSpeed = FMath::Max(InterpSpeed, 0.f);
}

void AJunMonster::EndHitReactFacingWindow()
{
	bHitReactFacingWindowActive = false;
	HitReactFacingWindowInterpSpeed = 0.f;
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
		if (bDebugCombatTurnYaw)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][EndIgnored] Ended=%s Current=%s Interrupted=%s Yaw=%.2f"),
				*GetNameSafe(Montage),
				*GetNameSafe(CurrentCombatTurnMontage),
				bInterrupted ? TEXT("true") : TEXT("false"),
				GetActorRotation().Yaw);
		}
		return;
	}

	LastCombatTurnEndYaw = GetActorRotation().Yaw;
	if (bDebugCombatTurnYaw)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterTurn][End] Montage=%s Interrupted=%s Start=%.2f AfterPlay=%.2f End=%.2f TotalDelta=%.2f Velocity=%.2f State=%s"),
			*GetNameSafe(Montage),
			bInterrupted ? TEXT("true") : TEXT("false"),
			LastCombatTurnStartYaw,
			LastCombatTurnPostPlayYaw,
			LastCombatTurnEndYaw,
			FMath::FindDeltaAngleDegrees(LastCombatTurnStartYaw, LastCombatTurnEndYaw),
			GetVelocity().Size2D(),
			GetMonsterStateDebugText(CurrentState));
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
			const FName InitialWeaponSocketName = bStartWithCutsceneWait ? SheathedWeaponSocketName : WeaponSocketName;
			AttachWeaponToSocket(InitialWeaponSocketName);
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

	if (DefaultBowClass)
	{
		EquippedBow = World->SpawnActor<AWeaponActor>(
			DefaultBowClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!EquippedBow)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn monster bow."));
		}
		else
		{
			EquippedBow->StopAttackTrace();
			EquippedBow->SetActorTickEnabled(false);
			EquippedBow->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				BowSocketName
			);
			EquippedBow->SetActorHiddenInGame(true);
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

void AJunMonster::AttachWeaponToSocket(FName SocketName)
{
	if (!EquippedWeapon || !GetMesh() || SocketName.IsNone())
	{
		return;
	}

	EquippedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName
	);

	const bool bAttachedToHand = SocketName == WeaponSocketName;
	bWeaponAttachedToHand = bAttachedToHand;
	EquippedWeapon->SetWeaponEffectsEnabled(bWeaponAttachedToHand && !EquippedWeapon->IsHidden());
}

void AJunMonster::AttachWeaponToHandSocket()
{
	AttachWeaponToSocket(WeaponSocketName);
}

void AJunMonster::AttachWeaponToSheathedSocket()
{
	AttachWeaponToSocket(SheathedWeaponSocketName);
}

void AJunMonster::SetWeaponVisible(bool bVisible)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(!bVisible);
		EquippedWeapon->SetWeaponEffectsEnabled(bVisible && bWeaponAttachedToHand);
	}
}

void AJunMonster::SetBowVisible(bool bVisible)
{
	if (EquippedBow)
	{
		EquippedBow->SetActorHiddenInGame(!bVisible);
	}
}

void AJunMonster::SpawnAttachedArrow()
{
	if (!DefaultArrowProjectileClass || !GetWorld() || !GetMesh())
	{
		return;
	}

	if (AttachedArrow)
	{
		AttachedArrow->Destroy();
		AttachedArrow = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AttachedArrow = GetWorld()->SpawnActor<AArrowProjectile>(
		DefaultArrowProjectileClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!AttachedArrow)
	{
		return;
	}

	AttachedArrow->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ArrowSocketName
	);
}

void AJunMonster::FireAttachedArrow(
	EHitReactType HitReactType,
	const FJunAttackDamageData& DamageData,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	float Speed,
	float LifeSeconds,
	float HomingDuration,
	float HomingInterpSpeed,
	float HomingTargetHeightOffset)
{
	if (!AttachedArrow)
	{
		SpawnAttachedArrow();
	}

	if (!AttachedArrow)
	{
		return;
	}

	FVector AimPoint = CurrentTarget ? CurrentTarget->GetActorLocation() : (GetActorLocation() + GetActorForwardVector() * 1000.f);
	AimPoint.Z += 50.f;
	const FVector FireDirection = (AimPoint - AttachedArrow->GetActorLocation()).GetSafeNormal();

	AttachedArrow->InitializeArrow(this, HitReactType, DamageData, DefenseKnockbackData);
	AttachedArrow->Fire(FireDirection, Speed, LifeSeconds, CurrentTarget.Get(), HomingDuration, HomingInterpSpeed, HomingTargetHeightOffset);
	AttachedArrow = nullptr;
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

void AJunMonster::UpdateHitReactFacing(float DeltaTime)
{
	if (!bHitReactFacingWindowActive)
	{
		return;
	}

	if (!HitReactFacingTarget)
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

	const float ResolvedInterpSpeed = HitReactFacingWindowInterpSpeed > 0.f ? HitReactFacingWindowInterpSpeed : AttackFacingInterpSpeed;
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		Direction.Rotation(),
		DeltaTime,
		ResolvedInterpSpeed
	));
}

void AJunMonster::EndHitReact()
{
	// HitReact 종료는 태그 해제만 담당하고,
	// 이후 AI 상태 갱신은 CanUpdateBehavior()가 다시 허용한다.
	const EHitReactType EndedHitReact = CurrentHitReact;
	CurrentHitReact = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();
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

ECharacterKnockbackDirection AJunMonster::DetermineKnockbackDirectionFromDamageCauser(const AActor* DamageCauser) const
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

void AJunMonster::ApplyHitReactKnockback(AActor* DamageCauser, const FJunDefenseKnockbackData& KnockbackData, UAnimMontage* HitReactMontage)
{
	if (KnockbackData.Strength <= 0.f || DoesMontageUseRootMotion(HitReactMontage))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	FVector KnockbackDirection = FVector::ZeroVector;
	switch (DetermineKnockbackDirectionFromDamageCauser(DamageCauser))
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

	HitReactKnockbackBrakingDecelerationOverride = KnockbackData.BrakingDeceleration;
	HitReactKnockbackGroundFrictionOverride = KnockbackData.GroundFriction;
	HitReactKnockbackBrakingFrictionFactorOverride = KnockbackData.BrakingFrictionFactor;
	HitReactKnockbackBrakingOverrideRemainTime = KnockbackData.OverrideDuration;
	bHitReactKnockbackBrakingOverrideActive = true;

	MovementComponent->AddImpulse(KnockbackDirection * KnockbackData.Strength, true);
}

void AJunMonster::UpdateHitReactKnockbackBraking(float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (HitReactKnockbackBrakingOverrideRemainTime > 0.f)
	{
		HitReactKnockbackBrakingOverrideRemainTime = FMath::Max(0.f, HitReactKnockbackBrakingOverrideRemainTime - DeltaTime);
		MovementComponent->BrakingDecelerationWalking = HitReactKnockbackBrakingDecelerationOverride;
		MovementComponent->GroundFriction = HitReactKnockbackGroundFrictionOverride;
		MovementComponent->BrakingFrictionFactor = HitReactKnockbackBrakingFrictionFactorOverride;

		if (HitReactKnockbackBrakingOverrideRemainTime <= 0.f)
		{
			RestoreDefaultMonsterMovementBrakingSettings();
			bHitReactKnockbackBrakingOverrideActive = false;
		}

		return;
	}

	if (bHitReactKnockbackBrakingOverrideActive)
	{
		RestoreDefaultMonsterMovementBrakingSettings();
		bHitReactKnockbackBrakingOverrideActive = false;
	}
}

void AJunMonster::RestoreDefaultMonsterMovementBrakingSettings()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->BrakingDecelerationWalking = DefaultMonsterBrakingDecelerationWalking;
		MovementComponent->GroundFriction = DefaultMonsterGroundFriction;
		MovementComponent->BrakingFrictionFactor = DefaultMonsterBrakingFrictionFactor;
	}
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

	if (bExecutionReady || bBeingExecuted)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	return true;
}

void AJunMonster::AddPosture(float Amount)
{
	if (bDisablePostureGain)
	{
		return;
	}

	if (CurrentState == EMonsterState::Dead || bExecutionReady || bBeingExecuted || Amount <= 0.f)
	{
		return;
	}

	CurrentPosture = FMath::Clamp(CurrentPosture + Amount, 0.f, MaxPosture);
	PostureRecoveryDelayRemainTime = PostureRecoveryDelay;
	if (CurrentPosture >= MaxPosture)
	{
		if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			JunPlayerController->PlayBossPostureBreakGlow();
		}
		StartExecutionReady();
	}
}

void AJunMonster::UpdatePostureRecovery(float DeltaTime)
{
	if (bDisablePostureGain || CurrentPosture <= 0.f || !CanRecoverPosture())
	{
		return;
	}

	if (PostureRecoveryDelayRemainTime > 0.f)
	{
		PostureRecoveryDelayRemainTime = FMath::Max(0.f, PostureRecoveryDelayRemainTime - DeltaTime);
		return;
	}

	const float RecoveryAmount = PostureRecoveryRate * GetPostureRecoveryVitalityScale() * DeltaTime;
	CurrentPosture = FMath::Max(0.f, CurrentPosture - RecoveryAmount);
}

bool AJunMonster::CanRecoverPosture() const
{
	if (CurrentState == EMonsterState::Dead || bExecutionReady || bBeingExecuted)
	{
		return false;
	}

	if (bIsAttacking || IsInHitReact())
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	return true;
}

float AJunMonster::GetPostureRecoveryVitalityScale() const
{
	const float HpRatio = MaxHp > 0
		? FMath::Clamp(static_cast<float>(Hp) / static_cast<float>(MaxHp), 0.f, 1.f)
		: 0.f;

	if (HpRatio > 0.75f)
	{
		return 1.f;
	}
	if (HpRatio > 0.5f)
	{
		return 0.66f;
	}
	if (HpRatio > 0.25f)
	{
		return 0.33f;
	}
	return 0.01f;
}

void AJunMonster::StartExecutionReady()
{
	if (CurrentState == EMonsterState::Dead || bExecutionReady || bBeingExecuted)
	{
		return;
	}

	bExecutionReady = true;
	ExecutionReadyRemainTime = ExecutionReadyDuration;
	CurrentPosture = 0.f;

	StopAIMovement();
	StopAllAttackTraces();
	CancelCombatTurn();
	ResetCombatTurnState();
	if (bIsAttacking)
	{
		FinishAttack();
	}
	EndHitReact();
	SetDesiredMoveAxes(0.f, 0.f);
	CombatMoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->StopMovementImmediately();
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	if (ReadyExecutedMontage)
	{
		PlayAnimMontage(ReadyExecutedMontage);
	}
}

void AJunMonster::EndExecutionReady()
{
	if (!bExecutionReady || bBeingExecuted)
	{
		return;
	}

	bExecutionReady = false;
	ExecutionReadyRemainTime = 0.f;
	CurrentPosture = 0.f;

	// HP 0으로 처형 대기가 끝나면 몬스터가 즉시 Dead 태그를 얻지 않도록 최소 생존 HP를 보장한다.
	if (Hp <= 0 && CurrentLifeCount > 0)
	{
		Hp = 1;
	}

	if (ReadyExecutedMontage)
	{
		if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			MonsterAnimInstance->Montage_Stop(0.15f, ReadyExecutedMontage);
		}
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
}

void AJunMonster::FinishExecutionRecovery()
{
	if (!bBeingExecuted || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	RestoreExecutionCapsuleCollisionIgnore();
	RestoreExecutionCapsuleRadius();
	Hp = MaxHp;
	bBeingExecuted = false;
	bExecutionResultApplied = false;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();
	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
	}
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);

	if (CurrentTarget)
	{
		SetMonsterState(EMonsterState::Combat);
	}
	else
	{
		SetMonsterState(EMonsterState::Idle);
	}
}

void AJunMonster::OnExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentExecutionMontage)
	{
		return;
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
	}

	CurrentExecutionMontage = nullptr;
	EndAttackFacingWindow();

	if (!bBeingExecuted || CurrentState == EMonsterState::Dead)
	{
		return;
	}

	FinishExecutionRecovery();
}

void AJunMonster::OnCutsceneWaitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CutsceneWaitMontage)
	{
		return;
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCutsceneWaitMontageEnded);
	}

	if (CurrentState != EMonsterState::CutsceneWait)
	{
		return;
	}

	if (CurrentTarget)
	{
		PlayCutsceneWaitEquipMontage(CutsceneWaitEquipBlendInTime);
	}
	else
	{
		SetMonsterState(EMonsterState::Idle);
	}
}

void AJunMonster::OnCutsceneWaitEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CutsceneWaitEquipMontage)
	{
		return;
	}

	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnCutsceneWaitEquipMontageEnded);
	}

	if (CurrentState != EMonsterState::CutsceneWait)
	{
		return;
	}

	bCutsceneWaitEquipMontageActive = false;
	SetMonsterState(CurrentTarget ? EMonsterState::BattleStart : EMonsterState::Idle);
}

void AJunMonster::UpdateExecutionFacing(float DeltaTime)
{
	if (!CurrentExecutionInstigator)
	{
		EndAttackFacingWindow();
		return;
	}

	FVector ToExecutor = CurrentExecutionInstigator->GetActorLocation() - GetActorLocation();
	ToExecutor.Z = 0.f;
	if (ToExecutor.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = ToExecutor.Rotation();
	const float FacingInterpSpeed = AttackFacingWindowInterpSpeed > 0.f
		? AttackFacingWindowInterpSpeed
		: AttackFacingInterpSpeed;

	SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FacingInterpSpeed));
}

void AJunMonster::ResetExecutionRuntimeState()
{
	RestoreExecutionCapsuleCollisionIgnore();
	RestoreExecutionCapsuleRadius();
	bExecutionReady = false;
	bBeingExecuted = false;
	bExecutionResultApplied = false;
	ExecutionReadyRemainTime = 0.f;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;
	CurrentPosture = 0.f;
	EndAttackFacingWindow();
	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
	}
	GetWorldTimerManager().ClearTimer(ExecutionRecoveryTimerHandle);
}

float AJunMonster::GetExecutionMontageDuration(const UAnimMontage* Montage, float FallbackDuration) const
{
	return Montage ? FMath::Max(Montage->GetPlayLength(), 0.01f) : FallbackDuration;
}
