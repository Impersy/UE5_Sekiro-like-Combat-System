#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

#pragma region Basic Attack Runtime
// ============================================================================
// Basic Attack Runtime
// ============================================================================

void AJunPlayer::BasicAttack()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	if (bIsDodgeAttacking)
	{
		TryCancelDodgeAttackIntoBasicAttack();
		return;
	}

	if (bIsHeavyAttacking)
	{
		TryCancelHeavyAttackIntoBasicAttack();
		return;
	}

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		StartJumpAttack();
		return;
	}

	if (!BasicAttackMontage || !AnimInstance)
	{
		return;
	}

	if (!bIsBasicAttacking)
	{
		CancelLockOnTurn();
		StartBasicAttack();
		return;
	}

	if (bBasicAttackComboAdvanceStateActive)
	{
		bBufferedBasicAttackComboInput = true;
		TryAdvanceBasicAttackCombo();
		return;
	}

	if (bCanBufferBasicAttackComboInput)
	{
		bBufferedBasicAttackComboInput = true;
		return;
	}

	if (bCanRestartBasicAttackAfterComboAdvance)
	{
		CancelBasicAttackForRecoveryTransition(BasicAttackRestartBlendOutTime);
		StartBasicAttack();
		return;
	}

	bBufferedBasicAttackInput = true;
}


void AJunPlayer::CancelBasicAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsBasicAttacking)
	{
		return;
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	if (AnimInstance && BasicAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnBasicAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, BasicAttackMontage);
	}

	FinishBasicAttack();
}

void AJunPlayer::ResetBasicAttackCombo()
{
	FinishBasicAttack();
	bBufferedBasicAttackInput = false;
}

void AJunPlayer::FinishBasicAttack()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);

	bIsBasicAttacking = false;
	bCanBufferBasicAttackComboInput = false;
	bBufferedBasicAttackComboInput = false;
	bBasicAttackComboAdvanceStateActive = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	PendingBasicAttackRecoveryActionRequest.Reset();
	PostBasicAttackJumpDodgeBufferRemainTime = PostBasicAttackJumpDodgeBufferDuration;
	PostBasicAttackDefenseBufferRemainTime = PostBasicAttackDefenseBufferDuration;
	BasicAttackSectionElapsedTime = 0.f;
	BasicAttackComboIndex = 0;
	ClearActionStateIf(EJunPlayerActionState::BasicAttack, EJunActionTransitionReason::System);
}

void AJunPlayer::StartBasicAttack()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	if (!AnimInstance || !BasicAttackMontage)
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::BasicAttack),
		[this]()
		{
			ProcessBasicAttackStart();
		});
}

void AJunPlayer::ProcessBasicAttackStart()
{
	if (!AnimInstance || !BasicAttackMontage)
	{
		return;
	}

	// 怨듦꺽 ?쒖옉 ???寃??뚯쟾 蹂댁“/?대룞 李⑤떒/肄ㅻ낫 ?곹깭瑜???踰덉뿉 ?명똿?쒕떎.
	bBufferedBasicAttackInput = false;

	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);

	bIsBasicAttacking = true;
	bCanBufferBasicAttackComboInput = false;
	bBufferedBasicAttackComboInput = false;
	bBasicAttackComboAdvanceStateActive = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	PendingBasicAttackRecoveryActionRequest.Reset();
	PostBasicAttackJumpDodgeBufferRemainTime = 0.f;
	PostBasicAttackDefenseBufferRemainTime = 0.f;
	BasicAttackSectionElapsedTime = 0.f;
	BasicAttackComboIndex = 1;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnBasicAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnBasicAttackMontageEnded);

	AnimInstance->Montage_Play(BasicAttackMontage, FMath::Max(BasicAttackPlayRate, KINDA_SMALL_NUMBER));
	AnimInstance->Montage_JumpToSection(FName("1"), BasicAttackMontage);
}

void AJunPlayer::TryAdvanceBasicAttackCombo()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	if (!bIsBasicAttacking || !bBufferedBasicAttackComboInput || !BasicAttackMontage)
	{
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	FName NextSectionName = NAME_None;

	switch (BasicAttackComboIndex)
	{
	case 1:
		NextSectionName = FName("2");
		break;
	case 2:
		NextSectionName = FName("3");
		break;
	case 3:
		NextSectionName = FName("4");
		break;
	case 4:
		bBufferedBasicAttackComboInput = false;
		bCanBufferBasicAttackComboInput = false;
		bBasicAttackComboAdvanceStateActive = false;
		bCanRestartBasicAttackAfterComboAdvance = false;
		CancelBasicAttackForRecoveryTransition(BasicAttackRestartBlendOutTime);
		StartBasicAttack();
		return;
	default:
		return;
	}

	bBufferedBasicAttackComboInput = false;
	bCanBufferBasicAttackComboInput = false;
	bBasicAttackComboAdvanceStateActive = false;
	bCanRestartBasicAttackAfterComboAdvance = false;
	BasicAttackComboIndex = BasicAttackComboIndex >= 4 ? 1 : BasicAttackComboIndex + 1;
	BasicAttackSectionElapsedTime = 0.f;

	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AnimInstance->Montage_JumpToSection(NextSectionName, BasicAttackMontage);
}


void AJunPlayer::TryProcessBufferedBasicAttackRecoveryAction()
{
	TryProcessPendingRecoveryActionRequest(PendingBasicAttackRecoveryActionRequest);
}

void AJunPlayer::UpdateBasicAttackJumpAndMoveCancelState(float DeltaTime)
{
	if (!bIsBasicAttacking)
	{
		return;
	}

	BasicAttackSectionElapsedTime += DeltaTime * FMath::Max(BasicAttackPlayRate, KINDA_SMALL_NUMBER);

	const int32 ComboArrayIndex = FMath::Max(0, BasicAttackComboIndex - 1);
	const float JumpCancelOpenTime = BasicAttackJumpCancelOpenTimes.IsValidIndex(ComboArrayIndex)
		? BasicAttackJumpCancelOpenTimes[ComboArrayIndex]
		: 0.18f;
	FJunPlayerCancelRule MoveCancelRule;
	const float MoveCancelOpenTime = TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::None,
		MoveCancelRule)
		? MoveCancelRule.OpenTime
		: JumpCancelOpenTime;

	if (BasicAttackSectionElapsedTime >= JumpCancelOpenTime)
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	}

	if (BasicAttackSectionElapsedTime >= MoveCancelOpenTime)
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	}
}

bool AJunPlayer::CanCancelBasicAttackIntoMove() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::None,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule);
}

bool AJunPlayer::TryCancelBasicAttackIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::BasicAttack,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	CancelBasicAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

void AJunPlayer::UpdateBasicAttackRecoveryBuffer(float DeltaTime)
{
	if (bIsBasicAttacking)
	{
		return;
	}

	if (PostBasicAttackJumpDodgeBufferRemainTime > 0.f)
	{
		PostBasicAttackJumpDodgeBufferRemainTime = FMath::Max(0.f, PostBasicAttackJumpDodgeBufferRemainTime - DeltaTime);
	}

	if (PostBasicAttackDefenseBufferRemainTime > 0.f)
	{
		PostBasicAttackDefenseBufferRemainTime = FMath::Max(0.f, PostBasicAttackDefenseBufferRemainTime - DeltaTime);
	}
}


void AJunPlayer::CancelBasicAttackIntoDefense()
{
	CancelBasicAttackForRecoveryTransition();
}

void AJunPlayer::OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != BasicAttackMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	ResetBasicAttackCombo();

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}
}

#pragma endregion