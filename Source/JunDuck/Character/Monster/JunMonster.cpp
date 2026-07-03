#include "Character/Monster/JunMonster.h"
#include "AI/JunAIController.h"
#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunBGMManager.h"
#include "System/JunTimeEffectSubsystem.h"
#include "UI/JunMonsterHUDWidget.h"
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

	bool IsLightingShockHitType(const EHitReactType HitType)
	{
		return HitType == EHitReactType::Lighting_Shock;
	}

	bool IsFrontHitReactDirection(const ECharacterHitReactDirection HitDirection)
	{
		return HitDirection == ECharacterHitReactDirection::Front_F ||
			HitDirection == ECharacterHitReactDirection::Front_FL ||
			HitDirection == ECharacterHitReactDirection::Front_FR;
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

	Hp = 400;
	MaxHp = 400;
	HitDamageSoundVolume = 2.f;

	LightPhysicalHitReactionSettings.bEnable = false;
	LightPhysicalHitReactionSettings.RootBoneName = TEXT("spine_02");
	LightPhysicalHitReactionSettings.BlendWeight = 0.22f;
	LightPhysicalHitReactionSettings.Duration = 0.2f;
	LightPhysicalHitReactionSettings.ImpulseStrength = 3000.f;
	LightPhysicalHitReactionSettings.OrientationStrength = 100.f;
	LightPhysicalHitReactionSettings.AngularVelocityStrength = 50.f;
	LightPhysicalHitReactionSettings.MaxAngularForce = 50000.f;
	LightPhysicalHitReactionSettings.MinImpulseZ = -0.35f;
	LightPhysicalHitReactionSettings.MaxImpulseZ = -0.1f;
	LightPhysicalHitReactionSettings.bUseAdditionalRoots = false;
	LightPhysicalHitReactionSettings.AdditionalRootBoneNames = { TEXT("thigh_l"), TEXT("thigh_r") };
	LightPhysicalHitReactionSettings.AdditionalRootBlendWeight = 0.08f;
	LightPhysicalHitReactionSettings.AdditionalRootImpulseScale = 0.08f;
	LightPhysicalHitReactionSettings.bUseDampenedBones = true;
	LightPhysicalHitReactionSettings.DampenedBoneNames = { TEXT("upperarm_l"), TEXT("upperarm_r") };
	LightPhysicalHitReactionSettings.DampenedBoneBlendWeight = 0.06f;

	SuperArmorPhysicalHitReactionSettings.bEnable = true;
	SuperArmorPhysicalHitReactionSettings.RootBoneName = TEXT("spine_02");
	SuperArmorPhysicalHitReactionSettings.BlendWeight = 0.16f;
	SuperArmorPhysicalHitReactionSettings.Duration = 0.2f;
	SuperArmorPhysicalHitReactionSettings.ImpulseStrength = 3000.f;
	SuperArmorPhysicalHitReactionSettings.OrientationStrength = 900.f;
	SuperArmorPhysicalHitReactionSettings.AngularVelocityStrength = 90.f;
	SuperArmorPhysicalHitReactionSettings.MaxAngularForce = 50000.f;
	SuperArmorPhysicalHitReactionSettings.MinImpulseZ = -0.25f;
	SuperArmorPhysicalHitReactionSettings.MaxImpulseZ = 0.f;
	SuperArmorPhysicalHitReactionSettings.bUseAdditionalRoots = true;
	SuperArmorPhysicalHitReactionSettings.AdditionalRootBoneNames = { TEXT("thigh_l"), TEXT("thigh_r") };
	SuperArmorPhysicalHitReactionSettings.AdditionalRootBlendWeight = 0.05f;
	SuperArmorPhysicalHitReactionSettings.AdditionalRootImpulseScale = 0.2f;
	SuperArmorPhysicalHitReactionSettings.bUseDampenedBones = true;
	SuperArmorPhysicalHitReactionSettings.DampenedBoneNames = { TEXT("upperarm_l"), TEXT("upperarm_r") };
	SuperArmorPhysicalHitReactionSettings.DampenedBoneBlendWeight = 0.f;

	// 캡슐 충돌 프리셋
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("EnemyCapsule"));

	// 메쉬 기본 위치 및 회전 조정
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));

	CombatBGMComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CombatBGMComponent"));
	CombatBGMComponent->SetupAttachment(RootComponent);
	CombatBGMComponent->bAutoActivate = false;

	MonsterHUDWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MonsterHUDWidget"));
	MonsterHUDWidgetComponent->SetupAttachment(RootComponent);
	MonsterHUDWidgetComponent->SetRelativeLocation(MonsterHUDRelativeLocation);
	MonsterHUDWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	MonsterHUDWidgetComponent->SetDrawSize(MonsterHUDBaseDrawSize);
	MonsterHUDWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MonsterHUDWidgetComponent->SetHiddenInGame(true);

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
	bMonsterWorldHUDRevealed = false;

	// 팀 설정
	SetTeamTag(JunGameplayTags::Team_Enemy);
	PrimeSoundForFirstPlayback(BossClearSound);

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

	if (MonsterHUDWidgetComponent)
	{
		MonsterHUDWidgetComponent->SetDrawSize(MonsterHUDBaseDrawSize);
	}
	UpdateMonsterWorldHUD();
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

	const bool bWasExecutionReadyAtTickStart = bExecutionReady;
	if (bExecutionReady && !bBeingExecuted)
	{
		ExecutionReadyRemainTime = FMath::Max(0.f, ExecutionReadyRemainTime - DeltaTime);
		if (ExecutionReadyRemainTime <= 0.f)
		{
			EndExecutionReady();
		}
	}

	if (CurrentState != EMonsterState::Dead && (bExecutionReady || bBeingExecuted) && bAttackFacingWindowActive)
	{
		UpdateExecutionFacing(DeltaTime);
	}

	if (bWasExecutionReadyAtTickStart)
	{
		StateTime += DeltaTime;
		return;
	}

	UpdateAttack(DeltaTime);
	UpdateCombatTurn(DeltaTime);
	UpdateMonsterCodeMove(DeltaTime);
	UpdatePostureRecovery(DeltaTime);
	UpdateMonsterWorldHUD();

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

	if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Monster_DangerAttack) ||
		EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Boss_DangerAttack))
	{
		if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			JunPlayerController->PlayDangerMarkerOnPlayer();
		}
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
	ReceiveHit(HitType, DamageAmount, DamageCauser, SwingDirection, FJunAttackDefenseKnockbackData(), FJunAttackDefenseRuleData(), 0.f);
}

