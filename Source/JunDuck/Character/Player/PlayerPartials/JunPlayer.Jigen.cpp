#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

#pragma region Jigen Runtime
// ============================================================================
// Jigen Runtime
// ============================================================================

void AJunPlayer::OnJigenComboAdvanceStateBegin()
{
	if (!bIsJigenAttacking)
	{
		return;
	}

	bJigenComboAdvanceStateActive = true;
	bCanBufferJigenComboInput = false;

	if (bBufferedJigenComboInput)
	{
		TryAdvanceJigenCombo();
	}
}

void AJunPlayer::OnJigenComboAdvanceStateEnd()
{
	bJigenComboAdvanceStateActive = false;
	bBufferedJigenComboInput = false;
}

bool AJunPlayer::TryStartJigen()
{
	if (bIsJigenAttacking)
	{
		const bool bDefenseHeld = DefenseComponent
			? DefenseComponent->IsDefenseButtonHeld()
			: false;
		const bool bCanAcceptJigenComboInput = bDefenseHeld || JigenComboIndex <= 1;

		if (bCanAcceptJigenComboInput && bJigenComboAdvanceStateActive)
		{
			bBufferedJigenComboInput = true;
			TryAdvanceJigenCombo();
			return true;
		}

		if (bCanAcceptJigenComboInput && bCanBufferJigenComboInput)
		{
			bBufferedJigenComboInput = true;
			return true;
		}

		if (!bDefenseHeld)
		{
			if (JigenComboIndex <= 1)
			{
				bBufferedJigenInput = true;
				return true;
			}

			if (TryCancelJigenIntoBasicAttack())
			{
				return true;
			}

			return true;
		}

		bBufferedJigenInput = true;
		return true;
	}

	const bool bDefenseHeld = DefenseComponent
		? DefenseComponent->IsDefenseButtonHeld()
		: false;
	if (!bDefenseHeld)
	{
		return false;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		PendingParrySuccessActionRequest =
			MakeParrySuccessPendingActionRequest(EJunBufferedParrySuccessCancelAction::Jigen);
		TryProcessBufferedParrySuccessCancelAction();
		return true;
	}

	if (TryChooseFakeDeathDie())
	{
		return true;
	}

	if (!AnimInstance || !JigenMontage)
	{
		return false;
	}

	if (bIsBasicAttacking)
	{
		if (!TryCancelBasicAttackIntoJigen())
		{
			return false;
		}
	}

	if (bIsHeavyAttacking)
	{
		if (!TryCancelHeavyAttackIntoJigen())
		{
			return false;
		}
	}

	if (bIsJumpAttacking)
	{
		if (!TryCancelJumpAttackEndIntoJigen())
		{
			return false;
		}
	}

	if (bIsDodgeAttacking)
	{
		if (!TryCancelDodgeAttackIntoJigen())
		{
			return false;
		}
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (!TryCancelDodgeIntoJigen())
		{
			return false;
		}
	}

	if (CurrentHitState != EJunPlayerHitState::None)
	{
		return false;
	}

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return false;
	}

	if (GetDefenseState() != EJunDefenseState::None)
	{
		FinishDefenseForCancel();
	}

	StartJigen();
	return true;
}

void AJunPlayer::StartJigen()
{
	if (!AnimInstance || !JigenMontage)
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::Jigen),
		[this]()
		{
			ProcessJigenStart();
		});
}

void AJunPlayer::ProcessJigenStart()
{
	if (!AnimInstance || !JigenMontage)
	{
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJigenAttacking = true;
	JigenComboIndex = 1;
	bCanBufferJigenComboInput = false;
	bBufferedJigenComboInput = false;
	bBufferedJigenInput = false;
	bJigenComboAdvanceStateActive = false;
	JigenSectionElapsedTime = 0.f;
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJigenMontageEnded);
	AnimInstance->Montage_PlayWithBlendIn(
		JigenMontage,
		FAlphaBlendArgs(FMath::Max(0.f, JigenStartBlendInTime)),
		FMath::Max(JigenPlayRate, KINDA_SMALL_NUMBER));
	AnimInstance->Montage_JumpToSection(JigenFirstSectionName, JigenMontage);
}

