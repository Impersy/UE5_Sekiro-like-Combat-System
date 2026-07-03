#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"
#include "Kismet/KismetMathLibrary.h"
#pragma region Dodge / Dodge Attack
// ============================================================================
// Dodge / Dodge Attack
// ============================================================================

void AJunPlayer::StartDodge()
{
	StartDodgeInternal(false);
}

void AJunPlayer::StartDodgeInternal(bool bIgnoreDodgeBlockAndReleaseGate)
{
	if (CurrentHitState == EJunPlayerHitState::HitReact)
	{
		return;
	}

	if (bIsDodgeAttacking)
	{
		TryCancelDodgeAttackIntoDodge();
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (bIsHeavyAttacking)
	{
		if (!CanCancelHeavyAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Dodge))
		{
			BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction::Dodge);
			return;
		}

		CancelHeavyAttackForRecoveryTransition(HeavyAttackDodgeCancelBlendOutTime);
	}

	if (bIsJigenAttacking)
	{
		if (!CanCancelJigenIntoRecoveryAction(EJunBufferedRecoveryAction::Dodge))
		{
			BufferJigenRecoveryAction(EJunBufferedRecoveryAction::Dodge);
			return;
		}

		CancelJigenForRecoveryTransition(JigenDodgeCancelBlendOutTime);
	}

	CancelLockOnTurn();

	// 援щⅤ湲곕뒗 ?낅젰 異⑸룎????븘???곹깭 ?쒓렇瑜?癒쇱? ?좉렐 ??紐쏀?二쇰? ?쒖옉?쒕떎.
	// ?먯쑀 移대찓?쇰뒗 ?낅젰 諛⑺뼢?쇰줈 罹먮┃?곕? 癒쇱? 留욎텛怨?
	// ?쎌삩? StartDodge ?덉뿉???좏깮??諛⑺뼢蹂?紐쏀?二쇨? ?뚰뵾 諛⑺뼢??梨낆엫吏꾨떎.
	UAnimMontage* DodgeMontageToPlay = GetDodgeMontageToPlay();
	if (!DodgeMontageToPlay)
	{
		return;
	}

	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		if (bIgnoreDodgeBlockAndReleaseGate)
		{
			return;
		}

		TryStartDodgeChain();
		return;
	}

	if (!bIgnoreDodgeBlockAndReleaseGate &&
		(HasGameplayTag(JunGameplayTags::State_Block_Dodge) || !bDodgeInputReleasedSinceLastDodge))
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::Dodge),
		[this, DodgeMontageToPlay, bIgnoreDodgeBlockAndReleaseGate]()
		{
			ProcessDodgeStart(DodgeMontageToPlay, bIgnoreDodgeBlockAndReleaseGate);
		});
}

