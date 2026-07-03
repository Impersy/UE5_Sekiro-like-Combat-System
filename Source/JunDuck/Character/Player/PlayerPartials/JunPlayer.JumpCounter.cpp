#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunJumpCounterTargetInterface.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"
#include "EngineUtils.h"

#pragma region Jump Counter Stomp
// ============================================================================
// Jump Counter Stomp
// ============================================================================

bool AJunPlayer::IsJumpCounterEvadeSuccessful() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	return MovementComponent &&
		MovementComponent->IsFalling() &&
		GetDistanceFromGround() >= JumpCounterEvadeMinGroundDistance;
}

bool AJunPlayer::IsValidJumpCounterStompCandidate(
	const AActor* CandidateActor,
	float OpportunityDistance,
	bool bRequireEnemy) const
{
	const IJunJumpCounterTargetInterface* JumpCounterTarget = Cast<IJunJumpCounterTargetInterface>(CandidateActor);
	if (!CandidateActor ||
		!JumpCounterTarget ||
		!JumpCounterTarget->IsJumpCounterStompThreatActive() ||
		!JumpCounterTarget->CanStartJumpCounterStompFrom(this, OpportunityDistance))
	{
		return false;
	}

	if (bRequireEnemy)
	{
		const AJunCharacter* CandidateCharacter = Cast<AJunCharacter>(CandidateActor);
		if (!CandidateCharacter || !IsEnemyTo(CandidateCharacter))
		{
			return false;
		}
	}

	return true;
}

AActor* AJunPlayer::FindBestJumpCounterStompCandidate(float OpportunityDistance) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const float MaxDistanceSq = FMath::Square(OpportunityDistance);
	AActor* BestActor = nullptr;
	float BestDistanceSq = MaxDistanceSq;
	for (TActorIterator<AJunCharacter> It(World); It; ++It)
	{
		AJunCharacter* CandidateCharacter = *It;
		if (!CandidateCharacter || CandidateCharacter == this)
		{
			continue;
		}

		if (!IsValidJumpCounterStompCandidate(CandidateCharacter, OpportunityDistance, true))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), CandidateCharacter->GetActorLocation());
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestActor = CandidateCharacter;
		}
	}

	return BestActor;
}

void AJunPlayer::UpdateJumpCounterStompOpportunity()
{
	if (CurrentHitState != EJunPlayerHitState::None)
	{
		bJumpCounterStompOpportunityPending = false;
		JumpCounterStompPendingTarget = nullptr;
		return;
	}

	if (bJumpCounterStompFollowUpAvailable ||
		bJumpCounterStompFollowUpActive ||
		!GetCharacterMovement() ||
		!GetCharacterMovement()->IsFalling())
	{
		return;
	}

	const float OpportunityDistance = FMath::Max(JumpCounterStompOpportunityDistance, JumpCounterStompMaxStartDistance);
	AActor* CandidateActor = LockOnTarget.Get();
	if (IsValidJumpCounterStompCandidate(CandidateActor, OpportunityDistance, false))
	{
		bJumpCounterStompOpportunityPending = true;
		JumpCounterStompPendingTarget = CandidateActor;
		return;
	}

	if (bJumpCounterStompOpportunityPending)
	{
		AActor* PendingActor = JumpCounterStompPendingTarget.Get();
		AJunCharacter* PendingCharacter = Cast<AJunCharacter>(PendingActor);
		const IJunJumpCounterTargetInterface* PendingJumpCounterTarget = Cast<IJunJumpCounterTargetInterface>(PendingActor);
		if (!PendingActor || !PendingJumpCounterTarget || !PendingCharacter || PendingCharacter->Is_Dead())
		{
			bJumpCounterStompOpportunityPending = false;
			JumpCounterStompPendingTarget = nullptr;
			return;
		}

		if (!PendingJumpCounterTarget->IsJumpCounterStompThreatActive())
		{
			bJumpCounterFollowUpDefenseBypassAvailable = true;
			bJumpCounterStompFollowUpAvailable = true;
			JumpCounterStompTarget = PendingActor;
			bJumpCounterStompOpportunityPending = false;
			JumpCounterStompPendingTarget = nullptr;
			return;
		}
	}

	AActor* BestActor = FindBestJumpCounterStompCandidate(OpportunityDistance);
	if (!BestActor)
	{
		return;
	}

	bJumpCounterStompOpportunityPending = true;
	JumpCounterStompPendingTarget = BestActor;
}

