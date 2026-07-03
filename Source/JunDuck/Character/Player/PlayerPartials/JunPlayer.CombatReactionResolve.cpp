#include "Character/Player/JunPlayer.h"

#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

#pragma region Combat Reaction Resolve
// ============================================================================
// Combat Reaction Resolve
// ============================================================================

void AJunPlayer::UpdateDefenseInput(float DeltaTime)
{
	if (DefenseComponent)
	{
		DefenseComponent->UpdateDefense(DeltaTime);
	}
}

EJunPlayerHitResolveResult AJunPlayer::ResolveIncomingHitResult(EHitReactType IncomingHitType, const AActor* DamageCauser) const
{
	if (CombatReactionComponent)
	{
		return CombatReactionComponent->ResolveIncomingHitResult(IncomingHitType, DamageCauser);
	}

	if (LastIncomingDefenseRuleData.bCanBeDodgedByInvincibility &&
		HasGameplayTag(JunGameplayTags::State_Condition_Invincible))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	const bool bCanDefendFromIncomingAngle = IsDamageCauserInDefenseAngle(DamageCauser);

	if (LastIncomingDefenseRuleData.bCanBeParried && IsParryWindowOpen() && bCanDefendFromIncomingAngle)
	{
		const float CurrentWindowDuration = DefenseComponent->GetCurrentParryWindowDuration();
		const float CurrentPerfectWindowDuration = DefenseComponent->GetCurrentPerfectParryWindowDuration();
		const float PerfectParryRemainThreshold =
			FMath::Max(0.f, CurrentWindowDuration - CurrentPerfectWindowDuration);
		return DefenseComponent->GetParryWindowRemainTime() >= PerfectParryRemainThreshold
			? EJunPlayerHitResolveResult::PerfectParrySuccess
			: EJunPlayerHitResolveResult::NormalParrySuccess;
	}

	if (LastIncomingDefenseRuleData.bCanBeGuarded && GetDefenseState() == EJunDefenseState::Guarding && bCanDefendFromIncomingAngle)
	{
		return EJunPlayerHitResolveResult::GuardBlock;
	}

	if (!CanBeInterruptedBy(IncomingHitType))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	return EJunPlayerHitResolveResult::HitReact;
}

bool AJunPlayer::IsDamageCauserInDefenseAngle(const AActor* DamageCauser) const
{
	if (CombatReactionComponent)
	{
		return CombatReactionComponent->IsDamageCauserInDefenseAngle(DamageCauser);
	}

	if (!DamageCauser)
	{
		return true;
	}

	const FVector ToDamageCauser = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToDamageCauser.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, ToDamageCauser);
	const float HalfAngle = FMath::Clamp(DefenseFrontAngle * 0.5f, 0.f, 180.f);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(HalfAngle));
	return Dot >= MinDot;
}

bool AJunPlayer::CanBeInterruptedBy(EHitReactType IncomingHitType) const
{
	if (CombatReactionComponent)
	{
		return CombatReactionComponent->CanBeInterruptedBy(IncomingHitType);
	}

	auto IsHeavyHitType = [](const EHitReactType HitType)
	{
		return HitType == EHitReactType::HeavyHit_A
			|| HitType == EHitReactType::HeavyHit_B
			|| HitType == EHitReactType::HeavyHit_C;
	};

	auto IsLargeHitType = [](const EHitReactType HitType)
	{
		return HitType == EHitReactType::LargeHit_Short
			|| HitType == EHitReactType::LargeHit_Long;
	};

	if (CurrentHitState == EJunPlayerHitState::None)
	{
		return true;
	}

	if (CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		return !IsParryWindowOpen();
	}

	if (CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		return GetDefenseState() == EJunDefenseState::Guarding;
	}

	if (CurrentHitState == EJunPlayerHitState::GuardBreak)
	{
		return true;
	}

	if (CurrentHitState != EJunPlayerHitState::HitReact)
	{
		return true;
	}

	if (IncomingHitType == EHitReactType::LightHit ||
		IsLargeHitType(IncomingHitType) ||
		IsHeavyHitType(IncomingHitType) ||
		IncomingHitType == EHitReactType::Lighting_Shock)
	{
		return true;
	}

	return CurrentHitReactType == EHitReactType::None;
}

ECharacterHitReactDirection AJunPlayer::DetermineHitReactDirection(const AActor* DamageCauser, const FVector& SwingDirection) const
{
	auto ResolveFrontSwingDirection = [this, &SwingDirection]()
	{
		const FVector SafeSwingDirection = SwingDirection.GetSafeNormal();
		if (SafeSwingDirection.IsNearlyZero())
		{
			return ECharacterHitReactDirection::Front_F;
		}

		const FVector LocalSwingDirection = GetActorTransform().InverseTransformVectorNoScale(SafeSwingDirection);
		const float SwingYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalSwingDirection.Y, LocalSwingDirection.X));

		if (SwingYawDegrees > 25.f && SwingYawDegrees <= 70.f)
		{
			return ECharacterHitReactDirection::Front_FR;
		}
		if (SwingYawDegrees < -25.f && SwingYawDegrees >= -70.f)
		{
			return ECharacterHitReactDirection::Front_FL;
		}

		return ECharacterHitReactDirection::Front_F;
	};

	if (!DamageCauser)
	{
		return ResolveFrontSwingDirection();
	}

	FVector ToAttackerLocal = GetActorTransform().InverseTransformVectorNoScale(DamageCauser->GetActorLocation() - GetActorLocation());
	ToAttackerLocal.Z = 0.f;

	if (ToAttackerLocal.IsNearlyZero())
	{
		return ResolveFrontSwingDirection();
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

	return ResolveFrontSwingDirection();
}

