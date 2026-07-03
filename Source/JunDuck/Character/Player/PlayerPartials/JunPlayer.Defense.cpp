#include "Character/Player/JunPlayer.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "UObject/UnrealType.h"
#pragma region Defense State Machine
// ============================================================================
// Defense State Machine
// ============================================================================

void AJunPlayer::StartDefenseSequence()
{
	if (!DefenseComponent)
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::Defense),
		[this]()
		{
			ProcessDefenseSequenceStart(false);
		});
}

void AJunPlayer::StartAirParrySequence()
{
	if (!DefenseComponent)
	{
		return;
	}

	TryProcessActionStartRequest(
		MakeActionStartRequest(EJunPlayerActionState::Defense),
		[this]()
		{
			ProcessDefenseSequenceStart(true);
		});
}

void AJunPlayer::ProcessDefenseSequenceStart(bool bAirParry)
{
	if (!DefenseComponent)
	{
		return;
	}

	if (bAirParry)
	{
		DefenseComponent->StartAirParrySequence();
	}
	else
	{
		DefenseComponent->StartDefenseSequence();
	}
}

void AJunPlayer::BeginDefenseFromBufferedInput()
{
	if (DefenseComponent)
	{
		DefenseComponent->BeginDefenseFromBufferedInput();
	}
}

void AJunPlayer::ResolveCurrentDeflectAttempt()
{
	if (DefenseComponent)
	{
		DefenseComponent->ResolveCurrentDeflectAttempt();
	}
}

void AJunPlayer::ResetDefenseTransitionRuntime(bool bClearBufferedCancelAction)
{
	if (DefenseComponent)
	{
		DefenseComponent->ResetDefenseTransitionRuntime(bClearBufferedCancelAction);
	}
}

void AJunPlayer::ApplyDefenseStartBlockTags()
{
	if (DefenseComponent)
	{
		DefenseComponent->ApplyDefenseStartBlockTags();
	}
}

void AJunPlayer::ApplyAirDefenseStartBlockTags()
{
	if (DefenseComponent)
	{
		DefenseComponent->ApplyAirDefenseStartBlockTags();
	}
}

void AJunPlayer::ApplyGuardOnBlockTags()
{
	if (DefenseComponent)
	{
		DefenseComponent->ApplyGuardOnBlockTags();
	}
}

void AJunPlayer::ClearDefenseBlockTags()
{
	if (DefenseComponent)
	{
		DefenseComponent->ClearDefenseBlockTags();
	}
}

void AJunPlayer::EnterGuardLoop()
{
	if (DefenseComponent)
	{
		if (!RequestActionState(EJunPlayerActionState::Defense, EJunActionTransitionReason::System))
		{
			return;
		}

		DefenseComponent->EnterGuardLoop();
	}
}

void AJunPlayer::BeginGuardEnd()
{
	if (DefenseComponent)
	{
		DefenseComponent->BeginGuardEnd();
	}
}

void AJunPlayer::BeginAirGuardEnd()
{
	if (DefenseComponent)
	{
		DefenseComponent->BeginAirGuardEnd();
	}
}

void AJunPlayer::FinishDefense()
{
	if (DefenseComponent)
	{
		DefenseComponent->FinishDefense();
	}
	ClearActionStateIf(EJunPlayerActionState::Defense, EJunActionTransitionReason::System);
}

float AJunPlayer::GetCurrentDefenseTimelineRate() const
{
	const float TimelineRate = DefenseComponent
		? DefenseComponent->GetCurrentDefensePlayRate()
		: 1.f;
	return FMath::Max(TimelineRate, KINDA_SMALL_NUMBER);
}

void AJunPlayer::OnGuardStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != GuardStartMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardStartMontageEnded);
	}

	if (GetDefenseState() != EJunDefenseState::Starting)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	ResolveCurrentDeflectAttempt();
}

void AJunPlayer::OnGuardEndMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != GuardEndMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnGuardEndMontageEnded);
	}

	const EJunBufferedDefenseCancelAction BufferedAction = DefenseComponent
		? DefenseComponent->GetBufferedDefenseTransitionCancelAction()
		: EJunBufferedDefenseCancelAction::None;
	if (BufferedAction != EJunBufferedDefenseCancelAction::None)
	{
		TryProcessBufferedDefenseTransitionCancelAction();
		return;
	}

	const float GuardEndRemainTime = DefenseComponent
		? DefenseComponent->GetGuardEndFinishRemainTime()
		: 0.f;
	if (GuardEndRemainTime <= 0.f)
	{
		FinishDefense();
	}
}