void AJunPlayer::TryAdvanceJigenCombo()
{
	if (!bIsJigenAttacking || !bBufferedJigenComboInput || !JigenMontage || !AnimInstance)
	{
		return;
	}

	if (JigenComboIndex <= 1)
	{
		bBufferedJigenComboInput = false;
		bCanBufferJigenComboInput = false;
		bJigenComboAdvanceStateActive = false;
		JigenComboIndex = 2;
		JigenSectionElapsedTime = 0.f;

		TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

		const int32 SecondSectionIndex = JigenMontage->GetSectionIndex(JigenSecondSectionName);
		if (SecondSectionIndex != INDEX_NONE)
		{
			float SectionStartTime = 0.f;
			float SectionEndTime = 0.f;
			JigenMontage->GetSectionStartAndEndTime(SecondSectionIndex, SectionStartTime, SectionEndTime);

			AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
			const float MontagePlayResult = AnimInstance->Montage_PlayWithBlendIn(
				JigenMontage,
				FAlphaBlendArgs(FMath::Max(0.f, JigenComboBlendInTime)),
				FMath::Max(JigenPlayRate, KINDA_SMALL_NUMBER),
				EMontagePlayReturnType::MontageLength,
				SectionStartTime,
				true
			);
			AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJigenMontageEnded);

			if (MontagePlayResult > 0.f)
			{
				return;
			}
		}

		AnimInstance->Montage_JumpToSection(JigenSecondSectionName, JigenMontage);
		return;
	}

	bBufferedJigenComboInput = false;
	bCanBufferJigenComboInput = false;
	bJigenComboAdvanceStateActive = false;
}

void AJunPlayer::FinishJigen()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJigenAttacking = false;
	JigenComboIndex = 0;
	bCanBufferJigenComboInput = false;
	bBufferedJigenComboInput = false;
	bBufferedJigenInput = false;
	bJigenComboAdvanceStateActive = false;
	JigenSectionElapsedTime = 0.f;
	PendingJigenRecoveryActionRequest.Reset();

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}
	ClearActionStateIf(EJunPlayerActionState::Jigen, EJunActionTransitionReason::System);
}

bool AJunPlayer::CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction Action) const
{
	const FJunPendingActionRequest Request = MakeRecoveryPendingActionRequest(
		EJunPendingActionRequestSource::JigenRecovery,
		Action);
	FJunPlayerCancelRule CancelRule;
	return TryGetRecoveryCancelRule(Request, CancelRule) &&
		IsPendingActionCancelRuleOpen(Request, CancelRule) &&
		CanRequestResolvedBufferedAction(CancelRule.ToAction);
}

void AJunPlayer::BufferJigenRecoveryAction(EJunBufferedRecoveryAction Action)
{
	QueuePendingActionRequest(MakeRecoveryPendingActionRequest(
		EJunPendingActionRequestSource::JigenRecovery,
		Action));
}

void AJunPlayer::TryProcessBufferedJigenRecoveryAction()
{
	TryProcessPendingRecoveryActionRequest(PendingJigenRecoveryActionRequest);
}

void AJunPlayer::CancelJigenForRecoveryTransition(float BlendOutTime)
{
	CancelJigen(BlendOutTime);
}

bool AJunPlayer::CanCancelJigenIntoMove() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::Jigen,
		EJunPlayerActionState::None,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule);
}

bool AJunPlayer::TryCancelJigenIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Jigen,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	CancelJigenForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelJigenIntoBasicAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::Jigen,
		EJunPlayerActionState::BasicAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelJigenIntoBasicAttack()
{
	if (!CanCancelJigenIntoBasicAttack())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::Jigen,
		EJunPlayerActionState::BasicAttack,
		CancelRule);
	CancelJigenForRecoveryTransition(CancelRule.BlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::CanCancelJigenIntoHeavyAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::Jigen,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

void AJunPlayer::CancelJigen(float BlendOutTime)
{
	if (!bIsJigenAttacking)
	{
		return;
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	if (AnimInstance && JigenMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJigenMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, JigenMontage);
	}

	FinishJigen();
}

void AJunPlayer::OnJigenMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != JigenMontage)
	{
		return;
	}

	if (bInterrupted && AnimInstance && AnimInstance->Montage_IsPlaying(JigenMontage))
	{
		return;
	}

	FinishJigen();
}

#pragma endregion
