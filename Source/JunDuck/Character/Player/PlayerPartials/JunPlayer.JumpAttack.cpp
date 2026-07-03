#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

#pragma region Jump Attack Runtime
// ============================================================================
// Jump Attack Runtime
// ============================================================================

void AJunPlayer::UpdateJumpAttackState(float DeltaTime)
{
	if (!bIsJumpAttacking)
	{
		return;
	}

	JumpAttackSectionElapsedTime += DeltaTime * GetCurrentJumpAttackTimelineRate();

	if (bJumpAttackEndRequested &&
		JumpAttackState != EJunJumpAttackState::End &&
		JumpAttackSectionElapsedTime >= JumpAttackMinStartTime)
	{
		EnterJumpAttackEnd();
	}
}

bool AJunPlayer::CanStartJumpAttack() const
{
	return AnimInstance &&
		JumpAttackMontage &&
		GetCharacterMovement() &&
		GetCharacterMovement()->IsFalling() &&
		HasEnoughAirTimeForJumpAttack() &&
		!bIsJumpAttacking &&
		!bIsBasicAttacking &&
		!bIsHeavyAttacking &&
		GetDefenseState() == EJunDefenseState::None &&
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge) &&
		!HasGameplayTag(JunGameplayTags::State_Block_Attack) &&
		!HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
}

bool AJunPlayer::HasEnoughAirTimeForJumpAttack() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return false;
	}

	if (MovementComponent->Velocity.Z < JumpAttackMinStartVerticalVelocity)
	{
		return false;
	}

	if (MovementComponent->Velocity.Z >= 0.f)
	{
		return true;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return false;
	}

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float TraceDistance = CapsuleHalfHeight + JumpAttackMinGroundDistance + 20.f;
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JumpAttackGroundCheck), false, this);
	FHitResult Hit;
	const bool bHitGround = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (!bHitGround)
	{
		return true;
	}

	const float DistanceFromCapsuleBottomToGround = FMath::Max(0.f, Hit.Distance - CapsuleHalfHeight);
	return DistanceFromCapsuleBottomToGround >= JumpAttackMinGroundDistance;
}

float AJunPlayer::GetDistanceFromGround() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return 0.f;
	}

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float TraceDistance = CapsuleHalfHeight + FMath::Max(JumpCounterEvadeMinGroundDistance + 200.f, JumpAttackMinGroundDistance + 20.f);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JumpCounterGroundCheck), false, this);
	FHitResult Hit;
	const bool bHitGround = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);

	if (!bHitGround)
	{
		return TraceDistance - CapsuleHalfHeight;
	}

	return FMath::Max(0.f, Hit.Distance - CapsuleHalfHeight);
}

void AJunPlayer::StartJumpAttack()
{
	if (!CanStartJumpAttack())
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::JumpAttack),
		[this]()
		{
			ProcessJumpAttackStart();
		});
}

