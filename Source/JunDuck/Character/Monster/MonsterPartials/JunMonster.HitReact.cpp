#include "Character/Monster/JunMonster.h"

#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	bool IsHeavyHitType(const EHitReactType HitType)
	{
		return HitType == EHitReactType::HeavyHit_A
			|| HitType == EHitReactType::HeavyHit_B
			|| HitType == EHitReactType::HeavyHit_C;
	}

	bool IsLargeHitType(const EHitReactType HitType)
	{
		return HitType == EHitReactType::LargeHit_Short
			|| HitType == EHitReactType::LargeHit_Long;
	}

	bool IsLightingShockHitType(const EHitReactType HitType)
	{
		return HitType == EHitReactType::Lighting_Shock;
	}

	bool IsFrontHitReactDirection(const ECharacterHitReactDirection HitDirection)
	{
		return HitDirection == ECharacterHitReactDirection::Front_F ||
			HitDirection == ECharacterHitReactDirection::Front_FL ||
			HitDirection == ECharacterHitReactDirection::Front_FR;
	}

	bool DoesMontageUseRootMotion(const UAnimMontage* Montage)
	{
		return Montage && Montage->HasRootMotion();
	}

	ECharacterHitReactDirection ResolveHitReactDirectionFromSwing(const AActor& CharacterActor, const FVector& SwingDirection)
	{
		const FVector SafeSwingDirection = SwingDirection.GetSafeNormal();
		if (SafeSwingDirection.IsNearlyZero())
		{
			return ECharacterHitReactDirection::Front_F;
		}

		const FVector LocalSwingDirection = CharacterActor.GetActorTransform().InverseTransformVectorNoScale(SafeSwingDirection);
		const float YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalSwingDirection.Y, LocalSwingDirection.X));

		if (YawDegrees >= -25.f && YawDegrees <= 25.f)
		{
			return ECharacterHitReactDirection::Front_F;
		}
		if (YawDegrees > 25.f && YawDegrees <= 70.f)
		{
			return ECharacterHitReactDirection::Front_FR;
		}
		if (YawDegrees > 70.f && YawDegrees <= 135.f)
		{
			return ECharacterHitReactDirection::Right_R;
		}
		if (YawDegrees < -25.f && YawDegrees >= -70.f)
		{
			return ECharacterHitReactDirection::Front_FL;
		}
		if (YawDegrees < -70.f && YawDegrees >= -135.f)
		{
			return ECharacterHitReactDirection::Left_L;
		}

		return ECharacterHitReactDirection::Back_B;
	}

	UAnimMontage* ResolveDirectionalHitReactMontage(
		const ECharacterHitReactDirection HitDirection,
		UAnimMontage* BackMontage,
		UAnimMontage* FrontMontage,
		UAnimMontage* FrontLeftMontage,
		UAnimMontage* FrontRightMontage,
		UAnimMontage* LeftMontage,
		UAnimMontage* RightMontage)
	{
		switch (HitDirection)
		{
		case ECharacterHitReactDirection::Back_B:
			return BackMontage ? BackMontage : FrontMontage;
		case ECharacterHitReactDirection::Front_FL:
			return FrontLeftMontage ? FrontLeftMontage : (LeftMontage ? LeftMontage : FrontMontage);
		case ECharacterHitReactDirection::Front_FR:
			return FrontRightMontage ? FrontRightMontage : (RightMontage ? RightMontage : FrontMontage);
		case ECharacterHitReactDirection::Left_L:
			return LeftMontage ? LeftMontage : (FrontLeftMontage ? FrontLeftMontage : FrontMontage);
		case ECharacterHitReactDirection::Right_R:
			return RightMontage ? RightMontage : (FrontRightMontage ? FrontRightMontage : FrontMontage);
		case ECharacterHitReactDirection::Front_F:
		default:
			return FrontMontage;
		}
	}
}

