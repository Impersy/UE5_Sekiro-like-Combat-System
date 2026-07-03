#include "Character/Player/PlayerComponent/JunPlayerCombatReactionComponent.h"

#include "Character/JunCharacter.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/JunDefenseReactionTargetInterface.h"
#include "Interface/JunJumpCounterTargetInterface.h"
#include "JunGameplayTags.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunCombatVFXSubsystem.h"

UJunPlayerCombatReactionComponent::UJunPlayerCombatReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJunPlayerCombatReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayer = Cast<AJunPlayer>(GetOwner());
}

FJunAttackHitResult UJunPlayerCombatReactionComponent::ReceiveHit(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	FJunAttackHitResult Result;
	if (!OwnerPlayer ||
		OwnerPlayer->Is_Dead() ||
		OwnerPlayer->IsExecuting() ||
		OwnerPlayer->IsInDeathSequence())
	{
		return Result;
	}

	Result.bProcessed = true;
	Result.HpBeforeHit = OwnerPlayer->GetHp();

	OwnerPlayer->LastIncomingSwingDirection = SwingDirection;
	OwnerPlayer->LastIncomingHitReactType = HitType;
	OwnerPlayer->LastIncomingKnockbackDirection = OwnerPlayer->DetermineKnockbackDirectionFromDamageCauser(DamageCauser);
	OwnerPlayer->LastIncomingDefenseKnockbackData = DefenseKnockbackData;
	OwnerPlayer->LastIncomingDefenseRuleData = DefenseRuleData;
	OwnerPlayer->HitReactFacingTarget = DamageCauser;

	if (TryHandleSpecialDefense(DamageCauser, DefenseRuleData, Result.DefenseOutcome))
	{
		Result.HpAfterHit = OwnerPlayer->GetHp();
		return Result;
	}

	const EJunPlayerHitResolveResult ResolveResult = ResolveIncomingHitResult(HitType, DamageCauser);
	switch (ResolveResult)
	{
	case EJunPlayerHitResolveResult::Ignored:
		if (DefenseRuleData.bCanBeDodgedByInvincibility &&
			OwnerPlayer->HasGameplayTag(JunGameplayTags::State_Condition_Invincible))
		{
			Result.DefenseOutcome = EJunDefenseOutcome::Invincible;
		}
		else
		{
			Result.DefenseOutcome = EJunDefenseOutcome::Rejected;
		}
		break;
	case EJunPlayerHitResolveResult::PerfectParrySuccess:
		HandlePerfectParrySuccess(DamageCauser, DefenseRuleData, bCanBuildAttackerPostureOnParry);
		Result.DefenseOutcome = EJunDefenseOutcome::PerfectParried;
		Result.ReactionOutcome = EJunHitReactionOutcome::Montage;
		break;
	case EJunPlayerHitResolveResult::NormalParrySuccess:
		HandleNormalParrySuccess(DamageCauser, DefenseRuleData, bCanBuildAttackerPostureOnParry);
		Result.DefenseOutcome = EJunDefenseOutcome::NormalParried;
		Result.ReactionOutcome = EJunHitReactionOutcome::Montage;
		break;
	case EJunPlayerHitResolveResult::GuardBlock:
		HandleGuardBlockSuccess();
		Result.DefenseOutcome = EJunDefenseOutcome::Guarded;
		Result.ReactionOutcome = EJunHitReactionOutcome::Montage;
		break;
	case EJunPlayerHitResolveResult::HitReact:
	default:
		HandleDamageHitReact(HitType, DamageAmount, DamageCauser, SwingDirection, DefenseRuleData);
		Result.ReactionOutcome = EJunHitReactionOutcome::Montage;
		break;
	}

	Result.HpAfterHit = OwnerPlayer->GetHp();
	Result.bDamageApplied = Result.HpAfterHit < Result.HpBeforeHit;
	return Result;
}

