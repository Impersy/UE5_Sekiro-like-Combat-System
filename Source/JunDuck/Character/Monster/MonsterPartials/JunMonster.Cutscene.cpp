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
	AActor* FacingTarget = CurrentExecutionInstigator.Get();
	if (!FacingTarget && bExecutionReady)
	{
		FacingTarget = CurrentTarget.Get();
	}

	if (!FacingTarget)
	{
		EndAttackFacingWindow();
		return;
	}

	FVector ToExecutor = FacingTarget->GetActorLocation() - GetActorLocation();
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
	ExecutionInputUnlockRealTime = 0.f;
	CurrentExecutionInstigator = nullptr;
	CurrentExecutionMontage = nullptr;
	CurrentPosture = 0.f;
	EndAttackFacingWindow();
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	if (UAnimInstance* MonsterAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		MonsterAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunMonster::OnExecutionMontageEnded);
	}
	GetWorldTimerManager().ClearTimer(ExecutionRecoveryTimerHandle);
}