FJunAttackHitResult AJunMonster::ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	float PostureDamageAmount)
{
	FJunAttackHitResult Result;
	Result.HpBeforeHit = GetHp();
	Result.bWasExecutionReadyBeforeHit = bExecutionReady;
	auto FinalizeResult = [this, &Result]()
	{
		Result.HpAfterHit = GetHp();
		Result.bDamageApplied = Result.HpAfterHit < Result.HpBeforeHit;
		Result.bPhysicalReactionOnly = bLastHitPhysicalReactionOnly;
		return Result;
	};

	bLastHitPhysicalReactionOnly = false;

	if (CurrentState == EMonsterState::Dead || bBeingExecuted)
	{
		return FinalizeResult();
	}
	Result.bProcessed = true;

	// 처형 대기 중에는 레디 포즈와 상태를 유지한 채 타격 연출만 허용한다.
	// 데미지, 체간, 히트 리액션, 넉백, 히트스탑은 적용하지 않는다.
	if (bExecutionReady)
	{
		PlayHitDamageSound();
		return FinalizeResult();
	}

	Result.DefenseOutcome = TryHandleIncomingHitBeforeDamage(
		HitType,
		DamageAmount,
		DamageCauser,
		SwingDirection,
		DefenseKnockbackData,
		DefenseRuleData);
	if (Result.DefenseOutcome != EJunDefenseOutcome::None)
	{
		Result.ReactionOutcome = EJunHitReactionOutcome::Montage;
		return FinalizeResult();
	}

	HitReactFacingTarget = DamageCauser;

	// 데미지는 먼저 적용하고, 그 다음 "새 피격 리액션으로 갈아탈 수 있는지"를 판단한다.
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);

	// 데미지 적용 (기존 시스템 활용)
	OnDamaged(FMath::RoundToInt(DamageAmount), AttackerCharacter);

	if (bExecutionReady || bBeingExecuted)
	{
		return FinalizeResult();
	}

	AddPosture(PostureDamageAmount);

	if (bExecutionReady || bBeingExecuted)
	{
		return FinalizeResult();
	}

	// 사망 체크
	if (Is_Dead())
	{
		CurrentHitReact = EHitReactType::Dead;
		SetMonsterState(EMonsterState::Dead);
		Result.ReactionOutcome = EJunHitReactionOutcome::Montage;
		return FinalizeResult();
	}

	const FMonsterHitReactionDecision ReactionDecision = EvaluateMonsterHitReaction(
		HitType,
		DamageCauser,
		SwingDirection,
		DefenseRuleData);
	Result.ReactionOutcome = ApplyMonsterHitReaction(
		ReactionDecision,
		HitType,
		DamageCauser,
		SwingDirection,
		DefenseKnockbackData);
	return FinalizeResult();
}