void UJunPlayerCombatReactionComponent::HandlePerfectParrySuccess(
	AActor* DamageCauser,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (!OwnerPlayer)
	{
		return;
	}

	const bool bAirParry = OwnerPlayer->GetCharacterMovement() && OwnerPlayer->GetCharacterMovement()->IsFalling();

	OwnerPlayer->PlayDefenseSound(EJunDefenseSoundType::PerfectParry);
	OwnerPlayer->PlayDefenseCameraShake(OwnerPlayer->PerfectParryCameraShakeScale);
	if (bAirParry)
	{
		OwnerPlayer->ReducePosture(OwnerPlayer->PerfectAirParryPostureReduction);
	}
	else
	{
		OwnerPlayer->AddPosture(OwnerPlayer->PerfectParryPostureGain);
	}
	StartParrySuccess(true);
	NotifyDefenseReactionTarget(
		DamageCauser,
		true,
		bAirParry,
		1.f,
		DefenseRuleData,
		bCanBuildAttackerPostureOnParry);
}

bool UJunPlayerCombatReactionComponent::TryHandleSpecialDefense(
	AActor* DamageCauser,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	EJunDefenseOutcome& OutDefenseOutcome)
{
	OutDefenseOutcome = EJunDefenseOutcome::None;
	if (!OwnerPlayer)
	{
		return false;
	}

	if (DefenseRuleData.DangerAttackType == EJunDangerAttackType::JumpCounter &&
		OwnerPlayer->IsJumpCounterEvadeSuccessful())
	{
		if (DamageCauser && Cast<IJunJumpCounterTargetInterface>(DamageCauser))
		{
			OwnerPlayer->bJumpCounterFollowUpDefenseBypassAvailable = true;
			OwnerPlayer->bJumpCounterStompFollowUpAvailable = true;
			OwnerPlayer->JumpCounterStompTarget = DamageCauser;
		}
		OutDefenseOutcome = EJunDefenseOutcome::JumpCountered;
		return true;
	}

	if (DefenseRuleData.DangerAttackType == EJunDangerAttackType::MikiriCounter &&
		OwnerPlayer->TryHandleMikiriCounter(DamageCauser))
	{
		OutDefenseOutcome = EJunDefenseOutcome::MikiriCountered;
		return true;
	}

	return false;
}

void UJunPlayerCombatReactionComponent::NotifyDefenseReactionTarget(
	AActor* DamageCauser,
	bool bPerfectParry,
	bool bAirParry,
	float PostureScale,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (!OwnerPlayer)
	{
		return;
	}

	IJunDefenseReactionTargetInterface* DefenseReactionTarget = Cast<IJunDefenseReactionTargetInterface>(DamageCauser);
	if (!DefenseReactionTarget)
	{
		return;
	}

	if (bCanBuildAttackerPostureOnParry)
	{
		DefenseReactionTarget->NotifyAttackParriedBy(OwnerPlayer, PostureScale, DefenseRuleData);
	}

	DefenseReactionTarget->NotifyPlayerParrySucceeded(OwnerPlayer, bPerfectParry, bAirParry);
}

void UJunPlayerCombatReactionComponent::HandleNormalParrySuccess(
	AActor* DamageCauser,
	const FJunAttackDefenseRuleData& DefenseRuleData,
	bool bCanBuildAttackerPostureOnParry)
{
	if (!OwnerPlayer)
	{
		return;
	}

	const bool bAirParry = OwnerPlayer->GetCharacterMovement() && OwnerPlayer->GetCharacterMovement()->IsFalling();
	IJunDefenseReactionTargetInterface* DefenseReactionTarget =
		Cast<IJunDefenseReactionTargetInterface>(DamageCauser);

	OwnerPlayer->PlayDefenseSound(EJunDefenseSoundType::NormalParry);
	OwnerPlayer->PlayDefenseCameraShake(OwnerPlayer->NormalParryCameraShakeScale);
	if (OwnerPlayer->IsPostureFull())
	{
		StartGuardBreak();
		return;
	}
	if (bCanBuildAttackerPostureOnParry && DefenseReactionTarget)
	{
		DefenseReactionTarget->NotifyAttackParriedBy(OwnerPlayer, 0.5f, DefenseRuleData);
	}
	if (bAirParry)
	{
		OwnerPlayer->ReducePosture(OwnerPlayer->NormalAirParryPostureReduction);
	}
	else
	{
		OwnerPlayer->AddPosture(OwnerPlayer->NormalParryPostureGain);
	}
	StartParrySuccess(false);
	if (DefenseReactionTarget)
	{
		DefenseReactionTarget->NotifyPlayerParrySucceeded(OwnerPlayer, false, bAirParry);
	}
}