void AJunPlayer::ProcessDodgeStart(UAnimMontage* DodgeMontageToPlay, bool bIgnoreDodgeBlockAndReleaseGate)
{
	if (!AnimInstance || !DodgeMontageToPlay)
	{
		return;
	}

	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (!bLockOnActive)
	{
		AlignActorToDesiredMoveDirectionForDodge();
	}

	AddGameplayTag(JunGameplayTags::State_Action_Dodge);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	DodgeElapsedTime = 0.f;
	bDodgeInputReleasedSinceLastDodge = false;
	if (IsForwardDashMontage(DodgeMontageToPlay))
	{
		DodgeInternalCooldownRemainTime = DashForwardInternalCooldownDuration;
		DodgeFinishRemainTime = DashForwardFinishTime;
		DodgeInvincibleRemainTime = DashForwardInvincibleDuration;
	}
	else if (IsBackDashMontage(DodgeMontageToPlay))
	{
		DodgeInternalCooldownRemainTime = DashBackInternalCooldownDuration;
		DodgeFinishRemainTime = DashBackFinishTime;
		DodgeInvincibleRemainTime = DashBackInvincibleDuration;
	}
	else if (IsSideDashMontage(DodgeMontageToPlay))
	{
		DodgeInternalCooldownRemainTime = DashSideInternalCooldownDuration;
		DodgeFinishRemainTime = DashSideFinishTime;
		DodgeInvincibleRemainTime = DashSideInvincibleDuration;
	}
	else if (DodgeMontageToPlay == LockOnDodgeLeftMontage || DodgeMontageToPlay == LockOnDodgeRightMontage)
	{
		DodgeInternalCooldownRemainTime = SideDodgeInternalCooldownDuration;
		DodgeFinishRemainTime = SideDodgeFinishTime;
		DodgeInvincibleRemainTime = SideDodgeInvincibleDuration;
	}
	else
	{
		DodgeInternalCooldownRemainTime = ForwardDodgeInternalCooldownDuration;
		DodgeFinishRemainTime = ForwardDodgeFinishTime;
		DodgeInvincibleRemainTime = ForwardDodgeInvincibleDuration;
	}

	if (DodgeInvincibleRemainTime > 0.f)
	{
		AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnDodgeMontageEnded);

	CurrentDodgeMontage = DodgeMontageToPlay;
	CurrentDodgePlayRate = GetDodgePlayRateForMontage(DodgeMontageToPlay);

	bDodgeCameraAnchorProjectionActive = false;
	DodgeCameraAnchorStartLocation = FVector::ZeroVector;
	DodgeCameraAnchorDirection = FVector::ZeroVector;
	DodgeCameraAnchorProjectedDistance = 0.f;

	const bool bShouldProjectDodgeCameraAnchor =
		!IsDashDodgeMontage(DodgeMontageToPlay) &&
		(DodgeMontageToPlay == DodgeMontage ||
			DodgeMontageToPlay == LockOnDodgeBackMontage);

	if (bShouldProjectDodgeCameraAnchor && GetCapsuleComponent())
	{
		FVector DodgeDirection = GetActorForwardVector();
		if (DodgeMontageToPlay == BackDashMontage || DodgeMontageToPlay == LockOnDodgeBackMontage)
		{
			DodgeDirection = -GetActorForwardVector();
		}

		DodgeDirection.Z = 0.f;
		DodgeCameraAnchorDirection = DodgeDirection.GetSafeNormal();
		DodgeCameraAnchorStartLocation = GetCapsuleComponent()->GetComponentLocation();
		DodgeCameraAnchorProjectedDistance = 0.f;
		bDodgeCameraAnchorProjectionActive = !DodgeCameraAnchorDirection.IsNearlyZero();

		if (bShowDodgeCameraDebug)
		{
			UE_LOG(LogJun, Warning,
				TEXT("DodgeCameraDebug | Start=%s Montage=%s Chain=%d Dir=%s Start=%s Anchor=%s"),
				bDodgeCameraAnchorProjectionActive ? TEXT("Projection") : TEXT("None"),
				*GetNameSafe(DodgeMontageToPlay),
				bIgnoreDodgeBlockAndReleaseGate ? 1 : 0,
				*DodgeCameraAnchorDirection.ToCompactString(),
				*DodgeCameraAnchorStartLocation.ToCompactString(),
				CameraAnchor ? *CameraAnchor->GetComponentLocation().ToCompactString() : TEXT("None")
			);
		}

	}

	AnimInstance->Montage_Play(DodgeMontageToPlay, CurrentDodgePlayRate);
}

void AJunPlayer::UpdateDodgeState(float DeltaTime)
{
	const float DodgeTimelineDeltaTime = DeltaTime * GetCurrentDodgeTimelineRate();

	if (DodgeInvincibleRemainTime > 0.f)
	{
		DodgeInvincibleRemainTime = FMath::Max(0.f, DodgeInvincibleRemainTime - DodgeTimelineDeltaTime);

		if (DodgeInvincibleRemainTime <= 0.f)
		{
			RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
		}
	}

	if (DodgeInternalCooldownRemainTime > 0.f)
	{
		DodgeInternalCooldownRemainTime = FMath::Max(0.f, DodgeInternalCooldownRemainTime - DodgeTimelineDeltaTime);

		if (DodgeInternalCooldownRemainTime <= 0.f && !HasGameplayTag(JunGameplayTags::State_Action_Dodge))
		{
			RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
			CurrentDodgePlayRate = 1.f;
		}
	}

	if (!HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return;
	}

	DodgeElapsedTime += DodgeTimelineDeltaTime;
	const bool bDefenseHeld = DefenseComponent
		? DefenseComponent->IsDefenseButtonHeld()
		: false;
	if (bDefenseHeld && CanCancelDodgeIntoRecoveryAction())
	{
		TryCancelDodgeIntoDefense();
		return;
	}

	FJunPlayerCancelRule MoveCancelRule;
	if (TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::None,
		MoveCancelRule) &&
		IsDirectActionCancelRuleOpen(MoveCancelRule))
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Move);

		if (IsDashDodgeMontage(CurrentDodgeMontage))
		{
			const bool bHasMoveInput =
				!FMath::IsNearlyZero(DesiredMoveForward) ||
				!FMath::IsNearlyZero(DesiredMoveRight) ||
				!FMath::IsNearlyZero(PendingMoveForward) ||
				!FMath::IsNearlyZero(PendingMoveRight);
			if (bHasMoveInput && TryCancelDashDodgeIntoMove())
			{
				return;
			}
		}
	}

	if (DodgeFinishRemainTime <= 0.f)
	{
		return;
	}

	DodgeFinishRemainTime = FMath::Max(0.f, DodgeFinishRemainTime - DodgeTimelineDeltaTime);

	if (DodgeFinishRemainTime <= 0.f)
	{
		FinishDodgeState();
	}
}