ECharacterHitReactDirection AJunPlayer::DetermineHitReactDirection(
	const AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData) const
{
	const ECharacterHitReactDirection AutoDirection = DetermineHitReactDirection(DamageCauser, SwingDirection);
	const bool bIsFrontDirection =
		AutoDirection == ECharacterHitReactDirection::Front_F ||
		AutoDirection == ECharacterHitReactDirection::Front_FL ||
		AutoDirection == ECharacterHitReactDirection::Front_FR;

	if (!bIsFrontDirection)
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

ECharacterKnockbackDirection AJunPlayer::DetermineKnockbackDirectionFromDamageCauser(const AActor* DamageCauser) const
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

UAnimMontage* AJunPlayer::GetHitReactMontage(EHitReactType HitType, ECharacterHitReactDirection HitDirection) const
{
	auto ResolveDirectionalMontage = [HitDirection](
		UAnimMontage* BackMontage,
		UAnimMontage* FrontMontage,
		UAnimMontage* FrontLeftMontage,
		UAnimMontage* FrontRightMontage,
		UAnimMontage* LeftMontage,
		UAnimMontage* RightMontage) -> UAnimMontage*
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
	};

	switch (HitType)
	{
	case EHitReactType::LightHit:
		return ResolveDirectionalMontage(
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
			return ResolveDirectionalMontage(
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
			return ResolveDirectionalMontage(
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
			return ResolveDirectionalMontage(
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
		return ResolveDirectionalMontage(
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

EJunAirHitReactType AJunPlayer::ResolveAirHitReactType() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		return EJunAirHitReactType::None;
	}

	const EJunAirHitReactType RequestedAirHitReactType = LastIncomingDefenseRuleData.AirHitReactType;
	if (RequestedAirHitReactType == EJunAirHitReactType::Heavy &&
		GetDistanceFromGround() < AirHeavyHitMinGroundDistance)
	{
		return EJunAirHitReactType::None;
	}

	return GetAirHitReactMontage(RequestedAirHitReactType) ? RequestedAirHitReactType : EJunAirHitReactType::None;
}

UAnimMontage* AJunPlayer::GetAirHitReactMontage(EJunAirHitReactType AirHitReactType) const
{
	switch (AirHitReactType)
	{
	case EJunAirHitReactType::Light:
		return AirHitFLightMontage;
	case EJunAirHitReactType::Heavy:
		return AirHitFHeavyMontage;
	case EJunAirHitReactType::None:
	default:
		return nullptr;
	}
}

UAnimMontage* AJunPlayer::GetParrySuccessMontage(const FVector& SwingDirection)
{
	if (!bHasSelectedInitialParrySuccessSide)
	{
		bNextParrySuccessUsesLeftSide = FMath::RandBool();
		bHasSelectedInitialParrySuccessSide = true;
	}

	const bool bUseLeft = bNextParrySuccessUsesLeftSide;
	bNextParrySuccessUsesLeftSide = !bNextParrySuccessUsesLeftSide;

	if (bUseLeft)
	{
		bLastParrySuccessUsedLeftSide = ParrySuccessL_UpMontage != nullptr || ParrySuccessR_UpMontage == nullptr;
		return ParrySuccessL_UpMontage ? ParrySuccessL_UpMontage : ParrySuccessR_UpMontage;
	}

	bLastParrySuccessUsedLeftSide = ParrySuccessR_UpMontage == nullptr && ParrySuccessL_UpMontage != nullptr;
	return ParrySuccessR_UpMontage ? ParrySuccessR_UpMontage : ParrySuccessL_UpMontage;
}

UAnimMontage* AJunPlayer::GetAirParrySuccessMontage()
{
	if (!bHasSelectedInitialParrySuccessSide)
	{
		bNextParrySuccessUsesLeftSide = FMath::RandBool();
		bHasSelectedInitialParrySuccessSide = true;
	}

	const bool bUseLeft = bNextParrySuccessUsesLeftSide;
	bNextParrySuccessUsesLeftSide = !bNextParrySuccessUsesLeftSide;

	if (bUseLeft)
	{
		bLastParrySuccessUsedLeftSide = AirParrySuccessL_UpMontage != nullptr ||
			(AirParrySuccessR_UpMontage == nullptr && ParrySuccessL_UpMontage != nullptr);
		return AirParrySuccessL_UpMontage
			? AirParrySuccessL_UpMontage.Get()
			: (AirParrySuccessR_UpMontage ? AirParrySuccessR_UpMontage.Get() : GetParrySuccessMontage(FVector::ZeroVector));
	}

	bLastParrySuccessUsedLeftSide = AirParrySuccessR_UpMontage == nullptr &&
		(AirParrySuccessL_UpMontage != nullptr || ParrySuccessR_UpMontage == nullptr);
	return AirParrySuccessR_UpMontage
		? AirParrySuccessR_UpMontage.Get()
		: (AirParrySuccessL_UpMontage ? AirParrySuccessL_UpMontage.Get() : GetParrySuccessMontage(FVector::ZeroVector));
}

#pragma endregion