void AJunPlayer::ProcessJumpAttackStart()
{
	if (!AnimInstance || !JumpAttackMontage)
	{
		return;
	}

	CancelLockOnTurn();
	TargetActor = LockOnTarget ? LockOnTarget.Get() : FindBestAttackTarget();

	FVector ForwardVelocityDirection = FVector::ZeroVector;
	if (TargetActor)
	{
		ForwardVelocityDirection = TargetActor->GetActorLocation() - GetActorLocation();
		ForwardVelocityDirection.Z = 0.f;
	}

	if (ForwardVelocityDirection.IsNearlyZero())
	{
		ForwardVelocityDirection = GetActorForwardVector();
		ForwardVelocityDirection.Z = 0.f;
	}

	ForwardVelocityDirection.Normalize();
	const FVector JumpAttackAddedVelocity =
		(ForwardVelocityDirection * FMath::Max(0.f, JumpAttackStartForwardVelocity)) +
		(FVector::UpVector * FMath::Max(0.f, JumpAttackStartUpVelocity));
	if (!JumpAttackAddedVelocity.IsNearlyZero())
	{
		LaunchCharacter(JumpAttackAddedVelocity, false, false);
	}

	AddGameplayTag(JunGameplayTags::State_Action_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJumpAttacking = true;
	bJumpAttackEndRequested = false;
	JumpAttackState = EJunJumpAttackState::Start;
	JumpAttackSectionElapsedTime = 0.f;
	CurrentJumpAttackPlayRate = FMath::Max(JumpAttackPlayRate, KINDA_SMALL_NUMBER);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
	AnimInstance->Montage_Play(JumpAttackMontage, CurrentJumpAttackPlayRate);
	AnimInstance->Montage_JumpToSection(JumpAttackStartSectionName, JumpAttackMontage);
}

void AJunPlayer::RequestJumpAttackEnd()
{
	if (!bIsJumpAttacking)
	{
		return;
	}

	bJumpAttackEndRequested = true;
	if (JumpAttackSectionElapsedTime >= JumpAttackMinStartTime)
	{
		EnterJumpAttackEnd();
	}
}

void AJunPlayer::EnterJumpAttackEnd()
{
	if (!bIsJumpAttacking || JumpAttackState == EJunJumpAttackState::End)
	{
		return;
	}

	JumpAttackState = EJunJumpAttackState::End;
	JumpAttackSectionElapsedTime = 0.f;
	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	AddGameplayTag(JunGameplayTags::State_Block_Move);

	if (AnimInstance && JumpAttackMontage)
	{
		AnimInstance->Montage_JumpToSection(JumpAttackEndSectionName, JumpAttackMontage);
	}
}

void AJunPlayer::FinishJumpAttack()
{
	EndAttackFacingWindow();

	RemoveGameplayTag(JunGameplayTags::State_Action_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	bIsJumpAttacking = false;
	bJumpAttackEndRequested = false;
	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompFollowUpAvailable = false;
	JumpCounterStompTarget = nullptr;
	JumpAttackState = EJunJumpAttackState::None;
	JumpAttackSectionElapsedTime = 0.f;
	CurrentJumpAttackPlayRate = 1.f;
	ClearActionStateIf(EJunPlayerActionState::JumpAttack, EJunActionTransitionReason::System);
}

float AJunPlayer::GetCurrentJumpAttackTimelineRate() const
{
	return FMath::Max(CurrentJumpAttackPlayRate, KINDA_SMALL_NUMBER);
}

bool AJunPlayer::CanCancelJumpAttackEndIntoMove() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::None,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule);
}

bool AJunPlayer::TryCancelJumpAttackEndIntoMove()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::None,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule))
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::CanCancelJumpAttackEndIntoDodge() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::Dodge,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelJumpAttackEndIntoDodge()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::Dodge,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction))
	{
		return false;
	}

	CancelJumpAttackForRecoveryTransition(CancelRule.BlendOutTime);
	StartDodge();
	return true;
}

bool AJunPlayer::CanCancelJumpAttackEndIntoDefense() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::Defense,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::CanCancelJumpAttackEndIntoBasicAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::BasicAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::CanCancelJumpAttackEndIntoHeavyAttack() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::CanCancelJumpAttackEndIntoJigen() const
{
	FJunPlayerCancelRule CancelRule;
	return TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::Jigen,
		CancelRule) &&
		IsDirectActionCancelRuleOpen(CancelRule) &&
		CanRequestActionState(CancelRule.ToAction);
}

bool AJunPlayer::TryCancelJumpAttackEndIntoDefense()
{
	if (!CanCancelJumpAttackEndIntoDefense())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::Defense,
		CancelRule);
	CancelJumpAttackForRecoveryTransition(CancelRule.BlendOutTime);
	BeginDefenseFromBufferedInput();
	return true;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoBasicAttack()
{
	if (!CanCancelJumpAttackEndIntoBasicAttack())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::BasicAttack,
		CancelRule);
	CancelJumpAttackForRecoveryTransition(CancelRule.BlendOutTime);
	BasicAttack();
	return true;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoHeavyAttack()
{
	if (!CanCancelJumpAttackEndIntoHeavyAttack())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::HeavyAttack,
		CancelRule);
	CancelJumpAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

bool AJunPlayer::TryCancelJumpAttackEndIntoJigen()
{
	if (!CanCancelJumpAttackEndIntoJigen())
	{
		return false;
	}

	FJunPlayerCancelRule CancelRule;
	TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpAttack,
		EJunPlayerActionState::Jigen,
		CancelRule);
	CancelJumpAttackForRecoveryTransition(CancelRule.BlendOutTime);
	return true;
}

void AJunPlayer::CancelJumpAttackForRecoveryTransition(float BlendOutTime)
{
	if (!bIsJumpAttacking)
	{
		return;
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StopAttackTrace();
	}

	if (AnimInstance && JumpAttackMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
		AnimInstance->Montage_Stop(BlendOutTime, JumpAttackMontage);
	}

	FinishJumpAttack();
}


void AJunPlayer::OnJumpAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != JumpAttackMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpAttackMontageEnded);
	}

	if (bIsJumpAttacking)
	{
		FinishJumpAttack();
	}
}

#pragma endregion