void AJunPlayer::UpdateJumpCounterStompFollowUp(float DeltaTime)
{
	if (!bJumpCounterStompFollowUpActive)
	{
		return;
	}

	const float CorrectionDuration = FMath::Max(JumpCounterStompCorrectionDuration, KINDA_SMALL_NUMBER);
	if (JumpCounterStompCorrectionElapsedTime < CorrectionDuration)
	{
		JumpCounterStompCorrectionElapsedTime = FMath::Min(
			JumpCounterStompCorrectionElapsedTime + DeltaTime,
			CorrectionDuration);

		const float Alpha = FMath::Clamp(JumpCounterStompCorrectionElapsedTime / CorrectionDuration, 0.f, 1.f);
		const float SmoothAlpha = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
		const FVector NewLocation = FMath::Lerp(
			JumpCounterStompCorrectionStartLocation,
			JumpCounterStompCorrectionTargetLocation,
			SmoothAlpha);

		SetActorLocation(NewLocation, true);
	}

	if (JumpCounterStompBounceMoveRemainTime > 0.f && !JumpCounterStompBounceMoveDirection.IsNearlyZero())
	{
		const float StepTime = FMath::Min(DeltaTime, JumpCounterStompBounceMoveRemainTime);
		JumpCounterStompBounceMoveRemainTime = FMath::Max(0.f, JumpCounterStompBounceMoveRemainTime - DeltaTime);

		const FVector MoveDelta = JumpCounterStompBounceMoveDirection * JumpCounterStompBounceMoveSpeed * StepTime;
		FHitResult MoveHit;
		AddActorWorldOffset(MoveDelta, true, &MoveHit);

		if (MoveHit.bBlockingHit)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("JumpCounterStompBounceBlocked | Delta=%s HitActor=%s HitComp=%s Normal=%s Location=%s"),
				*MoveDelta.ToCompactString(),
				*GetNameSafe(MoveHit.GetActor()),
				*GetNameSafe(MoveHit.GetComponent()),
				*MoveHit.ImpactNormal.ToCompactString(),
				*GetActorLocation().ToCompactString());
		}
	}
}

bool AJunPlayer::CanStartJumpCounterStompFollowUp() const
{
	const AActor* StompTargetActor = JumpCounterStompTarget.Get();
	const IJunJumpCounterTargetInterface* JumpCounterTarget = Cast<IJunJumpCounterTargetInterface>(StompTargetActor);
	return bJumpCounterStompFollowUpAvailable &&
		!bJumpCounterStompFollowUpActive &&
		JumpCounterStompFollowUpMontage &&
		AnimInstance &&
		StompTargetActor &&
		JumpCounterTarget &&
		JumpCounterTarget->CanStartJumpCounterStompFrom(this, JumpCounterStompMaxStartDistance) &&
		GetCharacterMovement() &&
		GetCharacterMovement()->IsFalling() &&
		DeathState == EJunPlayerDeathState::Alive &&
		!Is_Dead() &&
		!IsExecuting() &&
		!IsInDeathSequence() &&
		CurrentHitState == EJunPlayerHitState::None &&
		!bIsJumpAttacking &&
		!bIsDodgeAttacking &&
		!HasGameplayTag(JunGameplayTags::State_Action_Dodge);
}