void AJunMonster::StartHitReact(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection)
{
	// 피격 리액션은 "상태 태그 잠금 + AI 이동 정지 + 방향별 몽타주 재생"으로 시작한다.
	// 이 동안 일반 상태머신은 멈추고, Duration이 끝나면 다시 원래 AI 상태 갱신으로 돌아간다.
	const bool bRestartingHitReact = IsInHitReact();
	CurrentHitReact = NewHitReact;
	CurrentHitReactDirection = NewHitDirection;
	HitReactTime = 0.f;
	CurrentHitReactDuration = GetHitReactDuration(NewHitReact);
	CurrentHitReactControlLockRemainTime = GetHitReactControlLockDuration(NewHitReact);

	if (bIsAttacking)
	{
		StopMonsterCodeMove(true);
		bAttackInterruptedByHitReact = true;
		bIsAttacking = false;
		AttackTime = 0.f;
		AttackFacingRemainTime = 0.f;
		ResetCurrentAttackRuntimeState();
	}

	RemoveGameplayTag(JunGameplayTags::State_Condition_SuperArmor);
	AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);

	CancelCombatTurn();
	StopAIMovement();
	StopAllAttackTraces();
	OnHitReactStarted(NewHitReact, NewHitDirection);

	if (UAnimMontage* HitReactMontage = GetHitReactMontage(NewHitReact, NewHitDirection))
	{
		if (IsHeavyHitType(NewHitReact))
		{
			CurrentHitReactDuration = HitReactMontage->GetPlayLength();
		}

		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const float BlendInTime = bRestartingHitReact ? HitReactRestartBlendInTime : HitReactBlendInTime;
			const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, BlendInTime));
			if (AnimInstance->Montage_PlayWithBlendSettings(HitReactMontage, BlendInSettings) <= 0.f)
			{
				PlayAnimMontage(HitReactMontage);
			}
		}
		else
		{
			PlayAnimMontage(HitReactMontage);
		}
	}
}

void AJunMonster::UpdateHitReact(float DeltaTime)
{
	if (!IsInHitReact())
	{
		return;
	}

	// 현재는 몽타주 종료가 아니라 Duration 기준으로 HitReact 상태를 끝낸다.
	HitReactTime += DeltaTime;

	if (CurrentHitReactControlLockRemainTime > 0.f)
	{
		CurrentHitReactControlLockRemainTime = FMath::Max(0.f, CurrentHitReactControlLockRemainTime - DeltaTime);
		if (CurrentHitReactControlLockRemainTime <= 0.f)
		{
			ReleaseHitReactControlLock();
		}
	}

	if (HitReactTime >= CurrentHitReactDuration)
	{
		EndHitReact();
	}
}

void AJunMonster::UpdateHitReactFacing(float DeltaTime)
{
	if (!bHitReactFacingWindowActive)
	{
		return;
	}

	if (!HitReactFacingTarget)
	{
		EndHitReactFacingWindow();
		return;
	}

	FVector Direction = HitReactFacingTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const float ResolvedInterpSpeed = HitReactFacingWindowInterpSpeed > 0.f ? HitReactFacingWindowInterpSpeed : AttackFacingInterpSpeed;
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		Direction.Rotation(),
		DeltaTime,
		ResolvedInterpSpeed
	));
}

void AJunMonster::EndHitReact()
{
	// HitReact 종료는 태그 해제만 담당하고,
	// 이후 AI 상태 갱신은 CanUpdateBehavior()가 다시 허용한다.
	const EHitReactType EndedHitReact = CurrentHitReact;
	CurrentHitReact = EHitReactType::None;
	CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	HitReactFacingTarget = nullptr;
	EndHitReactFacingWindow();
	HitReactTime = 0.f;
	CurrentHitReactDuration = 0.f;
	CurrentHitReactControlLockRemainTime = 0.f;

	RemoveGameplayTag(JunGameplayTags::State_Condition_HitReact);
	ReleaseHitReactControlLock();
	OnHitReactEnded(EndedHitReact);
}

void AJunMonster::ReleaseHitReactControlLock()
{
	RemoveGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
}

ECharacterKnockbackDirection AJunMonster::DetermineKnockbackDirectionFromDamageCauser(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return ECharacterKnockbackDirection::Backward;
	}

	const FVector ToAttackerWorld = DamageCauser->GetActorLocation() - GetActorLocation();
	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(ToAttackerWorld);
	ToAttackerLocal.Z = 0.f;

	if (ToAttackerLocal.IsNearlyZero())
	{
		return ECharacterKnockbackDirection::Backward;
	}

	if (FMath::Abs(ToAttackerLocal.Y) > FMath::Abs(ToAttackerLocal.X))
	{
		return ToAttackerLocal.Y > 0.f
			? ECharacterKnockbackDirection::Left
			: ECharacterKnockbackDirection::Right;
	}

	return ToAttackerLocal.X > 0.f
		? ECharacterKnockbackDirection::Backward
		: ECharacterKnockbackDirection::Forward;
}