AJunMonster::FMonsterHitReactionDecision AJunMonster::EvaluateMonsterHitReaction(
	EHitReactType HitType,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	FMonsterHitReactionDecision Decision;
	if (!CanBeInterruptedBy(HitType))
	{
		Decision.Mode = EMonsterHitReactionMode::SuperArmor;
		return Decision;
	}

	if (!ShouldStartHitReact(HitType))
	{
		Decision.Mode = HitType == EHitReactType::LightHit
			? EMonsterHitReactionMode::SuperArmor
			: EMonsterHitReactionMode::PhysicalOnly;
		return Decision;
	}

	Decision.Mode = EMonsterHitReactionMode::Montage;
	Decision.Direction = AdjustHitReactDirection(
		HitType,
		DetermineHitReactDirection(DamageCauser, SwingDirection, DefenseRuleData),
		DefenseRuleData);
	Decision.Montage = GetHitReactMontage(HitType, Decision.Direction);
	return Decision;
}

EJunHitReactionOutcome AJunMonster::ApplyMonsterHitReaction(
	const FMonsterHitReactionDecision& Decision,
	EHitReactType HitType,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);
	if (Decision.Mode != EMonsterHitReactionMode::Montage)
	{
		ApplyPhysicalHitReaction(
			ResolvePhysicalHitDirection(DamageCauser, SwingDirection),
			SuperArmorPhysicalHitReactionSettings,
			1.f);
		bLastHitPhysicalReactionOnly = true;
		RequestPlayerAttackHitStop(AttackerCharacter);
		OnIncomingHitResolvedWithoutHitReact(HitType);
		return Decision.Mode == EMonsterHitReactionMode::SuperArmor
			? EJunHitReactionOutcome::SuperArmor
			: EJunHitReactionOutcome::PhysicalOnly;
	}

	if (HitType == EHitReactType::LightHit)
	{
		ApplyPhysicalHitReaction(SwingDirection, LightPhysicalHitReactionSettings, 1.f);
	}
	else if (IsHeavyHitType(HitType) || IsLargeHitType(HitType))
	{
		ApplyPhysicalHitReaction(SwingDirection, SuperArmorPhysicalHitReactionSettings, 1.f);
	}

	StartHitReact(HitType, Decision.Direction);
	ApplyHitReactKnockback(DamageCauser, DefenseKnockbackData.HitReact, Decision.Montage);
	RequestPlayerAttackHitStop(AttackerCharacter);
	return EJunHitReactionOutcome::Montage;
}

FVector AJunMonster::ResolvePhysicalHitDirection(AActor* DamageCauser, const FVector& SwingDirection) const
{
	return SwingDirection.IsNearlyZero()
		? (GetActorLocation() - (DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation())).GetSafeNormal()
		: SwingDirection.GetSafeNormal();
}

EJunDefenseOutcome AJunMonster::TryHandleIncomingHitBeforeDamage(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	return EJunDefenseOutcome::None;
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
		PlayHitDamageSound();
		SetMonsterWorldHUDRevealed(true);
	}

	if (Hp <= 0)
	{
		Hp = 0;
		StartExecutionReady();
	}
}

void AJunMonster::NotifyAttackParriedBy(
	AJunPlayer* Parrier,
	float PostureScale,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!Parrier || CurrentState == EMonsterState::Dead || bExecutionReady || bBeingExecuted)
	{
		return;
	}

	AddPosture(ParriedPostureGain * FMath::Max(0.f, PostureScale));
}

void AJunMonster::NotifyPlayerParrySucceeded(AJunPlayer* Parrier, bool bPerfectParry, bool bAirParry)
{
}

bool AJunMonster::NotifyMikiriCounteredBy(AJunPlayer* CounterPlayer)
{
	return false;
}

bool AJunMonster::IsMikiriCounterThreatActiveFor(const AJunPlayer* Player) const
{
	return Player && IsMikiriCounterThreatActive();
}