void UJunPlayerCombatReactionComponent::HandleGuardBlockSuccess()
{
	if (!OwnerPlayer)
	{
		return;
	}

	OwnerPlayer->PlayDefenseSound(EJunDefenseSoundType::GuardHit);
	OwnerPlayer->PlayDefenseCameraShake(OwnerPlayer->GuardHitCameraShakeScale);
	if (OwnerPlayer->IsPostureFull())
	{
		StartGuardBreak();
		return;
	}
	OwnerPlayer->AddPosture(OwnerPlayer->GuardBlockPostureGain);
	StartGuardBlock();
}

void UJunPlayerCombatReactionComponent::HandleDamageHitReact(
	EHitReactType HitType,
	float DamageAmount,
	AActor* DamageCauser,
	const FVector& SwingDirection,
	const FJunAttackDefenseRuleData& DefenseRuleData)
{
	if (!OwnerPlayer)
	{
		return;
	}

	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(DamageCauser);
	const float DamageMultiplier = OwnerPlayer->GuardBreakVulnerableRemainTime > 0.f
		? OwnerPlayer->GuardBreakDamageMultiplier
		: 1.f;
	OwnerPlayer->OnDamaged(FMath::RoundToInt(DamageAmount * DamageMultiplier), AttackerCharacter);
	if (OwnerPlayer->Is_Dead() || OwnerPlayer->IsInDeathSequence())
	{
		return;
	}
	if (OwnerPlayer->AddPosture(OwnerPlayer->HitReactPostureGain))
	{
		StartGuardBreak();
		return;
	}
	OwnerPlayer->StartHitReact(
		HitType,
		OwnerPlayer->DetermineHitReactDirection(DamageCauser, SwingDirection, DefenseRuleData));
}

void UJunPlayerCombatReactionComponent::StartParrySuccess(bool bPerfectParry)
{
	if (!OwnerPlayer)
	{
		return;
	}

	OwnerPlayer->ResetParrySpamPenalty();

	OwnerPlayer->InterruptActionsForHitReaction();
	OwnerPlayer->PlayDefenseEffect(
		bPerfectParry ? EJunCombatDefenseVFX::PerfectParry : EJunCombatDefenseVFX::NormalParry,
		OwnerPlayer->CachedParryEffectLocationComponent,
		OwnerPlayer->ParryEffectLocationComponentName);

	OwnerPlayer->CurrentHitState = EJunPlayerHitState::ParrySuccess;
	OwnerPlayer->SetActionState(EJunPlayerActionState::ParrySuccess, EJunActionTransitionReason::ForcedByDamage);
	OwnerPlayer->ChainParryWindowRemainTime = OwnerPlayer->ChainParryWindowDuration;
	OwnerPlayer->ParrySuccessElapsedTime = 0.f;
	OwnerPlayer->ParrySuccessDefenseHoldElapsedTime = 0.f;
	OwnerPlayer->PendingParrySuccessActionRequest.Reset();
	OwnerPlayer->bParrySuccessHeavyAttackInputHeld = false;
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	OwnerPlayer->ApplyCommonKnockback(
		OwnerPlayer->LastIncomingKnockbackDirection,
		OwnerPlayer->LastIncomingDefenseKnockbackData.ParrySuccess.Strength,
		OwnerPlayer->LastIncomingDefenseKnockbackData.ParrySuccess.BrakingDeceleration,
		OwnerPlayer->LastIncomingDefenseKnockbackData.ParrySuccess.GroundFriction,
		OwnerPlayer->LastIncomingDefenseKnockbackData.ParrySuccess.BrakingFrictionFactor,
		OwnerPlayer->LastIncomingDefenseKnockbackData.ParrySuccess.OverrideDuration
	);

	const bool bUseAirParrySuccessMontage =
		(OwnerPlayer->DefenseComponent
			? OwnerPlayer->DefenseComponent->IsAirParrySequenceActive()
			: false) ||
		(OwnerPlayer->GetCharacterMovement() && OwnerPlayer->GetCharacterMovement()->IsFalling());
	UAnimMontage* ParrySuccessMontage = bUseAirParrySuccessMontage
		? OwnerPlayer->GetAirParrySuccessMontage()
		: OwnerPlayer->GetParrySuccessMontage(OwnerPlayer->LastIncomingSwingDirection);
	OwnerPlayer->CurrentParrySuccessMontage = ParrySuccessMontage;
	OwnerPlayer->PlayerHitStateRemainTime = ParrySuccessMontage
		? ParrySuccessMontage->GetPlayLength()
		: OwnerPlayer->ParrySuccessDuration;

	if (ParrySuccessMontage)
	{
		if (OwnerPlayer->AnimInstance)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.f, ParrySuccessMontage);
		}

		OwnerPlayer->PlayAnimMontage(ParrySuccessMontage);
	}
}