void AJunMonster::ApplyHitReactKnockback(AActor* DamageCauser, const FJunDefenseKnockbackData& KnockbackData, UAnimMontage* HitReactMontage)
{
	if (KnockbackData.Strength <= 0.f || DoesMontageUseRootMotion(HitReactMontage))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	FVector KnockbackDirection = FVector::ZeroVector;
	switch (DetermineKnockbackDirectionFromDamageCauser(DamageCauser))
	{
	case ECharacterKnockbackDirection::Forward:
		KnockbackDirection = GetActorForwardVector();
		break;
	case ECharacterKnockbackDirection::Backward:
		KnockbackDirection = -GetActorForwardVector();
		break;
	case ECharacterKnockbackDirection::Left:
		KnockbackDirection = -GetActorRightVector();
		break;
	case ECharacterKnockbackDirection::Right:
		KnockbackDirection = GetActorRightVector();
		break;
	default:
		KnockbackDirection = -GetActorForwardVector();
		break;
	}

	KnockbackDirection.Z = 0.f;
	KnockbackDirection = KnockbackDirection.GetSafeNormal();
	if (KnockbackDirection.IsNearlyZero())
	{
		return;
	}

	HitReactKnockbackBrakingDecelerationOverride = KnockbackData.BrakingDeceleration;
	HitReactKnockbackGroundFrictionOverride = KnockbackData.GroundFriction;
	HitReactKnockbackBrakingFrictionFactorOverride = KnockbackData.BrakingFrictionFactor;
	HitReactKnockbackBrakingOverrideRemainTime = KnockbackData.OverrideDuration;
	bHitReactKnockbackBrakingOverrideActive = true;

	MovementComponent->AddImpulse(KnockbackDirection * KnockbackData.Strength, true);
}

void AJunMonster::UpdateHitReactKnockbackBraking(float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (HitReactKnockbackBrakingOverrideRemainTime > 0.f)
	{
		HitReactKnockbackBrakingOverrideRemainTime = FMath::Max(0.f, HitReactKnockbackBrakingOverrideRemainTime - DeltaTime);
		MovementComponent->BrakingDecelerationWalking = HitReactKnockbackBrakingDecelerationOverride;
		MovementComponent->GroundFriction = HitReactKnockbackGroundFrictionOverride;
		MovementComponent->BrakingFrictionFactor = HitReactKnockbackBrakingFrictionFactorOverride;

		if (HitReactKnockbackBrakingOverrideRemainTime <= 0.f)
		{
			RestoreDefaultMonsterMovementBrakingSettings();
			bHitReactKnockbackBrakingOverrideActive = false;
		}

		return;
	}

	if (bHitReactKnockbackBrakingOverrideActive)
	{
		RestoreDefaultMonsterMovementBrakingSettings();
		bHitReactKnockbackBrakingOverrideActive = false;
	}
}

void AJunMonster::RestoreDefaultMonsterMovementBrakingSettings()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->BrakingDecelerationWalking = DefaultMonsterBrakingDecelerationWalking;
		MovementComponent->GroundFriction = DefaultMonsterGroundFriction;
		MovementComponent->BrakingFrictionFactor = DefaultMonsterBrakingFrictionFactor;
	}
}

void AJunMonster::OnHitReactStarted(EHitReactType NewHitReact, ECharacterHitReactDirection NewHitDirection)
{
}

void AJunMonster::OnHitReactEnded(EHitReactType EndedHitReact)
{
}

void AJunMonster::OnIncomingHitResolvedWithoutHitReact(EHitReactType HitType)
{
}

bool AJunMonster::IsInHitReact() const
{
	return CurrentHitReact != EHitReactType::None;
}

bool AJunMonster::CanBeInterruptedBy(EHitReactType IncomingHitReact) const
{
	if (CurrentState == EMonsterState::Dead)
	{
		return false;
	}

	if (HasGameplayTag(JunGameplayTags::State_Condition_SuperArmor))
	{
		return IsHeavyHitType(IncomingHitReact) ||
			IncomingHitReact == EHitReactType::LargeHit_Long ||
			IsLightingShockHitType(IncomingHitReact);
	}

	return true;
}

bool AJunMonster::ShouldStartHitReact(EHitReactType IncomingHitReact) const
{
	return IncomingHitReact != EHitReactType::None;
}

float AJunMonster::GetHitReactDuration(EHitReactType HitType) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
		return LightHitDuration;
	case EHitReactType::HeavyHit_A:
	case EHitReactType::HeavyHit_B:
	case EHitReactType::HeavyHit_C:
		return HeavyHitDuration;
	case EHitReactType::LargeHit_Short:
		return LargeHitDuration;
	case EHitReactType::LargeHit_Long:
		return LargeHitLongDuration;
	case EHitReactType::Lighting_Shock:
		return LightingShockMontage ? LightingShockMontage->GetPlayLength() : LightingShockDuration;
	default:
		return 0.f;
	}
}

float AJunMonster::GetHitReactControlLockDuration(EHitReactType HitType) const
{
	return GetHitReactDuration(HitType);
}

ECharacterHitReactDirection AJunMonster::AdjustHitReactDirection(
	EHitReactType HitType,
	ECharacterHitReactDirection HitDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	return HitDirection;
}