FVector AJunPlayer::GetJumpCounterStompTargetLocation(const AActor* StompTargetActor) const
{
	if (!StompTargetActor)
	{
		return GetActorLocation();
	}

	if (const IJunJumpCounterTargetInterface* JumpCounterTarget = Cast<IJunJumpCounterTargetInterface>(StompTargetActor))
	{
		return JumpCounterTarget->ResolveJumpCounterStompAnchorLocation(this, GetJumpCounterStompPlacementSettings());
	}

	return StompTargetActor->GetActorLocation();
}

void AJunPlayer::SetJumpCounterStompTargetCollisionIgnored(AActor* StompTargetActor, bool bIgnore)
{
	if (!StompTargetActor)
	{
		return;
	}

	if (IJunJumpCounterTargetInterface* JumpCounterTarget = Cast<IJunJumpCounterTargetInterface>(StompTargetActor))
	{
		JumpCounterTarget->SetJumpCounterStompCollisionIgnored(this, bIgnore);
	}

	JumpCounterStompIgnoredCollisionTarget = bIgnore ? StompTargetActor : nullptr;
}

FJunJumpCounterStompPlacementSettings AJunPlayer::GetJumpCounterStompPlacementSettings() const
{
	FJunJumpCounterStompPlacementSettings Settings;
	Settings.ForwardOffset = JumpCounterStompTargetForwardOffset;
	Settings.HeightOffset = JumpCounterStompTargetHeightOffset;
	Settings.MinVisualDistance = JumpCounterStompMinVisualDistance;
	Settings.CapsuleMargin = JumpCounterStompCapsuleMargin;
	Settings.bIgnoreTargetCollisionDuringFollowUp = bJumpCounterStompIgnoreTargetCollisionDuringFollowUp;
	return Settings;
}

void AJunPlayer::RestoreJumpCounterStompTargetCollisionIgnored()
{
	AActor* IgnoredTarget = JumpCounterStompIgnoredCollisionTarget.Get();
	if (!IgnoredTarget)
	{
		return;
	}

	SetJumpCounterStompTargetCollisionIgnored(IgnoredTarget, false);
}

bool AJunPlayer::TryStartJumpCounterStompFollowUp()
{
	if (!CanStartJumpCounterStompFollowUp())
	{
		return false;
	}

	AActor* StompTargetActor = JumpCounterStompTarget.Get();
	if (!StompTargetActor)
	{
		return false;
	}

	return TryProcessActionStartRequestWithResult(
		MakeActionStartRequest(EJunPlayerActionState::JumpCounterStomp),
		[this, StompTargetActor]()
		{
			return ProcessJumpCounterStompFollowUpStart(StompTargetActor);
		});
}

bool AJunPlayer::ProcessJumpCounterStompFollowUpStart(AActor* StompTargetActor)
{
	if (!StompTargetActor || !AnimInstance || !JumpCounterStompFollowUpMontage)
	{
		return false;
	}

	InterruptActionsForHitReaction();
	CancelLockOnTurn();

	bJumpCounterStompFollowUpActive = true;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterFollowUpDefenseBypassAvailable = true;
	AddGameplayTag(JunGameplayTags::State_Condition_Invincible);
	bJumpCounterStompInvincibilityActive = true;
	if (bJumpCounterStompIgnoreTargetCollisionDuringFollowUp)
	{
		SetJumpCounterStompTargetCollisionIgnored(StompTargetActor, true);
	}
	JumpCounterStompCorrectionElapsedTime = 0.f;
	JumpCounterStompCorrectionStartLocation = GetActorLocation();
	JumpCounterStompCorrectionTargetLocation = GetJumpCounterStompTargetLocation(StompTargetActor);

	AddGameplayTag(JunGameplayTags::State_Block_Move);
	AddGameplayTag(JunGameplayTags::State_Block_Jump);
	AddGameplayTag(JunGameplayTags::State_Block_Attack);
	AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	DesiredMoveForward = 0.f;
	DesiredMoveRight = 0.f;
	PendingMoveForward = 0.f;
	PendingMoveRight = 0.f;
	ConsumeMovementInputVector();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpCounterStompFollowUpMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AJunPlayer::OnJumpCounterStompFollowUpMontageEnded);
	const float PlayedDuration = AnimInstance->Montage_Play(
		JumpCounterStompFollowUpMontage,
		FMath::Max(JumpCounterStompPlayRate, KINDA_SMALL_NUMBER));
	if (PlayedDuration <= 0.f)
	{
		FinishJumpCounterStompFollowUp(false);
		return false;
	}

	return true;
}