void AJunPlayer::OnDodgeInputReleased()
{
	bDodgeInputReleasedSinceLastDodge = true;
}

bool AJunPlayer::TryStartDodgeChain()
{
	if (!bDodgeChainWindowActive ||
		!AnimInstance ||
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		!CurrentDodgeMontage)
	{
		return false;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
	AnimInstance->Montage_Stop(DodgeChainBlendOutTime, DodgeMontageToStop);

	FinishDodgeState();
	DodgeInternalCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	CurrentDodgePlayRate = 1.f;

	StartDodgeInternal(true);
	return HasGameplayTag(JunGameplayTags::State_Action_Dodge);
}

bool AJunPlayer::CanCancelDodgeIntoRecoveryAction() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::BasicAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule);
}

bool AJunPlayer::TryCancelDashDodgeIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!IsDashDodgeMontage(CurrentDodgeMontage) ||
		!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	if (AnimInstance && DodgeMontageToStop)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
		AnimInstance->Montage_Stop(CancelRule.BlendOutTime, DodgeMontageToStop);
	}

	FinishDodgeState();
	DodgeInternalCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	CurrentDodgePlayRate = 1.f;
	DodgeElapsedTime = 0.f;
	DesiredMoveForward = PendingMoveForward;
	DesiredMoveRight = PendingMoveRight;
	return true;
}

void AJunPlayer::CancelDodgeForRecoveryTransition(float BlendOutTime)
{
	if (!CanCancelDodgeIntoRecoveryAction())
	{
		return;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	if (AnimInstance && DodgeMontageToStop)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, DodgeMontageToStop);
	}

	FinishDodgeState();
	DodgeInternalCooldownRemainTime = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
	CurrentDodgePlayRate = 1.f;
	DodgeElapsedTime = 0.f;
}

bool AJunPlayer::TryCancelDodgeIntoBasicAttack()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::BasicAttack,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(CancelRule.BlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoHeavyAttack()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoJigen()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::Jigen,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoJump()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::Jump,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(CancelRule.BlendOutTime);
	Jump();
	return true;
}

bool AJunPlayer::TryCancelDodgeIntoDefense()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::Dodge,
		EJunPlayerActionState::Defense,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeForRecoveryTransition(CancelRule.BlendOutTime);
	BeginDefenseFromBufferedInput();
	return true;
}

bool AJunPlayer::TryStartDodgeAttack()
{
	if (!AnimInstance ||
		!DodgeAttackMontage ||
		!bDodgeAttackWindowActive ||
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) ||
		bIsDodgeAttacking)
	{
		return false;
	}

	return TryProcessActionStartRequestWithResult(
		MakeActionStartRequest(
			EJunPlayerActionState::DodgeAttack,
			EJunActionTransitionReason::CancelFromDodge),
		[this]()
		{
			return ProcessDodgeAttackStart();
		});
}

bool AJunPlayer::ProcessDodgeAttackStart()
{
	if (!AnimInstance || !DodgeAttackMontage)
	{
		return false;
	}

	UAnimMontage* DodgeMontageToStop = CurrentDodgeMontage.Get();
	if (DodgeMontageToStop)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeMontageEnded);
		AnimInstance->Montage_Stop(DodgeAttackBlendOutTime, DodgeMontageToStop);
	}

	FinishDodgeState();
	bDodgeAttackWindowActive = false;

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsDodgeAttacking = true;
	DodgeAttackElapsedTime = 0.f;
	CurrentDodgeAttackMontage = DodgeAttackMontage;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
	AnimInstance->Montage_Play(DodgeAttackMontage, FMath::Max(DodgeAttackPlayRate, KINDA_SMALL_NUMBER));
	return true;
}