ECharacterHitReactDirection AJunMonster::DetermineHitReactDirection(const AActor* DamageCauser, const FVector& SwingDirection) const
{
	if (!DamageCauser)
	{
		return ResolveHitReactDirectionFromSwing(*this, SwingDirection);
	}

	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(DamageCauser->GetActorLocation() - GetActorLocation());
	ToAttackerLocal.Z = 0.f;

	if (ToAttackerLocal.IsNearlyZero())
	{
		return ResolveHitReactDirectionFromSwing(*this, SwingDirection);
	}

	const float AttackerYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(ToAttackerLocal.Y, ToAttackerLocal.X));

	if (AttackerYawDegrees > 60.f && AttackerYawDegrees <= 135.f)
	{
		return ECharacterHitReactDirection::Right_R;
	}

	if (AttackerYawDegrees < -60.f && AttackerYawDegrees >= -135.f)
	{
		return ECharacterHitReactDirection::Left_L;
	}

	if (AttackerYawDegrees > 135.f || AttackerYawDegrees < -135.f)
	{
		return ECharacterHitReactDirection::Back_B;
	}

	const ECharacterHitReactDirection SwingResolvedDirection = ResolveHitReactDirectionFromSwing(*this, SwingDirection);
	if (SwingResolvedDirection == ECharacterHitReactDirection::Front_FL ||
		SwingResolvedDirection == ECharacterHitReactDirection::Front_FR)
	{
		return SwingResolvedDirection;
	}

	return ECharacterHitReactDirection::Front_F;
}

ECharacterHitReactDirection AJunMonster::DetermineHitReactDirection(
	const AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	const ECharacterHitReactDirection AutoDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	if (!IsFrontHitReactDirection(AutoDirection))
	{
		return AutoDirection;
	}

	switch (DefenseRuleData.FrontHitReactDirectionOverride)
	{
	case EJunFrontHitReactDirectionOverride::Front_FL:
		return ECharacterHitReactDirection::Front_FL;
	case EJunFrontHitReactDirectionOverride::Front_FR:
		return ECharacterHitReactDirection::Front_FR;
	case EJunFrontHitReactDirectionOverride::Front_F:
		return ECharacterHitReactDirection::Front_F;
	case EJunFrontHitReactDirectionOverride::Auto:
	default:
		return AutoDirection;
	}
}

UAnimMontage* AJunMonster::GetHitReactMontage(EHitReactType HitType, ECharacterHitReactDirection HitDirection) const
{
	switch (HitType)
	{
	case EHitReactType::LightHit:
		return ResolveDirectionalHitReactMontage(
			HitDirection,
			LightHitBackMontage,
			LightHitFront_FMontage,
			LightHitFront_FLMontage,
			LightHitFront_FRMontage,
			LightHitLeftMontage,
			LightHitRightMontage
		);
	case EHitReactType::HeavyHit_A:
		if (HitDirection == ECharacterHitReactDirection::Back_B ||
			HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ResolveDirectionalHitReactMontage(
				HitDirection,
				LargeHitBackMontage,
				LargeHitFrontMontage,
				LargeHitFrontLeftMontage,
				LargeHitFrontRightMontage,
				LargeHitLeftMontage,
				LargeHitRightMontage
			);
		}
		return HeavyHitFront_AMontage;
	case EHitReactType::HeavyHit_B:
		if (HitDirection == ECharacterHitReactDirection::Back_B ||
			HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ResolveDirectionalHitReactMontage(
				HitDirection,
				LargeHitBackMontage,
				LargeHitFrontMontage,
				LargeHitFrontLeftMontage,
				LargeHitFrontRightMontage,
				LargeHitLeftMontage,
				LargeHitRightMontage
			);
		}
		return HeavyHitFront_BMontage;
	case EHitReactType::HeavyHit_C:
		if (HitDirection == ECharacterHitReactDirection::Back_B ||
			HitDirection == ECharacterHitReactDirection::Left_L ||
			HitDirection == ECharacterHitReactDirection::Right_R)
		{
			return ResolveDirectionalHitReactMontage(
				HitDirection,
				LargeHitBackMontage,
				LargeHitFrontMontage,
				LargeHitFrontLeftMontage,
				LargeHitFrontRightMontage,
				LargeHitLeftMontage,
				LargeHitRightMontage
			);
		}
		return HeavyHitFront_CMontage;
	case EHitReactType::LargeHit_Short:
	case EHitReactType::LargeHit_Long:
		return ResolveDirectionalHitReactMontage(
			HitDirection,
			LargeHitBackMontage,
			LargeHitFrontMontage,
			LargeHitFrontLeftMontage,
			LargeHitFrontRightMontage,
			LargeHitLeftMontage,
			LargeHitRightMontage
		);
	case EHitReactType::Lighting_Shock:
		return LightingShockMontage;
	default:
		return nullptr;
	}
}