void UJunPlayerCombatReactionComponent::StartGuardBlock()
{
	if (!OwnerPlayer)
	{
		return;
	}

	OwnerPlayer->GetWorldTimerManager().ClearTimer(OwnerPlayer->GuardBlockReleaseIntoGuardEndTimerHandle);
	OwnerPlayer->PlayDefenseEffect(
		EJunCombatDefenseVFX::GuardHit,
		OwnerPlayer->CachedDeflectEffectLocationComponent,
		OwnerPlayer->DeflectEffectLocationComponentName);

	OwnerPlayer->bGuardBlockReleasePending = false;
	OwnerPlayer->CurrentHitState = EJunPlayerHitState::GuardBlock;
	OwnerPlayer->SetActionState(EJunPlayerActionState::GuardBlock, EJunActionTransitionReason::ForcedByDamage);
	OwnerPlayer->PlayerHitStateRemainTime = OwnerPlayer->GuardBlockMontage
		? OwnerPlayer->GuardBlockMontage->GetPlayLength()
		: OwnerPlayer->GuardBlockDuration;
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);
	OwnerPlayer->ApplyCommonKnockback(
		OwnerPlayer->LastIncomingKnockbackDirection,
		OwnerPlayer->LastIncomingDefenseKnockbackData.GuardBlock.Strength,
		OwnerPlayer->LastIncomingDefenseKnockbackData.GuardBlock.BrakingDeceleration,
		OwnerPlayer->LastIncomingDefenseKnockbackData.GuardBlock.GroundFriction,
		OwnerPlayer->LastIncomingDefenseKnockbackData.GuardBlock.BrakingFrictionFactor,
		OwnerPlayer->LastIncomingDefenseKnockbackData.GuardBlock.OverrideDuration
	);

	if (OwnerPlayer->GuardBlockMontage)
	{
		if (OwnerPlayer->AnimInstance)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.f, OwnerPlayer->GuardBlockMontage);
		}

		OwnerPlayer->PlayAnimMontage(OwnerPlayer->GuardBlockMontage);
	}
}