bool AJunPlayer::TryCancelJumpCounterStompFollowUpIntoJumpAttack()
{
	FJunPlayerCancelRule CancelRule;
	if (!TryGetDirectActionCancelRule(
		EJunPlayerActionState::JumpCounterStomp,
		EJunPlayerActionState::JumpAttack,
		CancelRule) ||
		!IsDirectActionCancelRuleOpen(CancelRule) ||
		!CanRequestActionState(CancelRule.ToAction) ||
		!JumpAttackMontage ||
		!AnimInstance ||
		!GetCharacterMovement() ||
		!GetCharacterMovement()->IsFalling() ||
		bIsJumpAttacking ||
		DeathState != EJunPlayerDeathState::Alive ||
		Is_Dead())
	{
		return false;
	}

	if (AnimInstance && JumpCounterStompFollowUpMontage)
	{
		AnimInstance->Montage_Stop(
			FMath::Max(0.f, CancelRule.BlendOutTime),
			JumpCounterStompFollowUpMontage);
	}

	FinishJumpCounterStompFollowUp(false);
	StartJumpAttack();
	return bIsJumpAttacking;
}

void AJunPlayer::BeginJumpCounterStompJumpAttackWindow()
{
	if (bJumpCounterStompFollowUpActive)
	{
		bJumpCounterStompJumpAttackWindowOpen = true;
	}
}

void AJunPlayer::EndJumpCounterStompJumpAttackWindow()
{
	bJumpCounterStompJumpAttackWindowOpen = false;
}

void AJunPlayer::ApplyJumpCounterStompBounce(float UpVelocity, float BackwardVelocity)
{
	ApplyJumpCounterStompBounce(UpVelocity, BackwardVelocity, 0.15f);
}

void AJunPlayer::ApplyJumpCounterStompBounce(float UpVelocity, float BackwardVelocity, float BackwardMoveDuration)
{
	if (!bJumpCounterStompFollowUpActive || !GetCharacterMovement())
	{
		return;
	}

	JumpCounterStompCorrectionElapsedTime = FMath::Max(JumpCounterStompCorrectionDuration, 0.f);
	if (!JumpCounterStompCorrectionTargetLocation.IsNearlyZero())
	{
		FHitResult SnapHit;
		SetActorLocation(JumpCounterStompCorrectionTargetLocation, true, &SnapHit);
		if (SnapHit.bBlockingHit)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("JumpCounterStompSnapBlocked | Target=%s Actual=%s HitActor=%s HitComp=%s Normal=%s"),
				*JumpCounterStompCorrectionTargetLocation.ToCompactString(),
				*GetActorLocation().ToCompactString(),
				*GetNameSafe(SnapHit.GetActor()),
				*GetNameSafe(SnapHit.GetComponent()),
				*SnapHit.ImpactNormal.ToCompactString());
		}
	}

	AActor* JumpCounterTargetActor = JumpCounterStompTarget.Get();
	const FVector BounceReferenceLocation = JumpCounterTargetActor
		? JumpCounterTargetActor->GetActorLocation()
		: (LockOnTarget ? LockOnTarget->GetActorLocation() : GetActorLocation() + GetActorForwardVector());
	FVector BounceAwayDirection = (GetActorLocation() - BounceReferenceLocation).GetSafeNormal2D();
	if (JumpCounterTargetActor && BounceAwayDirection.IsNearlyZero())
	{
		BounceAwayDirection = -JumpCounterTargetActor->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector SafeBounceAwayDirection = BounceAwayDirection.IsNearlyZero()
		? -GetActorForwardVector().GetSafeNormal2D()
		: BounceAwayDirection;
	const FVector BounceVelocity =
		SafeBounceAwayDirection * FMath::Max(0.f, BackwardVelocity) +
		FVector::UpVector * UpVelocity;

	JumpCounterStompBounceMoveDirection = SafeBounceAwayDirection;
	JumpCounterStompBounceMoveSpeed = FMath::Max(0.f, BackwardVelocity);
	JumpCounterStompBounceMoveRemainTime = FMath::Max(0.f, BackwardMoveDuration);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->Velocity.X = BounceVelocity.X;
		MovementComponent->Velocity.Y = BounceVelocity.Y;
	}

	UE_LOG(
		LogJun,
		Warning,
		TEXT("JumpCounterStompBounceApply | Loc=%s Ref=%s Dir=%s Velocity=%s CodeMoveSpeed=%.1f CodeMoveDuration=%.3f Active=%d"),
		*GetActorLocation().ToCompactString(),
		*BounceReferenceLocation.ToCompactString(),
		*SafeBounceAwayDirection.ToCompactString(),
		*BounceVelocity.ToCompactString(),
		JumpCounterStompBounceMoveSpeed,
		JumpCounterStompBounceMoveRemainTime,
		bJumpCounterStompFollowUpActive ? 1 : 0);

	LaunchCharacter(BounceVelocity, true, true);
}