bool AJunMonster::CanBeMikiriCounteredBy(const AJunPlayer* Player) const
{
	return Player && !Is_Dead() && Player->IsEnemyTo(this);
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

void AJunMonster::ApplyPostureFromPlayer(float Amount)
{
	AddPosture(Amount);
}

void AJunMonster::OnPlayerFakeDeathStarted()
{
	EnterPlayerDeathWait();
}

void AJunMonster::OnPlayerFakeDeathRevived()
{
	ResumeAfterPlayerFakeDeath();
}

void AJunMonster::OnPlayerRealDeathStarted()
{
	EnterPlayerDeathWait();
}

void AJunMonster::OnPlayerRealDeathRespawned()
{
	ResetAfterPlayerRealDeath();
}

FVector AJunMonster::ResolveJumpCounterStompAnchorLocation(
	const AJunPlayer* Player,
	const FJunJumpCounterStompPlacementSettings& PlacementSettings) const
{
	if (!Player)
	{
		return GetActorLocation();
	}

	const FVector TargetPoint = GetLockOnTargetPoint();
	const FVector MonsterLocation = GetActorLocation();
	const FVector DesiredPoint =
		TargetPoint +
		GetActorForwardVector().GetSafeNormal2D() * PlacementSettings.ForwardOffset +
		FVector::UpVector * PlacementSettings.HeightOffset;

	FVector DesiredOffset2D = DesiredPoint - MonsterLocation;
	DesiredOffset2D.Z = 0.f;

	if (DesiredOffset2D.IsNearlyZero())
	{
		DesiredOffset2D = Player->GetActorLocation() - MonsterLocation;
		DesiredOffset2D.Z = 0.f;
	}

	if (DesiredOffset2D.IsNearlyZero())
	{
		DesiredOffset2D = -GetActorForwardVector();
		DesiredOffset2D.Z = 0.f;
	}

	const float MonsterCapsuleRadius = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.f;
	const float PlayerCapsuleRadius = Player->GetCapsuleComponent()
		? Player->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.f;
	const float MinSafeDistance = PlacementSettings.bIgnoreTargetCollisionDuringFollowUp
		? FMath::Max(0.f, PlacementSettings.MinVisualDistance)
		: MonsterCapsuleRadius + PlayerCapsuleRadius + FMath::Max(0.f, PlacementSettings.CapsuleMargin);
	const float DesiredDistance = DesiredOffset2D.Size();
	const float ResolvedDistance = FMath::Max(DesiredDistance, MinSafeDistance);
	const FVector SafeOffset2D = DesiredOffset2D.GetSafeNormal2D() * ResolvedDistance;

	return FVector(
		MonsterLocation.X + SafeOffset2D.X,
		MonsterLocation.Y + SafeOffset2D.Y,
		DesiredPoint.Z);
}

bool AJunMonster::CanStartJumpCounterStompFrom(const AJunPlayer* Player, float MaxStartDistance) const
{
	return Player &&
		!Is_Dead() &&
		FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) <= FMath::Max(0.f, MaxStartDistance);
}

bool AJunMonster::IsJumpCounterStompThreatActive() const
{
	return IsJumpCounterThreatActive();
}

void AJunMonster::SetJumpCounterStompCollisionIgnored(AJunPlayer* Player, bool bIgnore)
{
	UCapsuleComponent* PlayerCapsule = Player ? Player->GetCapsuleComponent() : nullptr;
	UCapsuleComponent* MonsterCapsule = GetCapsuleComponent();
	if (!PlayerCapsule || !Player)
	{
		return;
	}

	PlayerCapsule->IgnoreActorWhenMoving(this, bIgnore);
	if (MonsterCapsule)
	{
		MonsterCapsule->IgnoreActorWhenMoving(Player, bIgnore);
	}
}

void AJunMonster::ApplyJumpCounterStompResult(
	AJunPlayer* Player,
	EHitReactType HitReactType,
	const FJunAttackDamageData& DamageData,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	float PostureAmount)
{
	if (!Player || Is_Dead())
	{
		return;
	}

	FJunAttackDefenseRuleData DefenseRuleData;
	DefenseRuleData.bCanBeParried = false;
	DefenseRuleData.bCanBeGuarded = true;
	DefenseRuleData.bCanBeDodgedByInvincibility = false;

	const FVector SwingDirection = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
	FJunAttackHitContext HitContext;
	HitContext.HitReactType = HitReactType;
	HitContext.DamageData = DamageData;
	HitContext.Attacker = Player;
	HitContext.SwingDirection = SwingDirection.IsNearlyZero() ? Player->GetActorForwardVector() : SwingDirection;
	HitContext.DefenseKnockbackData = DefenseKnockbackData;
	HitContext.DefenseRuleData = DefenseRuleData;
	ProcessAttackHit(HitContext);

	AddPosture(PostureAmount);
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

FVector AJunMonster::GetExecutionCameraTargetPoint() const
{
	return GetLockOnTargetPoint();
}

bool AJunMonster::CanBeLockedOnBy(const AJunPlayer* Player) const
{
	return Player && CanBeLockOnTarget();
}

FVector AJunMonster::GetLockOnCameraTargetPoint() const
{
	return GetLockOnTargetPoint();
}

bool AJunMonster::IsLockOnExecutionReady() const
{
	return IsExecutionReady();
}

bool AJunMonster::CanBeAttackTargetedBy(const AJunPlayer* Player) const
{
	return Player && !Is_Dead() && Player->IsEnemyTo(this);
}

FVector AJunMonster::GetAttackTargetPoint(const AJunPlayer* Player) const
{
	return GetActorLocation();
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