void AJunPlayer::BeginDodgeAttackWindow()
{
	bDodgeAttackWindowActive = true;
}

void AJunPlayer::EndDodgeAttackWindow()
{
	bDodgeAttackWindowActive = false;
}

void AJunPlayer::BeginDodgeChainWindow()
{
	bDodgeChainWindowActive = true;
}

void AJunPlayer::EndDodgeChainWindow()
{
	bDodgeChainWindowActive = false;
}

void AJunPlayer::NotifyGuardRestartAnchorReached(UAnimMontage* Montage, float AnchorPosition)
{
	if (DefenseComponent)
	{
		DefenseComponent->NotifyGuardRestartAnchorReached(Montage, AnchorPosition);
	}
}

void AJunPlayer::StartGuardRestartBlend(UAnimMontage* Montage, float TargetPosition, float BlendDuration)
{
	if (DefenseComponent)
	{
		DefenseComponent->StartGuardRestartBlend(Montage, TargetPosition, BlendDuration);
	}
}

void AJunPlayer::UpdateGuardRestartBlend(float DeltaTime)
{
	if (DefenseComponent)
	{
		DefenseComponent->UpdateGuardRestartBlend(DeltaTime);
	}
}

void AJunPlayer::FinishGuardRestartBlend(bool bSnapToTarget)
{
	if (DefenseComponent)
	{
		DefenseComponent->FinishGuardRestartBlend(bSnapToTarget);
	}
}

void AJunPlayer::ResetGuardRestartAnchor()
{
	if (DefenseComponent)
	{
		DefenseComponent->ResetGuardRestartAnchor();
	}
}

void AJunPlayer::FinishDodgeState()
{
	if (!HasGameplayTag(JunGameplayTags::State_Action_Dodge) && !CurrentDodgeMontage)
	{
		return;
	}

	RemoveGameplayTag(JunGameplayTags::State_Action_Dodge);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	DodgeFinishRemainTime = 0.f;
	DodgeInvincibleRemainTime = 0.f;
	DodgeElapsedTime = 0.f;
	bDodgeCameraAnchorProjectionActive = false;
	DodgeCameraAnchorStartLocation = FVector::ZeroVector;
	DodgeCameraAnchorDirection = FVector::ZeroVector;
	DodgeCameraAnchorProjectedDistance = 0.f;
	RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
	if (DodgeInternalCooldownRemainTime <= 0.f)
	{
		RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
		CurrentDodgePlayRate = 1.f;
	}
	GetCharacterMovement()->bOrientRotationToMovement = !bLockOnActive;
	CurrentDodgeMontage = nullptr;
	bDodgeAttackWindowActive = false;
	bDodgeChainWindowActive = false;
	ClearActionStateIf(EJunPlayerActionState::Dodge, EJunActionTransitionReason::System);
}

void AJunPlayer::FinishDodgeAttack()
{
	EndAttackFacingWindow();

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsDodgeAttacking = false;
	DodgeAttackElapsedTime = 0.f;
	CurrentDodgeAttackMontage = nullptr;
	ClearActionStateIf(EJunPlayerActionState::DodgeAttack, EJunActionTransitionReason::System);
}

float AJunPlayer::GetCurrentDodgeAttackTimelineRate() const
{
	return FMath::Max(DodgeAttackPlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::CanCancelDodgeAttackIntoMove() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::None,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule);
}

bool AJunPlayer::TryCancelDodgeAttackIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoDodge() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::Dodge,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelDodgeAttackIntoDodge()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::Dodge,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(CancelRule.BlendOutTime);
	StartDodge();
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoBasicAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::BasicAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelDodgeAttackIntoBasicAttack()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::BasicAttack,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(CancelRule.BlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoHeavyAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::CanCancelDodgeAttackIntoJigen() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::Jigen,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelDodgeAttackIntoHeavyAttack()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::TryCancelDodgeAttackIntoJigen()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::Jigen,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelDodgeAttackIntoDefense() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::Defense,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelDodgeAttackIntoDefense()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::DodgeAttack,
		EJunPlayerActionState::Defense,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelDodgeAttackForRecoveryTransition(CancelRule.BlendOutTime);
	BeginDefenseFromBufferedInput();
	return true;
}

