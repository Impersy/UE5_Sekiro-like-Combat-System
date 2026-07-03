#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerPotionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunTutorialTargetInterface.h"
#include "JunGameplayTags.h"
#include "EngineUtils.h"
#pragma region Damage / Potion
// ============================================================================
// Damage / Potion
// ============================================================================

void AJunPlayer::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	if (Damage <= 0 || DeathState != EJunPlayerDeathState::Alive)
	{
		return;
	}

	const int32 PreviousHp = Hp;
	Hp = FMath::Clamp(Hp - Damage, 0, MaxHp);
	if (PreviousHp > Hp)
	{
		PlayHitDamageSound();
		FinishDrinkPotion(true);
	}

	if (Hp <= 0)
	{
		for (TActorIterator<AJunCharacter> It(GetWorld()); It; ++It)
		{
			const IJunTutorialTargetInterface* TutorialTarget = Cast<IJunTutorialTargetInterface>(*It);
			if (TutorialTarget && TutorialTarget->ShouldPreventPlayerDeathForTutorial(this))
			{
				Hp = FMath::Clamp(1, 1, MaxHp);
				return;
			}
		}
	}

	if (Hp > 0)
	{
		return;
	}

	if (CurrentReviveCount > 0)
	{
		EnterFakeDeath();
		return;
	}

	EnterRealDeath();
}

bool AJunPlayer::TryStartDrinkPotion()
{
	if (!CanStartDrinkPotion())
	{
		return false;
	}

	return TryProcessActionStartRequestWithResult(
		MakeActionStartRequest(EJunPlayerActionState::DrinkingPotion),
		[this]()
		{
			return ProcessDrinkPotionStart();
		});
}

bool AJunPlayer::ProcessDrinkPotionStart()
{
	UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance();
	if (!PlayerAnimInstance || !DrinkMontage)
	{
		ClearActionStateIf(EJunPlayerActionState::DrinkingPotion, EJunActionTransitionReason::System);
		return false;
	}

	if (GetDefenseState() != EJunDefenseState::None)
	{
		FinishDefenseForCancel();
	}

	PotionComponent->BeginDrink(bWalkRequested);
	SetWalkRequested(true);
	HideWeaponForDrinkPotion();

	PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDrinkPotionMontageEnded);
	PlayerAnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnDrinkPotionMontageEnded);

	const float PlayResult = PlayerAnimInstance->Montage_PlayWithBlendIn(
		DrinkMontage,
		FAlphaBlendArgs(FMath::Max(0.f, DrinkPotionBlendInTime)),
		FMath::Max(DrinkPotionPlayRate, KINDA_SMALL_NUMBER));
	if (PlayResult <= 0.f)
	{
		FinishDrinkPotion(true);
		return false;
	}

	if (!PotionComponent->ConsumeCharge())
	{
		FinishDrinkPotion(true);
		return false;
	}
	PendingDrinkPotionTutorialTarget = LockOnTarget.Get();
	ApplyDrinkPotionHeal();
	return true;
}

void AJunPlayer::SetHpForTutorial(int32 NewHp)
{
	Hp = FMath::Clamp(NewHp, 1, FMath::Max(1, MaxHp));
}

void AJunPlayer::SetPostureForTutorial(float NewPosture)
{
	CurrentPosture = FMath::Clamp(NewPosture, 0.f, FMath::Max(0.f, MaxPosture));
	PostureRecoveryDelayRemainTime = 0.f;
}

void AJunPlayer::RestoreResourcesAfterTutorial()
{
	CancelDrinkPotionHeal();
	Hp = FMath::Max(1, MaxHp);
	RefillDrinkPotionCharges();
}

