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

void AJunMonster::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, const FJunAttackDefenseRuleData& DefenseRuleData, EJunWeaponNiagaraComponent NiagaraComponent, const FJunAttackTraceOverrideData& TraceOverrideData)
{
	if (!EquippedWeapon)
	{
		return;
	}

	Super::BeginAttackTraceWindow(HitReactType, DamageData, DefenseKnockbackData, DefenseRuleData, NiagaraComponent, TraceOverrideData);

	AWeaponActor::FAttackTraceConfig TraceConfig;
	TraceConfig.HitReactType = HitReactType;
	TraceConfig.DamageData = DamageData;
	TraceConfig.DefenseKnockbackData = DefenseKnockbackData;
	TraceConfig.DefenseRuleData = DefenseRuleData;
	TraceConfig.OverrideData = TraceOverrideData;
	TraceConfig.NiagaraComponent = NiagaraComponent;
	EquippedWeapon->StartAttackTrace(TraceConfig);
}

void AJunMonster::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
	Super::EndAttackTraceWindow(NiagaraComponent);

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
		AWeaponActor::FAttackTraceConfig TraceConfig;
		TraceConfig.HitReactType = HitReactType;
		TraceConfig.DamageData = DamageData;
		TraceConfig.DefenseKnockbackData = DefenseKnockbackData;
		EquippedKickWeapon->StartAttackTrace(TraceConfig);
	}

	if (EquippedKickWeaponRight)
	{
		AWeaponActor::FAttackTraceConfig TraceConfig;
		TraceConfig.HitReactType = HitReactType;
		TraceConfig.DamageData = DamageData;
		TraceConfig.DefenseKnockbackData = DefenseKnockbackData;
		EquippedKickWeaponRight->StartAttackTrace(TraceConfig);
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

void AJunMonster::BeginMonsterCodeMoveWindow(const FJunMonsterCodeMoveData& CodeMoveData)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!CurrentTarget || !MovementComponent || CodeMoveData.MoveSpeed <= 0.f)
	{
		StopMonsterCodeMove(true);
		return;
	}

	FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		StopMonsterCodeMove(true);
		return;
	}

	const float StopDistance = FMath::Max(CodeMoveData.MoveStandOffDistance, CodeMoveData.StopDistance);
	if (ToTarget.Size() <= StopDistance)
	{
		StopMonsterCodeMove(true);
		return;
	}

	ActiveMonsterCodeMoveData = CodeMoveData;
	bMonsterCodeMoveActive = true;
	MonsterCodeMoveStartLocation = GetActorLocation();
	ApplyMonsterCodeMoveGroundMotionOverride(CodeMoveData.GroundMotionOverrideDuration);

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.X = MoveDirection.X * CodeMoveData.MoveSpeed;
	NewVelocity.Y = MoveDirection.Y * CodeMoveData.MoveSpeed;
	MovementComponent->Velocity = NewVelocity;

	if (CodeMoveData.bFaceTarget)
	{
		SetActorRotation(MoveDirection.Rotation());
	}
}

void AJunMonster::EndMonsterCodeMoveWindow()
{
	StopMonsterCodeMove(true);
}

// Top-level state machine