void AJunPlayer::CancelDodgeAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsDodgeAttacking)
	{
		return;
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	if (AnimInstance && CurrentDodgeAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, CurrentDodgeAttackMontage);
	}

	FinishDodgeAttack();
}

void AJunPlayer::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeMontage)
	{
		return;
	}

	if (bInterrupted && AnimInstance && Montage && AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	if (HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		FinishDodgeState();
	}
}

void AJunPlayer::OnDodgeAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeAttackMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnDodgeAttackMontageEnded);
	}

	if (bIsDodgeAttacking)
	{
		FinishDodgeAttack();
	}
}


void AJunPlayer::AlignActorToDesiredMoveDirectionForDodge()
{
	if (!Controller)
	{
		return;
	}

	FVector2D DesiredMoveInput(DesiredMoveForward, DesiredMoveRight);
	if (DesiredMoveInput.IsNearlyZero())
	{
		DesiredMoveInput = FVector2D(PendingMoveForward, PendingMoveRight);
	}

	if (DesiredMoveInput.IsNearlyZero())
	{
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Pitch = 0.f;
	ControlRotation.Roll = 0.f;

	const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(ControlRotation);
	const FVector RightDirection = UKismetMathLibrary::GetRightVector(ControlRotation);

	FVector DesiredWorldDirection =
		(ForwardDirection * DesiredMoveInput.X) +
		(RightDirection * DesiredMoveInput.Y);

	DesiredWorldDirection.Z = 0.f;

	if (DesiredWorldDirection.IsNearlyZero())
	{
		return;
	}

	SetActorRotation(DesiredWorldDirection.GetSafeNormal().Rotation());
}

UAnimMontage* AJunPlayer::GetDodgeMontageToPlay() const
{
	const auto ResolveForwardDashMontage = [this]() -> UAnimMontage*
	{
		return DashForwardMontage ? DashForwardMontage.Get() : DodgeMontage.Get();
	};
	const auto ResolveLeftDashMontage = [this, &ResolveForwardDashMontage]() -> UAnimMontage*
	{
		return DashLeftMontage ? DashLeftMontage.Get() : ResolveForwardDashMontage();
	};
	const auto ResolveRightDashMontage = [this, &ResolveForwardDashMontage]() -> UAnimMontage*
	{
		return DashRightMontage ? DashRightMontage.Get() : ResolveForwardDashMontage();
	};

	FVector2D DodgeInput(
		FMath::Clamp(DesiredMoveForward, -1.f, 1.f),
		FMath::Clamp(DesiredMoveRight, -1.f, 1.f)
	);

	if (DodgeInput.IsNearlyZero())
	{
		DodgeInput = FVector2D(
			FMath::Clamp(PendingMoveForward, -1.f, 1.f),
			FMath::Clamp(PendingMoveRight, -1.f, 1.f)
		);
	}

	if (DodgeInput.IsNearlyZero())
	{
		return bLockOnActive && BackDashMontage
			? BackDashMontage.Get()
			: ResolveForwardDashMontage();
	}

	if (!bLockOnActive)
	{
		return ResolveForwardDashMontage();
	}

	const float ForwardAbs = FMath::Abs(DodgeInput.X);
	const float RightAbs = FMath::Abs(DodgeInput.Y);

	constexpr float DiagonalInputThreshold = 0.25f;
	const bool bHasForwardInput = ForwardAbs >= DiagonalInputThreshold;
	const bool bHasRightInput = RightAbs >= DiagonalInputThreshold;

	if (bHasForwardInput && bHasRightInput)
	{
		if (DodgeInput.X >= 0.f)
		{
			if (DodgeInput.Y >= 0.f)
			{
				return LockOnDashForwardRightMontage ? LockOnDashForwardRightMontage.Get() : ResolveRightDashMontage();
			}

			return LockOnDashForwardLeftMontage ? LockOnDashForwardLeftMontage.Get() : ResolveLeftDashMontage();
		}

		if (DodgeInput.Y >= 0.f)
		{
			return LockOnDashBackRightMontage ? LockOnDashBackRightMontage.Get() : (BackDashMontage ? BackDashMontage.Get() : ResolveRightDashMontage());
		}

		return LockOnDashBackLeftMontage ? LockOnDashBackLeftMontage.Get() : (BackDashMontage ? BackDashMontage.Get() : ResolveLeftDashMontage());
	}

	if (RightAbs >= ForwardAbs)
	{
		return DodgeInput.Y >= 0.f
			? ResolveRightDashMontage()
			: ResolveLeftDashMontage();
	}

	return DodgeInput.X >= 0.f
		? ResolveForwardDashMontage()
		: (BackDashMontage ? BackDashMontage.Get() : ResolveForwardDashMontage());
}

bool AJunPlayer::IsDashDodgeMontage(const UAnimMontage* Montage) const
{
	return IsForwardDashMontage(Montage) ||
		IsBackDashMontage(Montage) ||
		IsSideDashMontage(Montage);
}

bool AJunPlayer::IsForwardDashMontage(const UAnimMontage* Montage) const
{
	return Montage &&
		(Montage == DashForwardMontage ||
			Montage == LockOnDashForwardLeftMontage ||
			Montage == LockOnDashForwardRightMontage);
}

bool AJunPlayer::IsBackDashMontage(const UAnimMontage* Montage) const
{
	return Montage &&
		(Montage == BackDashMontage ||
			Montage == LockOnDodgeBackMontage ||
			Montage == LockOnDashBackLeftMontage ||
			Montage == LockOnDashBackRightMontage);
}

bool AJunPlayer::IsSideDashMontage(const UAnimMontage* Montage) const
{
	return Montage &&
		(Montage == DashLeftMontage ||
			Montage == DashRightMontage);
}

float AJunPlayer::GetDodgePlayRateForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return 1.f;
	}

	if (IsForwardDashMontage(Montage))
	{
		return FMath::Max(DashForwardPlayRate, KINDA_SMALL_NUMBER);
	}

	if (IsBackDashMontage(Montage))
	{
		return FMath::Max(DashBackPlayRate, KINDA_SMALL_NUMBER);
	}

	if (IsSideDashMontage(Montage))
	{
		return FMath::Max(DashSidePlayRate, KINDA_SMALL_NUMBER);
	}

	if (Montage == LockOnDodgeLeftMontage)
	{
		return FMath::Max(LeftDodgePlayRate, KINDA_SMALL_NUMBER);
	}

	if (Montage == LockOnDodgeRightMontage)
	{
		return FMath::Max(RightDodgePlayRate, KINDA_SMALL_NUMBER);
	}

	return FMath::Max(ForwardDodgePlayRate, KINDA_SMALL_NUMBER);
}