void UJunPlayerCombatReactionComponent::StartGuardBreak()
{
	if (!OwnerPlayer)
	{
		return;
	}

	const bool bRestartingGuardBreak = OwnerPlayer->CurrentHitState == EJunPlayerHitState::GuardBreak;
	OwnerPlayer->SetActionState(EJunPlayerActionState::GuardBreak, EJunActionTransitionReason::ForcedByGuardBreak);

	OwnerPlayer->bJumpCounterFollowUpDefenseBypassAvailable = false;
	OwnerPlayer->bJumpCounterStompFollowUpAvailable = false;
	OwnerPlayer->bJumpCounterStompOpportunityPending = false;
	OwnerPlayer->JumpCounterStompTarget = nullptr;
	OwnerPlayer->JumpCounterStompPendingTarget = nullptr;

	OwnerPlayer->InterruptActionsForHitReaction();
	OwnerPlayer->PlayDefenseEffect(
		EJunCombatDefenseVFX::GuardBreak,
		OwnerPlayer->CachedGuardBreakEffectLocationComponent,
		OwnerPlayer->GuardBreakEffectLocationComponentName);

	const bool bUseLightingShockGuardBreak =
		OwnerPlayer->LastIncomingHitReactType == EHitReactType::Lighting_Shock &&
		OwnerPlayer->LightingShockMontage != nullptr;
	const bool bUseAirGuardBreak =
		OwnerPlayer->GetCharacterMovement() &&
		OwnerPlayer->GetCharacterMovement()->IsFalling() &&
		OwnerPlayer->AirGuardBreakMontage &&
		!bUseLightingShockGuardBreak;
	UAnimMontage* GuardBreakMontageToPlay = bUseLightingShockGuardBreak
		? OwnerPlayer->LightingShockMontage.Get()
		: (bUseAirGuardBreak
			? OwnerPlayer->AirGuardBreakMontage.Get()
			: OwnerPlayer->GuardBreakMontage.Get());

	OwnerPlayer->CurrentPosture = 0.f;
	OwnerPlayer->PostureRecoveryDelayRemainTime = 0.f;
	OwnerPlayer->CurrentHitState = EJunPlayerHitState::GuardBreak;
	OwnerPlayer->PlayerHitStateRemainTime = bUseAirGuardBreak
		? FMath::Max(OwnerPlayer->AirGuardBreakAirborneMaxDuration, OwnerPlayer->GuardBreakControlLockDuration)
		: (bUseLightingShockGuardBreak
			? (GuardBreakMontageToPlay ? GuardBreakMontageToPlay->GetPlayLength() : OwnerPlayer->LightingShockDuration)
			: (GuardBreakMontageToPlay ? GuardBreakMontageToPlay->GetPlayLength() : OwnerPlayer->GuardBreakDuration));
	OwnerPlayer->PlayerHitControlLockRemainTime = bUseAirGuardBreak
		? FMath::Max(OwnerPlayer->AirGuardBreakAirborneMaxDuration, OwnerPlayer->GuardBreakControlLockDuration)
		: (bUseLightingShockGuardBreak ? OwnerPlayer->LightingShockControlLockDuration : OwnerPlayer->GuardBreakControlLockDuration);
	OwnerPlayer->GuardBreakVulnerableRemainTime = OwnerPlayer->PlayerHitControlLockRemainTime;
	OwnerPlayer->ChainParryWindowRemainTime = 0.f;
	if (OwnerPlayer->DefenseComponent)
	{
		OwnerPlayer->DefenseComponent->CloseParryWindow();
	}
	OwnerPlayer->ParrySuccessElapsedTime = 0.f;
	OwnerPlayer->ParrySuccessDefenseHoldElapsedTime = 0.f;
	OwnerPlayer->PendingParrySuccessActionRequest.Reset();
	OwnerPlayer->bParrySuccessHeavyAttackInputHeld = false;
	OwnerPlayer->CurrentParrySuccessMontage = nullptr;
	OwnerPlayer->CurrentHitReactType = bUseLightingShockGuardBreak ? EHitReactType::Lighting_Shock : EHitReactType::None;
	OwnerPlayer->CurrentHitReactDirection = ECharacterHitReactDirection::Front_F;
	OwnerPlayer->CurrentHitReactMontage = bUseLightingShockGuardBreak
		? OwnerPlayer->LightingShockMontage.Get()
		: (bUseAirGuardBreak ? OwnerPlayer->AirHitFLightMontage.Get() : nullptr);
	OwnerPlayer->bCurrentHitReactUsesAirMontage = bUseAirGuardBreak;
	OwnerPlayer->ResetAirHeavyHitReactState();
	OwnerPlayer->bAirGuardBreakLandMontagePending = bUseAirGuardBreak;
	OwnerPlayer->DesiredMoveForward = 0.f;
	OwnerPlayer->DesiredMoveRight = 0.f;
	OwnerPlayer->PendingMoveForward = 0.f;
	OwnerPlayer->PendingMoveRight = 0.f;

	if (UCharacterMovementComponent* MovementComponent = OwnerPlayer->GetCharacterMovement())
	{
		if (bUseAirGuardBreak)
		{
			OwnerPlayer->ApplyAirGuardBreakTuning();
		}
		else
		{
			FVector NewVelocity = MovementComponent->Velocity;
			NewVelocity.X = 0.f;
			NewVelocity.Y = 0.f;
			MovementComponent->Velocity = NewVelocity;
		}
	}
	OwnerPlayer->ConsumeMovementInputVector();

	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Condition_HitReact);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);

	if (AJunPlayerController* JunPlayerController = Cast<AJunPlayerController>(OwnerPlayer->GetController()))
	{
		JunPlayerController->PlayPlayerPostureBreakGlow();
		JunPlayerController->StartPlayerPostureBreakHidePresentation();
	}

	if (GuardBreakMontageToPlay && (!bUseAirGuardBreak || bUseLightingShockGuardBreak))
	{
		OwnerPlayer->PlayHitReactMontageWithBlend(GuardBreakMontageToPlay, bRestartingGuardBreak);
	}
	else if (bUseAirGuardBreak && OwnerPlayer->AirHitFLightMontage)
	{
		OwnerPlayer->PlayHitReactMontageWithBlend(OwnerPlayer->AirHitFLightMontage, bRestartingGuardBreak);
	}
}