void AJunPlayer::ApplyJumpCounterStompHit(
	EHitReactType HitReactType,
	const FJunAttackDamageData& DamageData,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
	AActor* StompTargetActor = JumpCounterStompTarget.Get();
	IJunJumpCounterTargetInterface* JumpCounterTarget = Cast<IJunJumpCounterTargetInterface>(StompTargetActor);
	if (!bJumpCounterStompFollowUpActive || !StompTargetActor || !JumpCounterTarget)
	{
		return;
	}

	JumpCounterTarget->ApplyJumpCounterStompResult(
		this,
		HitReactType,
		DamageData,
		DefenseKnockbackData,
		JumpCounterStompPostureGain);
}

void AJunPlayer::FinishJumpCounterStompFollowUp(bool bApplyBounce)
{
	if (AnimInstance && JumpCounterStompFollowUpMontage)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnJumpCounterStompFollowUpMontageEnded);
	}

	bJumpCounterStompFollowUpActive = false;
	bJumpCounterStompFollowUpAvailable = false;
	bJumpCounterFollowUpDefenseBypassAvailable = false;
	bJumpCounterStompOpportunityPending = false;
	bJumpCounterStompJumpAttackWindowOpen = false;
	if (bJumpCounterStompInvincibilityActive)
	{
		RemoveGameplayTag(JunGameplayTags::State_Condition_Invincible);
		bJumpCounterStompInvincibilityActive = false;
	}
	JumpCounterStompCorrectionElapsedTime = 0.f;
	JumpCounterStompCorrectionStartLocation = FVector::ZeroVector;
	JumpCounterStompCorrectionTargetLocation = FVector::ZeroVector;
	JumpCounterStompBounceMoveRemainTime = 0.f;
	JumpCounterStompBounceMoveDirection = FVector::ZeroVector;
	JumpCounterStompBounceMoveSpeed = 0.f;
	RestoreJumpCounterStompTargetCollisionIgnored();
	JumpCounterStompPendingTarget = nullptr;
	JumpCounterStompTarget = nullptr;
	ClearActionStateIf(EJunPlayerActionState::JumpCounterStomp, EJunActionTransitionReason::System);

	RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);

	if (bApplyBounce)
	{
		ApplyJumpCounterStompBounce(JumpCounterStompBounceUpVelocity, JumpCounterStompBounceBackwardVelocity);
	}
}


void AJunPlayer::OnJumpCounterStompFollowUpMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != JumpCounterStompFollowUpMontage)
	{
		return;
	}

	FinishJumpCounterStompFollowUp(false);
}

#pragma endregion