float AJunPlayer::GetDodgeRecoveryCancelOpenTimeForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return ForwardDodgeRecoveryCancelOpenTime;
	}

	if (IsForwardDashMontage(Montage))
	{
		return DashForwardRecoveryCancelOpenTime;
	}

	if (IsBackDashMontage(Montage))
	{
		return DashBackRecoveryCancelOpenTime;
	}

	if (IsSideDashMontage(Montage))
	{
		return DashSideRecoveryCancelOpenTime;
	}

	if (Montage == LockOnDodgeLeftMontage || Montage == LockOnDodgeRightMontage)
	{
		return SideDodgeRecoveryCancelOpenTime;
	}

	return ForwardDodgeRecoveryCancelOpenTime;
}

float AJunPlayer::GetDodgeMoveCancelOpenTimeForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return ForwardDodgeMoveCancelOpenTime;
	}

	if (IsForwardDashMontage(Montage))
	{
		return DashForwardMoveCancelOpenTime;
	}

	if (IsBackDashMontage(Montage))
	{
		return DashBackMoveCancelOpenTime;
	}

	if (IsSideDashMontage(Montage))
	{
		return DashSideMoveCancelOpenTime;
	}

	if (Montage == LockOnDodgeLeftMontage || Montage == LockOnDodgeRightMontage)
	{
		return SideDodgeMoveCancelOpenTime;
	}

	return ForwardDodgeMoveCancelOpenTime;
}

float AJunPlayer::GetCurrentDodgeTimelineRate() const
{
	return FMath::Max(CurrentDodgePlayRate, KINDA_SMALL_NUMBER);
}


#pragma endregion