EJunPlayerHitResolveResult UJunPlayerCombatReactionComponent::ResolveIncomingHitResult(
	EHitReactType IncomingHitType,
	const AActor* DamageCauser) const
{
	if (!OwnerPlayer)
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	if (OwnerPlayer->LastIncomingDefenseRuleData.bCanBeDodgedByInvincibility &&
		OwnerPlayer->HasGameplayTag(JunGameplayTags::State_Condition_Invincible))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	const bool bCanDefendFromIncomingAngle = IsDamageCauserInDefenseAngle(DamageCauser);

	if (OwnerPlayer->LastIncomingDefenseRuleData.bCanBeParried &&
		OwnerPlayer->IsParryWindowOpen() &&
		bCanDefendFromIncomingAngle)
	{
		const float CurrentWindowDuration = OwnerPlayer->DefenseComponent->GetCurrentParryWindowDuration();
		const float CurrentPerfectWindowDuration = OwnerPlayer->DefenseComponent->GetCurrentPerfectParryWindowDuration();
		const float PerfectParryRemainThreshold =
			FMath::Max(0.f, CurrentWindowDuration - CurrentPerfectWindowDuration);
		return OwnerPlayer->DefenseComponent->GetParryWindowRemainTime() >= PerfectParryRemainThreshold
			? EJunPlayerHitResolveResult::PerfectParrySuccess
			: EJunPlayerHitResolveResult::NormalParrySuccess;
	}

	if (OwnerPlayer->LastIncomingDefenseRuleData.bCanBeGuarded &&
		OwnerPlayer->GetDefenseState() == EJunDefenseState::Guarding &&
		bCanDefendFromIncomingAngle)
	{
		return EJunPlayerHitResolveResult::GuardBlock;
	}

	if (!CanBeInterruptedBy(IncomingHitType))
	{
		return EJunPlayerHitResolveResult::Ignored;
	}

	return EJunPlayerHitResolveResult::HitReact;
}

bool UJunPlayerCombatReactionComponent::IsDamageCauserInDefenseAngle(const AActor* DamageCauser) const
{
	if (!OwnerPlayer || !DamageCauser)
	{
		return true;
	}

	const FVector ToDamageCauser = (DamageCauser->GetActorLocation() - OwnerPlayer->GetActorLocation()).GetSafeNormal2D();
	if (ToDamageCauser.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = OwnerPlayer->GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, ToDamageCauser);
	const float HalfAngle = FMath::Clamp(OwnerPlayer->DefenseFrontAngle * 0.5f, 0.f, 180.f);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(HalfAngle));
	return Dot >= MinDot;
}

bool UJunPlayerCombatReactionComponent::CanBeInterruptedBy(EHitReactType IncomingHitType) const
{
	if (!OwnerPlayer)
	{
		return false;
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

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::None)
	{
		return true;
	}

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		return !OwnerPlayer->IsParryWindowOpen();
	}

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		return OwnerPlayer->GetDefenseState() == EJunDefenseState::Guarding;
	}

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::GuardBreak)
	{
		return true;
	}

	if (OwnerPlayer->CurrentHitState != EJunPlayerHitState::HitReact)
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

	return OwnerPlayer->CurrentHitReactType == EHitReactType::None;
}