void AJunPlayer::OnAirGuardStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != AirGuardStartMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardStartMontageEnded);
	}

	const bool bAirParryActive = DefenseComponent
		? DefenseComponent->IsAirParrySequenceActive()
		: false;
	if (GetDefenseState() != EJunDefenseState::Starting || !bAirParryActive)
	{
		return;
	}

	if (bInterrupted)
	{
		return;
	}

	ResolveCurrentDeflectAttempt();
}

void AJunPlayer::OnAirGuardEndMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != AirGuardEndMontage)
	{
		return;
	}

	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AJunPlayer::OnAirGuardEndMontageEnded);
	}

	const bool bAirParryActive = DefenseComponent
		? DefenseComponent->IsAirParrySequenceActive()
		: false;
	if (GetDefenseState() == EJunDefenseState::Ending && bAirParryActive)
	{
		FinishDefense();
	}
}

void AJunPlayer::ApplyRunningStateToAnimInstance(bool bNewIsRunning)
{
	if (!AnimInstance)
	{
		AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	}

	if (!AnimInstance)
	{
		return;
	}

	if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), TEXT("bIsRunning")))
	{
		BoolProp->SetPropertyValue_InContainer(AnimInstance, bNewIsRunning);
	}

	if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), TEXT("bUseRunLocomotion")))
	{
		BoolProp->SetPropertyValue_InContainer(AnimInstance, bNewIsRunning);
	}
}

void AJunPlayer::OpenParryWindow(bool bCountAsParryAttempt)
{
	if (DefenseComponent)
	{
		DefenseComponent->OpenParryWindow(bCountAsParryAttempt);
	}
}

void AJunPlayer::ResetParrySpamPenalty()
{
	if (DefenseComponent)
	{
		DefenseComponent->ResetParrySpamPenalty();
	}
}

void AJunPlayer::DrawDefenseTimingDebug() const
{
	if (!bShowDefenseTimingDebug || !GEngine)
	{
		return;
	}

	const bool bParryOpen = DefenseComponent && DefenseComponent->IsParryWindowOpen();
	const float ParryRemainTime = DefenseComponent ? DefenseComponent->GetParryWindowRemainTime() : 0.f;
	const float ParryRemainFrames = ParryRemainTime * 60.f;
	const float ParryWindowFrames = DefenseComponent ? DefenseComponent->GetCurrentParryWindowDuration() * 60.f : 0.f;
	const float PerfectWindowFrames = DefenseComponent ? DefenseComponent->GetCurrentPerfectParryWindowDuration() * 60.f : 0.f;
	const int32 ParryPenaltyStack = DefenseComponent ? DefenseComponent->GetParrySpamPenaltyStack() : 0;
	const float DodgeTimelineRate = GetCurrentDodgeTimelineRate();
	const float DodgeInvincibleRealTime = DodgeInvincibleRemainTime / FMath::Max(DodgeTimelineRate, KINDA_SMALL_NUMBER);
	const float DodgeInvincibleFrames = DodgeInvincibleRealTime * 60.f;
	const bool bDodgeInvincibleActive = HasGameplayTag(JunGameplayTags::State_Condition_Invincible);

	const FString DebugText = FString::Printf(
		TEXT("Defense Timing Debug\n")
		TEXT("Parry: %s | Remain %.3fs (%.1ff) | Window %.1ff | Perfect %.1ff | SpamStack %d\n")
		TEXT("Dodge IFrame: %s | Remain %.3fs (%.1ff) | PlayRate %.2f"),
		bParryOpen ? TEXT("OPEN") : TEXT("CLOSED"),
		ParryRemainTime,
		ParryRemainFrames,
		ParryWindowFrames,
		PerfectWindowFrames,
		ParryPenaltyStack,
		bDodgeInvincibleActive ? TEXT("ON") : TEXT("OFF"),
		DodgeInvincibleRealTime,
		DodgeInvincibleFrames,
		CurrentDodgePlayRate);

	const FColor DebugColor = (bParryOpen || bDodgeInvincibleActive) ? FColor::Green : FColor::White;
	GEngine->AddOnScreenDebugMessage(61001, 0.f, DebugColor, DebugText);
}


#pragma endregion
