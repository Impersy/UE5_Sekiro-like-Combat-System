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
	SetMonsterWorldHUDRevealed(true);
	if (CurrentPosture >= MaxPosture)
	{
		if (ShouldShowBossCombatHUD())
		{
			if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
			{
				JunPlayerController->PlayBossPostureBreakGlow();
			}
		}
		else if (MonsterHUDWidgetComponent)
		{
			if (UJunMonsterHUDWidget* MonsterHUDWidget = Cast<UJunMonsterHUDWidget>(MonsterHUDWidgetComponent->GetWidget()))
			{
				MonsterHUDWidget->PlayMonsterPostureBreakGlow();
			}
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

	const bool bTargetMaintainingAttackPosturePressure = IsTargetMaintainingAttackPosturePressure();
	if ((bIsAttacking && bTargetMaintainingAttackPosturePressure) || IsInHitReact())
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked) &&
		(!bIsAttacking || bTargetMaintainingAttackPosturePressure))
	{
		return false;
	}

	return true;
}

bool AJunMonster::IsTargetMaintainingAttackPosturePressure() const
{
	if (!bIsAttacking || !CurrentTarget || AttackPosturePressureDistance <= 0.f)
	{
		return false;
	}

	const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistanceSq > FMath::Square(AttackPosturePressureDistance))
	{
		return false;
	}

	const AJunPlayer* TargetPlayer = Cast<AJunPlayer>(CurrentTarget);
	if (!TargetPlayer)
	{
		return true;
	}

	return TargetPlayer->IsBasicAttacking() ||
		TargetPlayer->IsHeavyAttacking() ||
		TargetPlayer->IsJigenAttacking() ||
		TargetPlayer->IsJumpAttacking() ||
		TargetPlayer->IsDodgeAttacking() ||
		TargetPlayer->IsInParrySuccess() ||
		TargetPlayer->IsGuardBlockReacting() ||
		TargetPlayer->IsParryWindowOpen();
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

