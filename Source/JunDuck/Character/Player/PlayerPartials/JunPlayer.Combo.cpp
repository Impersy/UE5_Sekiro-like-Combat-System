#include "Character/Player/JunPlayer.h"

#include "Animation/Notifies/Player/AnimNotifyState_AttackComboAdvance.h"
#pragma region Combo Notify Windows
// ============================================================================
// Combo Notify Windows
// ============================================================================

void AJunPlayer::OnBasicAttackComboWindowBegin()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	bCanBufferBasicAttackComboInput = true;

	if (bBufferedBasicAttackInput)
	{
		bBufferedBasicAttackComboInput = true;
		bBufferedBasicAttackInput = false;
	}
}

void AJunPlayer::OnAttackComboAdvanceStateBegin(EJunAttackComboType ComboType)
{
	if (ComboType == EJunAttackComboType::Jigen)
	{
		OnJigenComboAdvanceStateBegin();
		return;
	}

	if (ComboType == EJunAttackComboType::HeavyAttack)
	{
		OnHeavyAttackComboAdvanceStateBegin();
		return;
	}

	OnBasicAttackComboAdvanceStateBegin();
}

void AJunPlayer::OnAttackComboAdvanceStateEnd(EJunAttackComboType ComboType)
{
	if (ComboType == EJunAttackComboType::Jigen)
	{
		OnJigenComboAdvanceStateEnd();
		return;
	}

	if (ComboType == EJunAttackComboType::HeavyAttack)
	{
		OnHeavyAttackComboAdvanceStateEnd();
		return;
	}

	OnBasicAttackComboAdvanceStateEnd();
}

void AJunPlayer::OnBasicAttackComboAdvanceStateBegin()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	bBasicAttackComboAdvanceStateActive = true;
	bCanBufferBasicAttackComboInput = false;
	bCanRestartBasicAttackAfterComboAdvance = false;

	if (bBufferedBasicAttackComboInput)
	{
		TryAdvanceBasicAttackCombo();
	}
}

void AJunPlayer::OnBasicAttackComboAdvanceStateEnd()
{
	if (Is_Dead() || IsInDeathSequence())
	{
		ClearCombatInputBuffers();
		return;
	}

	const bool bWasComboAdvanceStateActive = bBasicAttackComboAdvanceStateActive;
	bBasicAttackComboAdvanceStateActive = false;
	bBufferedBasicAttackComboInput = false;
	bCanRestartBasicAttackAfterComboAdvance = bWasComboAdvanceStateActive && bIsBasicAttacking;
}

void AJunPlayer::OnHeavyAttackComboWindowBegin()
{
	if (HeavyAttackState != EJunHeavyAttackState::Tap)
	{
		return;
	}

	bCanBufferHeavyAttackComboInput = true;

	if (bBufferedHeavyAttackInput)
	{
		bBufferedHeavyAttackComboInput = true;
		bBufferedHeavyAttackInput = false;
	}
}

void AJunPlayer::OnHeavyAttackComboAdvanceStateBegin()
{
	if (HeavyAttackState != EJunHeavyAttackState::Tap)
	{
		return;
	}

	bHeavyAttackComboAdvanceStateActive = true;
	bCanBufferHeavyAttackComboInput = false;
	bCanRestartHeavyAttackAfterComboAdvance = false;

	if (bBufferedHeavyAttackComboInput)
	{
		TryAdvanceHeavyAttackCombo();
	}
}

void AJunPlayer::OnHeavyAttackComboAdvanceStateEnd()
{
	const bool bWasComboAdvanceStateActive = bHeavyAttackComboAdvanceStateActive;
	bHeavyAttackComboAdvanceStateActive = false;
	bBufferedHeavyAttackComboInput = false;
	bCanRestartHeavyAttackAfterComboAdvance =
		bWasComboAdvanceStateActive &&
		bIsHeavyAttacking &&
		HeavyAttackState == EJunHeavyAttackState::Tap;
}

void AJunPlayer::OnJigenComboWindowBegin()
{
	if (!bIsJigenAttacking)
	{
		return;
	}

	bCanBufferJigenComboInput = true;

	if (bBufferedJigenInput)
	{
		bBufferedJigenComboInput = true;
		bBufferedJigenInput = false;
	}
}

#pragma endregion