bool AJunPlayer::CanStartDrinkPotion() const
{
	if (!DrinkMontage || !PotionComponent || !PotionComponent->HasCharges())
	{
		return false;
	}

	if (IsDrinkingPotion() ||
		DeathState != EJunPlayerDeathState::Alive ||
		Is_Dead() ||
		IsExecuting() ||
		IsInDeathSequence() ||
		CurrentHitState != EJunPlayerHitState::None)
	{
		return false;
	}

	if (bIsBasicAttacking ||
		bIsHeavyAttacking ||
		bIsJigenAttacking ||
		bIsJumpAttacking ||
		bIsDodgeAttacking ||
		HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked) ||
		HasGameplayTag(JunGameplayTags::State_Block_Input))
	{
		return false;
	}

	const EJunDefenseState CurrentDefenseState = GetDefenseState();
	if (CurrentDefenseState == EJunDefenseState::Starting ||
		CurrentDefenseState == EJunDefenseState::Guarding)
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent && MovementComponent->IsFalling())
	{
		return false;
	}

	return true;
}

void AJunPlayer::RefillDrinkPotionCharges()
{
	if (PotionComponent)
	{
		PotionComponent->RefillCharges(MaxDrinkPotionCharges);
	}
}

void AJunPlayer::ApplyDrinkPotionHeal()
{
	if (!PotionComponent)
	{
		return;
	}

	if (PotionComponent->StartHeal(Hp, MaxHp, DrinkPotionHealRatio, DrinkPotionHealDuration))
	{
		CompleteDrinkPotionHeal();
	}
}

void AJunPlayer::UpdateDrinkPotionHeal(float DeltaTime)
{
	if (PotionComponent && PotionComponent->UpdateHeal(DeltaTime, Hp, MaxHp))
	{
		CompleteDrinkPotionHeal();
	}
}

void AJunPlayer::CompleteDrinkPotionHeal()
{
	if (IJunTutorialTargetInterface* TutorialTarget = Cast<IJunTutorialTargetInterface>(PendingDrinkPotionTutorialTarget.Get()))
	{
		TutorialTarget->NotifyPlayerDrinkPotionForTutorial(this);
	}

	PendingDrinkPotionTutorialTarget.Reset();
}

void AJunPlayer::CancelDrinkPotionHeal()
{
	PendingDrinkPotionTutorialTarget.Reset();
	if (PotionComponent)
	{
		PotionComponent->CancelHeal();
	}
}

void AJunPlayer::HideWeaponForDrinkPotion()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->HideWeaponForDrinkPotion();
	}
}

void AJunPlayer::RestoreDrinkPotionWeaponVisibility()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->RestoreWeaponAfterDrinkPotion();
	}
}

void AJunPlayer::FinishDrinkPotion(bool bInterrupted)
{
	if (bInterrupted)
	{
		CancelDrinkPotionHeal();
	}

	const bool bWeaponHiddenForDrinkPotion = EquipmentComponent && EquipmentComponent->IsWeaponHiddenForDrinkPotion();
	if (!IsDrinkingPotion() && !bWeaponHiddenForDrinkPotion)
	{
		return;
	}

	if (UAnimInstance* PlayerAnimInstance = GetPlayerAnimInstance())
	{
		PlayerAnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDrinkPotionMontageEnded);
		if (bInterrupted && DrinkMontage && PlayerAnimInstance->Montage_IsPlaying(DrinkMontage))
		{
			PlayerAnimInstance->Montage_Stop(FMath::Max(0.f, DrinkPotionInterruptBlendOutTime), DrinkMontage);
		}
	}

	RestoreDrinkPotionWeaponVisibility();
	bool bSavedWalkRequested = false;
	const bool bWasDrinking = PotionComponent && PotionComponent->EndDrink(bSavedWalkRequested);
	if (bWasDrinking)
	{
		SetWalkRequested(bSavedWalkRequested);
	}
	ClearActionStateIf(EJunPlayerActionState::DrinkingPotion, EJunActionTransitionReason::System);
}

void AJunPlayer::OnDrinkPotionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != DrinkMontage)
	{
		return;
	}

	FinishDrinkPotion(bInterrupted);
}


#pragma endregion
