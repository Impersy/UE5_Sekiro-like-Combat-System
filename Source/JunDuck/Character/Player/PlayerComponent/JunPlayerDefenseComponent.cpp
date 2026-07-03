#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"

#include "Character/Player/JunPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"

UJunPlayerDefenseComponent::UJunPlayerDefenseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJunPlayerDefenseComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayer = Cast<AJunPlayer>(GetOwner());
}

void UJunPlayerDefenseComponent::HandleDefenseStarted()
{
	if (!OwnerPlayer)
	{
		return;
	}

	if (TryHandleDefenseStartedFromActionState())
	{
		return;
	}

	if (TryHandleDefenseStartedFromHitState())
	{
		return;
	}

	if (TryHandleDefenseStartedFromAttackState())
	{
		return;
	}

	if (OwnerPlayer->HasGameplayTag(JunGameplayTags::State_Condition_ControlLocked))
	{
		return;
	}

	OwnerPlayer->CancelLockOnTurn();
	StartDefenseFromNeutralInput();
}

void UJunPlayerDefenseComponent::HandleDefenseReleased()
{
	if (!OwnerPlayer)
	{
		return;
	}

	// Release only changes intent during the deflect attempt.
	// BlockGuard transitions into Ending immediately on release.
	SetDefenseHoldIntent(false);

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::ParrySuccess &&
		OwnerPlayer->PendingParrySuccessActionRequest.ParrySuccessCancelAction ==
			EJunBufferedParrySuccessCancelAction::Defense)
	{
		OwnerPlayer->PendingParrySuccessActionRequest.Reset();
	}
	OwnerPlayer->ParrySuccessDefenseHoldElapsedTime = 0.f;

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		if (OwnerPlayer->AnimInstance && OwnerPlayer->GuardBlockMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(
				GuardBlockReleaseBlendOutTime,
				OwnerPlayer->GuardBlockMontage);
		}

		OwnerPlayer->bGuardBlockReleasePending = true;
		OwnerPlayer->PlayerHitStateRemainTime = FMath::Max(
			OwnerPlayer->PlayerHitStateRemainTime,
			GuardBlockReleaseBlendOutTime);
		OwnerPlayer->GetWorldTimerManager().ClearTimer(OwnerPlayer->GuardBlockReleaseIntoGuardEndTimerHandle);
		OwnerPlayer->GetWorldTimerManager().SetTimer(
			OwnerPlayer->GuardBlockReleaseIntoGuardEndTimerHandle,
			OwnerPlayer.Get(),
			&AJunPlayer::CompleteGuardBlockReleaseIntoGuardEnd,
			GuardBlockReleaseBlendOutTime,
			false);
		return;
	}

	if (DefenseState == EJunDefenseState::Guarding)
	{
		BeginGuardEnd();
	}
}

void UJunPlayerDefenseComponent::UpdateDefense(float DeltaTime)
{
	if (!OwnerPlayer)
	{
		return;
	}

	UpdateGuardRestartBlend(DeltaTime);

	const float DefenseTimelineDeltaTime = DeltaTime * OwnerPlayer->GetCurrentDefenseTimelineRate();

	// Tick owns the delayed movement release after GuardEnd.
	if (GuardMoveBlendRemainTime > 0.f)
	{
		GuardMoveBlendRemainTime = FMath::Max(0.f, GuardMoveBlendRemainTime - DeltaTime);
	}

	if (GuardExitMoveReleaseRemainTime > 0.f)
	{
		GuardExitMoveReleaseRemainTime =
			FMath::Max(0.f, GuardExitMoveReleaseRemainTime - DeltaTime);

		if (GuardExitMoveReleaseRemainTime <= 0.f)
		{
			OwnerPlayer->bDeferGuardMoveInput = false;
			OwnerPlayer->DesiredMoveForward = OwnerPlayer->PendingMoveForward;
			OwnerPlayer->DesiredMoveRight = OwnerPlayer->PendingMoveRight;
			ClearDefenseBlockTags();
		}
	}

	if (DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Ending)
	{
		DefenseTransitionElapsedTime += DefenseTimelineDeltaTime;

		if (DefenseState == EJunDefenseState::Ending &&
			PendingDefenseTransitionActionRequest.IsSet())
		{
			TryProcessBufferedDefenseTransitionCancelAction();
		}
	}
	else
	{
		DefenseTransitionElapsedTime = 0.f;
	}

	if (DefenseState == EJunDefenseState::Starting && !bDeflectResolveReceived)
	{
		if (bHoldRequestedForCurrentDeflect)
		{
			CurrentDeflectHeldDuration += DeltaTime;
		}

		DeflectResolveRemainTime =
			FMath::Max(0.f, DeflectResolveRemainTime - DefenseTimelineDeltaTime);

		if (DeflectResolveRemainTime <= 0.f)
		{
			ResolveCurrentDeflectAttempt();
		}
	}

	if (DefenseState == EJunDefenseState::Ending && GuardEndFinishRemainTime > 0.f)
	{
		if (GuardEndBaseReleaseRemainTime > 0.f)
		{
			GuardEndBaseReleaseRemainTime =
				FMath::Max(0.f, GuardEndBaseReleaseRemainTime - DefenseTimelineDeltaTime);

			if (GuardEndBaseReleaseRemainTime <= 0.f)
			{
				bKeepGuardBaseWhileEnding = false;
			}
		}

		GuardEndFinishRemainTime =
			FMath::Max(0.f, GuardEndFinishRemainTime - DefenseTimelineDeltaTime);

		if (GuardEndFinishRemainTime <= 0.f)
		{
			if (bAirParrySequenceActive)
			{
				FinishDefense();
				return;
			}

			OwnerPlayer->DesiredMoveForward = 0.f;
			OwnerPlayer->DesiredMoveRight = 0.f;
			OwnerPlayer->PendingMoveForward = 0.f;
			OwnerPlayer->PendingMoveRight = 0.f;

			if (PendingDefenseTransitionActionRequest.IsSet())
			{
				TryProcessBufferedDefenseTransitionCancelAction();
			}
			else
			{
				FinishDefense();
			}

			return;
		}
	}

	if (DefenseState == EJunDefenseState::None &&
		PostDefenseTransitionCancelBufferRemainTime > 0.f)
	{
		PostDefenseTransitionCancelBufferRemainTime =
			FMath::Max(0.f, PostDefenseTransitionCancelBufferRemainTime - DeltaTime);

		if (PendingDefenseTransitionActionRequest.IsSet())
		{
			ProcessBufferedDefenseTransitionCancelAction();
			return;
		}
	}

	if (ParrySpamPenaltyResetRemainTime > 0.f)
	{
		ParrySpamPenaltyResetRemainTime =
			FMath::Max(0.f, ParrySpamPenaltyResetRemainTime - DeltaTime);

		if (ParrySpamPenaltyResetRemainTime <= 0.f)
		{
			ParrySpamPenaltyStack = 0;
		}
	}

	if (bParryWindowOpen)
	{
		ParryWindowRemainTime = FMath::Max(0.f, ParryWindowRemainTime - DeltaTime);

		if (ParryWindowRemainTime <= 0.f)
		{
			bParryWindowOpen = false;
		}
	}
}

void UJunPlayerDefenseComponent::OpenParryWindow(bool bCountAsParryAttempt)
{
	if (!OwnerPlayer)
	{
		return;
	}

	if (bCountAsParryAttempt)
	{
		if (ParrySpamPenaltyResetRemainTime <= 0.f)
		{
			ParrySpamPenaltyStack = 0;
		}
		else
		{
			++ParrySpamPenaltyStack;
		}

		ParrySpamPenaltyResetRemainTime = ParrySpamPenaltyResetDuration;
	}

	const float PenaltyScale = FMath::Clamp(
		1.f - static_cast<float>(ParrySpamPenaltyStack) * ParrySpamPenaltyPerStack,
		0.f,
		1.f);

	CurrentParryWindowDuration =
		FMath::Max(MinParryWindowDuration, DefaultParryWindowDuration * PenaltyScale);
	CurrentPerfectParryWindowDuration = FMath::Min(
		CurrentParryWindowDuration,
		FMath::Max(MinPerfectParryWindowDuration, PerfectParryWindowDuration * PenaltyScale));

	bParryWindowOpen = CurrentParryWindowDuration > KINDA_SMALL_NUMBER;
	ParryWindowRemainTime = CurrentParryWindowDuration;
}

void UJunPlayerDefenseComponent::ResetParrySpamPenalty()
{
	if (!OwnerPlayer)
	{
		return;
	}

	ParrySpamPenaltyStack = 0;
	ParrySpamPenaltyResetRemainTime = 0.f;
}

void UJunPlayerDefenseComponent::CloseParryWindow()
{
	bParryWindowOpen = false;
	ParryWindowRemainTime = 0.f;
}

void UJunPlayerDefenseComponent::StartDefenseSequence()
{
	if (!OwnerPlayer)
	{
		return;
	}

	if (!OwnerPlayer->CanRequestActionState(EJunPlayerActionState::Defense))
	{
		return;
	}

	OwnerPlayer->SetActionState(EJunPlayerActionState::Defense, EJunActionTransitionReason::NormalInput);

	// Every tap/press starts a fresh deflect attempt from frame 0 of GuardStart.
	CurrentDefensePlayRate = FMath::Max(GuardStartPlayRate, KINDA_SMALL_NUMBER);
	OpenParryWindow(true);
	ResetDefenseTransitionRuntime();
	bHoldRequestedForCurrentDeflect = bDefenseButtonHeld;
	DeflectResolveRemainTime =
		(OwnerPlayer->GuardStartMontage
			? OwnerPlayer->GuardStartMontage->GetPlayLength()
			: DefaultDeflectResolveTime);
	bKeepGuardBaseWhileEnding = false;
	GuardExitMoveReleaseRemainTime = 0.f;
	bAirParrySequenceActive = false;
	ResetGuardRestartAnchor();
	OwnerPlayer->bDeferGuardMoveInput = true;
	OwnerPlayer->DesiredMoveForward = 0.f;
	OwnerPlayer->DesiredMoveRight = 0.f;
	OwnerPlayer->PendingMoveForward = 0.f;
	OwnerPlayer->PendingMoveRight = 0.f;

	ApplyDefenseStartBlockTags();

	if (OwnerPlayer->AnimInstance)
	{
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardEndMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardEndMontageEnded);

		if (OwnerPlayer->GuardEndMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.05f, OwnerPlayer->GuardEndMontage);
		}

		if (OwnerPlayer->AirGuardStartMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.02f, OwnerPlayer->AirGuardStartMontage);
		}

		if (OwnerPlayer->AirGuardEndMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.05f, OwnerPlayer->AirGuardEndMontage);
		}
	}

	DefenseState = EJunDefenseState::Starting;
	++GuardStartRestartSerial;
}

void UJunPlayerDefenseComponent::StartAirParrySequence()
{
	if (!OwnerPlayer)
	{
		return;
	}

	CurrentDefensePlayRate = FMath::Max(AirGuardStartPlayRate, KINDA_SMALL_NUMBER);
	OpenParryWindow(true);
	ResetDefenseTransitionRuntime();
	bHoldRequestedForCurrentDeflect = false;
	DeflectResolveRemainTime =
		(OwnerPlayer->AirGuardStartMontage
			? OwnerPlayer->AirGuardStartMontage->GetPlayLength()
			: DefaultDeflectResolveTime);
	bKeepGuardBaseWhileEnding = false;
	GuardExitMoveReleaseRemainTime = 0.f;
	GuardMoveBlendRemainTime = 0.f;
	OwnerPlayer->bDeferGuardMoveInput = false;
	bAirParrySequenceActive = true;
	if (!OwnerPlayer->AnimInstance ||
		!OwnerPlayer->AirGuardStartMontage ||
		!OwnerPlayer->AnimInstance->Montage_IsPlaying(OwnerPlayer->AirGuardStartMontage))
	{
		ResetGuardRestartAnchor();
	}
	OwnerPlayer->DesiredMoveForward = OwnerPlayer->PendingMoveForward;
	OwnerPlayer->DesiredMoveRight = OwnerPlayer->PendingMoveRight;

	ApplyAirDefenseStartBlockTags();

	if (OwnerPlayer->AnimInstance)
	{
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardEndMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardEndMontageEnded);

		if (OwnerPlayer->GuardStartMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.02f, OwnerPlayer->GuardStartMontage);
		}

		if (OwnerPlayer->GuardEndMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.05f, OwnerPlayer->GuardEndMontage);
		}

		if (OwnerPlayer->AirGuardEndMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(0.05f, OwnerPlayer->AirGuardEndMontage);
		}
	}

	DefenseState = EJunDefenseState::Starting;
	if (!OwnerPlayer->AnimInstance || !OwnerPlayer->AirGuardStartMontage)
	{
		BeginAirGuardEnd();
		return;
	}

	OwnerPlayer->AnimInstance->OnMontageEnded.AddDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardStartMontageEnded);
	if (OwnerPlayer->AnimInstance->Montage_IsPlaying(OwnerPlayer->AirGuardStartMontage))
	{
		OwnerPlayer->AnimInstance->Montage_SetPlayRate(
			OwnerPlayer->AirGuardStartMontage,
			CurrentDefensePlayRate);
		if (bGuardRestartAnchorReached &&
			GuardRestartAnchorMontage == OwnerPlayer->AirGuardStartMontage)
		{
			FinishGuardRestartBlend(false);
			OwnerPlayer->AnimInstance->Montage_SetPosition(
				OwnerPlayer->AirGuardStartMontage,
				GuardRestartAnchorPosition);
		}
	}
	else
	{
		OwnerPlayer->AnimInstance->Montage_Play(
			OwnerPlayer->AirGuardStartMontage,
			CurrentDefensePlayRate);
	}
}

void UJunPlayerDefenseComponent::BeginDefenseFromBufferedInput()
{
	if (!OwnerPlayer)
	{
		return;
	}

	if (OwnerPlayer->GetCharacterMovement()->IsFalling())
	{
		StartAirParrySequence();
		return;
	}

	if (OwnerPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		return;
	}

	StartDefenseSequence();
}

void UJunPlayerDefenseComponent::ResolveCurrentDeflectAttempt()
{
	if (!OwnerPlayer ||
		DefenseState != EJunDefenseState::Starting ||
		bDeflectResolveReceived)
	{
		return;
	}

	bDeflectResolveReceived = true;

	if (bAirParrySequenceActive)
	{
		BeginAirGuardEnd();
		return;
	}

	const bool bShouldEnterGuardLoop =
		bHoldRequestedForCurrentDeflect &&
		CurrentDeflectHeldDuration >= GuardEnterHoldThreshold;

	if (bShouldEnterGuardLoop)
	{
		EnterGuardLoop();
		return;
	}
	BeginGuardEnd();
}

void UJunPlayerDefenseComponent::ResetDefenseTransitionRuntime(bool bClearBufferedCancelAction)
{
	if (!OwnerPlayer)
	{
		return;
	}

	if (bClearBufferedCancelAction)
	{
		PendingDefenseTransitionActionRequest.Reset();
	}
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	GuardEndFinishRemainTime = 0.f;
	GuardEndBaseReleaseRemainTime = 0.f;
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	DefenseTransitionElapsedTime = 0.f;
}

void UJunPlayerDefenseComponent::ApplyDefenseStartBlockTags()
{
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void UJunPlayerDefenseComponent::ApplyAirDefenseStartBlockTags()
{
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void UJunPlayerDefenseComponent::ApplyGuardOnBlockTags()
{
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->AddGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void UJunPlayerDefenseComponent::ClearDefenseBlockTags()
{
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Move);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Jump);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Attack);
	OwnerPlayer->RemoveGameplayTag(JunGameplayTags::State_Block_Dodge);
}

void UJunPlayerDefenseComponent::EnterGuardLoop()
{
	if (!OwnerPlayer)
	{
		return;
	}

	// Guarding is the only sustained block state. Start/End remain montage-driven.
	DefenseState = EJunDefenseState::Guarding;
	OwnerPlayer->SetActionState(EJunPlayerActionState::Defense, EJunActionTransitionReason::System);
	CurrentDefensePlayRate = 1.f;
	CloseParryWindow();
	ResetDefenseTransitionRuntime();
	OwnerPlayer->bDeferGuardMoveInput = false;
	OwnerPlayer->DesiredMoveForward = OwnerPlayer->PendingMoveForward;
	OwnerPlayer->DesiredMoveRight = OwnerPlayer->PendingMoveRight;
	GuardMoveBlendRemainTime = GuardMoveBlendDuration;
	ApplyGuardOnBlockTags();
}

void UJunPlayerDefenseComponent::BeginGuardEnd()
{
	if (!OwnerPlayer ||
		DefenseState == EJunDefenseState::None ||
		DefenseState == EJunDefenseState::Ending)
	{
		return;
	}

	DefenseState = EJunDefenseState::Ending;
	CloseParryWindow();
	CurrentDefensePlayRate = FMath::Max(GuardEndPlayRate, KINDA_SMALL_NUMBER);
	ResetDefenseTransitionRuntime(false);
	GuardEndFinishRemainTime =
		OwnerPlayer->GuardEndMontage
			? FMath::Max(0.f, OwnerPlayer->GuardEndMontage->GetPlayLength() - GuardEndFinishTimeOffset)
			: FMath::Max(0.f, GuardEndStateDuration);
	GuardEndBaseReleaseRemainTime =
		OwnerPlayer->GuardEndMontage
			? FMath::Max(0.f, OwnerPlayer->GuardEndMontage->GetPlayLength() - GuardEndBaseReleaseTimeOffset)
			: FMath::Max(0.f, GuardEndStateDuration);
	bKeepGuardBaseWhileEnding = true;
	OwnerPlayer->bDeferGuardMoveInput = true;
	OwnerPlayer->DesiredMoveForward = 0.f;
	OwnerPlayer->DesiredMoveRight = 0.f;
	GuardMoveBlendRemainTime = GuardMoveBlendDuration;
	ApplyDefenseStartBlockTags();

	if (GuardEndFinishRemainTime <= 0.f)
	{
		FinishDefense();
		return;
	}

	if (OwnerPlayer->AnimInstance)
	{
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardEndMontageEnded);
	}

	// Ground guard end visuals are driven by the player ABP state machine.
	if (PendingDefenseTransitionActionRequest.IsSet())
	{
		TryProcessBufferedDefenseTransitionCancelAction();
	}
}

void UJunPlayerDefenseComponent::BeginAirGuardEnd()
{
	if (!OwnerPlayer ||
		DefenseState == EJunDefenseState::None ||
		DefenseState == EJunDefenseState::Ending)
	{
		return;
	}

	CloseParryWindow();
	bHoldRequestedForCurrentDeflect = false;
	bDeflectResolveReceived = false;
	DeflectResolveRemainTime = 0.f;
	CurrentDeflectHeldDuration = 0.f;
	OwnerPlayer->bDeferGuardMoveInput = false;
	OwnerPlayer->DesiredMoveForward = OwnerPlayer->PendingMoveForward;
	OwnerPlayer->DesiredMoveRight = OwnerPlayer->PendingMoveRight;

	if (OwnerPlayer->AnimInstance)
	{
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardEndMontageEnded);
	}

	FinishDefense();
}

void UJunPlayerDefenseComponent::FinishDefense()
{
	if (!OwnerPlayer)
	{
		return;
	}

	const bool bWasAirParrySequenceActive = bAirParrySequenceActive;
	DefenseState = EJunDefenseState::None;
	CurrentDefensePlayRate = 1.f;
	CloseParryWindow();
	ResetDefenseTransitionRuntime();
	bKeepGuardBaseWhileEnding = false;
	OwnerPlayer->bDeferGuardMoveInput = true;
	PostDefenseTransitionCancelBufferRemainTime = PostDefenseTransitionCancelBufferDuration;
	bAirParrySequenceActive = false;

	if (bWasAirParrySequenceActive)
	{
		OwnerPlayer->bDeferGuardMoveInput = false;
		GuardMoveBlendRemainTime = 0.f;
		GuardExitMoveReleaseRemainTime = 0.f;
		PostDefenseTransitionCancelBufferRemainTime = 0.f;
		ClearDefenseBlockTags();
		OwnerPlayer->DesiredMoveForward = OwnerPlayer->PendingMoveForward;
		OwnerPlayer->DesiredMoveRight = OwnerPlayer->PendingMoveRight;
		OwnerPlayer->ClearActionStateIf(EJunPlayerActionState::Defense, EJunActionTransitionReason::System);
		return;
	}

	GuardMoveBlendRemainTime = GuardExitMoveBlendDuration;
	GuardExitMoveReleaseRemainTime = GuardExitMoveReleaseDelay;
	OwnerPlayer->ClearActionStateIf(EJunPlayerActionState::Defense, EJunActionTransitionReason::System);
}

void UJunPlayerDefenseComponent::FinishDefenseForCancel()
{
	if (!OwnerPlayer)
	{
		return;
	}

	DefenseState = EJunDefenseState::None;
	CurrentDefensePlayRate = 1.f;
	CloseParryWindow();
	ResetDefenseTransitionRuntime();
	bKeepGuardBaseWhileEnding = false;
	OwnerPlayer->bDeferGuardMoveInput = false;
	bAirParrySequenceActive = false;
	OwnerPlayer->bGuardBlockReleasePending = false;
	ResetGuardRestartAnchor();
	GuardExitMoveReleaseRemainTime = 0.f;
	GuardMoveBlendRemainTime = 0.f;
	ClearDefenseBlockTags();
	OwnerPlayer->DesiredMoveForward = OwnerPlayer->PendingMoveForward;
	OwnerPlayer->DesiredMoveRight = OwnerPlayer->PendingMoveRight;
	OwnerPlayer->ClearActionStateIf(EJunPlayerActionState::Defense, EJunActionTransitionReason::System);
}

bool UJunPlayerDefenseComponent::TryProcessBufferedDefenseTransitionCancelAction()
{
	const bool bCanExecute =
		(DefenseState == EJunDefenseState::Starting ||
		 DefenseState == EJunDefenseState::Ending) ||
		PostDefenseTransitionCancelBufferRemainTime > 0.f;

	if (!bCanExecute)
	{
		return false;
	}

	ProcessBufferedDefenseTransitionCancelAction();
	return true;
}

void UJunPlayerDefenseComponent::ProcessBufferedDefenseTransitionCancelAction()
{
	if (!OwnerPlayer ||
		!PendingDefenseTransitionActionRequest.IsSet())
	{
		return;
	}

	if (!OwnerPlayer->TryProcessPendingDefenseTransitionActionRequest(PendingDefenseTransitionActionRequest))
	{
		return;
	}
}

void UJunPlayerDefenseComponent::SetBufferedDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction Action)
{
	PendingDefenseTransitionActionRequest = OwnerPlayer
		? OwnerPlayer->MakeDefensePendingActionRequest(Action)
		: FJunPendingActionRequest();
}

void UJunPlayerDefenseComponent::ConsumeBufferedDefenseTransitionRequest()
{
	PendingDefenseTransitionActionRequest.Reset();
	PostDefenseTransitionCancelBufferRemainTime = 0.f;
}

void UJunPlayerDefenseComponent::ClearDefenseInputBuffers()
{
	PendingDefenseTransitionActionRequest.Reset();
	bDefenseButtonHeld = false;
	bHoldRequestedForCurrentDeflect = false;
}

void UJunPlayerDefenseComponent::NotifyGuardRestartAnchorReached(UAnimMontage* Montage, float AnchorPosition)
{
	if (!OwnerPlayer || !Montage)
	{
		return;
	}

	if (Montage != OwnerPlayer->GuardStartMontage && Montage != OwnerPlayer->AirGuardStartMontage)
	{
		return;
	}

	GuardRestartAnchorMontage = Montage;
	GuardRestartAnchorPosition = FMath::Max(0.f, AnchorPosition);
	bGuardRestartAnchorReached = true;

	OwnerPlayer->GuardRestartAnchorMontage = GuardRestartAnchorMontage;
	OwnerPlayer->GuardRestartAnchorPosition = GuardRestartAnchorPosition;
	OwnerPlayer->bGuardRestartAnchorReached = bGuardRestartAnchorReached;
}

void UJunPlayerDefenseComponent::StartGuardRestartBlend(UAnimMontage* Montage, float TargetPosition, float BlendDuration)
{
	if (!OwnerPlayer || !OwnerPlayer->AnimInstance || !Montage || !OwnerPlayer->AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	const float CurrentPosition = OwnerPlayer->AnimInstance->Montage_GetPosition(Montage);
	const float ClampedTargetPosition = FMath::Clamp(TargetPosition, 0.f, Montage->GetPlayLength());
	if (BlendDuration <= KINDA_SMALL_NUMBER || FMath::IsNearlyEqual(CurrentPosition, ClampedTargetPosition, KINDA_SMALL_NUMBER))
	{
		OwnerPlayer->AnimInstance->Montage_SetPosition(Montage, ClampedTargetPosition);
		OwnerPlayer->AnimInstance->Montage_SetPlayRate(Montage, CurrentDefensePlayRate);
		FinishGuardRestartBlend(false);
		return;
	}

	GuardRestartBlendMontage = Montage;
	GuardRestartBlendStartPosition = CurrentPosition;
	GuardRestartBlendTargetPosition = ClampedTargetPosition;
	GuardRestartBlendElapsedTime = 0.f;
	GuardRestartBlendDuration = BlendDuration;
	GuardRestartBlendRestorePlayRate = CurrentDefensePlayRate;
	bGuardRestartBlendActive = true;

	OwnerPlayer->GuardRestartBlendMontage = GuardRestartBlendMontage;
	OwnerPlayer->GuardRestartBlendStartPosition = GuardRestartBlendStartPosition;
	OwnerPlayer->GuardRestartBlendTargetPosition = GuardRestartBlendTargetPosition;
	OwnerPlayer->GuardRestartBlendElapsedTime = GuardRestartBlendElapsedTime;
	OwnerPlayer->GuardRestartBlendDuration = GuardRestartBlendDuration;
	OwnerPlayer->GuardRestartBlendRestorePlayRate = GuardRestartBlendRestorePlayRate;
	OwnerPlayer->bGuardRestartBlendActive = bGuardRestartBlendActive;

	OwnerPlayer->AnimInstance->Montage_SetPlayRate(Montage, 0.f);
}

void UJunPlayerDefenseComponent::UpdateGuardRestartBlend(float DeltaTime)
{
	if (!bGuardRestartBlendActive)
	{
		return;
	}

	UAnimMontage* Montage = GuardRestartBlendMontage.Get();
	if (!OwnerPlayer || !OwnerPlayer->AnimInstance || !Montage || !OwnerPlayer->AnimInstance->Montage_IsPlaying(Montage))
	{
		FinishGuardRestartBlend(false);
		return;
	}

	GuardRestartBlendElapsedTime = FMath::Min(
		GuardRestartBlendDuration,
		GuardRestartBlendElapsedTime + DeltaTime);
	const float Alpha = GuardRestartBlendDuration > KINDA_SMALL_NUMBER
		? GuardRestartBlendElapsedTime / GuardRestartBlendDuration
		: 1.f;
	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	const float NewPosition = FMath::Lerp(
		GuardRestartBlendStartPosition,
		GuardRestartBlendTargetPosition,
		SmoothAlpha);
	OwnerPlayer->AnimInstance->Montage_SetPosition(Montage, NewPosition);

	OwnerPlayer->GuardRestartBlendElapsedTime = GuardRestartBlendElapsedTime;

	if (GuardRestartBlendElapsedTime >= GuardRestartBlendDuration)
	{
		FinishGuardRestartBlend(true);
	}
}

void UJunPlayerDefenseComponent::FinishGuardRestartBlend(bool bSnapToTarget)
{
	UAnimMontage* Montage = GuardRestartBlendMontage.Get();
	if (OwnerPlayer && OwnerPlayer->AnimInstance && Montage && OwnerPlayer->AnimInstance->Montage_IsPlaying(Montage))
	{
		if (bSnapToTarget)
		{
			OwnerPlayer->AnimInstance->Montage_SetPosition(Montage, GuardRestartBlendTargetPosition);
		}

		OwnerPlayer->AnimInstance->Montage_SetPlayRate(Montage, GuardRestartBlendRestorePlayRate);
	}

	GuardRestartBlendMontage = nullptr;
	GuardRestartBlendStartPosition = 0.f;
	GuardRestartBlendTargetPosition = 0.f;
	GuardRestartBlendElapsedTime = 0.f;
	GuardRestartBlendDuration = 0.f;
	GuardRestartBlendRestorePlayRate = 1.f;
	bGuardRestartBlendActive = false;

	if (OwnerPlayer)
	{
		OwnerPlayer->GuardRestartBlendMontage = nullptr;
		OwnerPlayer->GuardRestartBlendStartPosition = 0.f;
		OwnerPlayer->GuardRestartBlendTargetPosition = 0.f;
		OwnerPlayer->GuardRestartBlendElapsedTime = 0.f;
		OwnerPlayer->GuardRestartBlendDuration = 0.f;
		OwnerPlayer->GuardRestartBlendRestorePlayRate = 1.f;
		OwnerPlayer->bGuardRestartBlendActive = false;
	}
}

void UJunPlayerDefenseComponent::ResetGuardRestartAnchor()
{
	FinishGuardRestartBlend(false);
	GuardRestartAnchorMontage = nullptr;
	GuardRestartAnchorPosition = 0.f;
	bGuardRestartAnchorReached = false;

	if (OwnerPlayer)
	{
		OwnerPlayer->GuardRestartAnchorMontage = nullptr;
		OwnerPlayer->GuardRestartAnchorPosition = 0.f;
		OwnerPlayer->bGuardRestartAnchorReached = false;
	}
}

void UJunPlayerDefenseComponent::CancelDefenseForCancelTransition(float BlendOutTime)
{
	if (!OwnerPlayer ||
		DefenseState == EJunDefenseState::None ||
		DefenseState == EJunDefenseState::Guarding)
	{
		return;
	}

	const float ResolvedBlendOutTime =
		BlendOutTime >= 0.f ? BlendOutTime : DefenseCancelBlendOutTime;

	if (OwnerPlayer->AnimInstance)
	{
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnGuardEndMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardStartMontageEnded);
		OwnerPlayer->AnimInstance->OnMontageEnded.RemoveDynamic(OwnerPlayer.Get(), &AJunPlayer::OnAirGuardEndMontageEnded);

		if (OwnerPlayer->GuardStartMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(ResolvedBlendOutTime, OwnerPlayer->GuardStartMontage);
		}

		if (OwnerPlayer->GuardEndMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(ResolvedBlendOutTime, OwnerPlayer->GuardEndMontage);
		}

		if (OwnerPlayer->AirGuardStartMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(ResolvedBlendOutTime, OwnerPlayer->AirGuardStartMontage);
		}

		if (OwnerPlayer->AirGuardEndMontage)
		{
			OwnerPlayer->AnimInstance->Montage_Stop(ResolvedBlendOutTime, OwnerPlayer->AirGuardEndMontage);
		}
	}

	FinishDefenseForCancel();
}

void UJunPlayerDefenseComponent::SetDefenseHoldIntent(bool bHoldRequested)
{
	bDefenseButtonHeld = bHoldRequested;
	bHoldRequestedForCurrentDeflect = bHoldRequested;
}

bool UJunPlayerDefenseComponent::TryHandleDefenseStartedFromHitState()
{
	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::ParrySuccess)
	{
		SetDefenseHoldIntent(true);
		if (OwnerPlayer->ChainParryWindowRemainTime > 0.f)
		{
			OwnerPlayer->OpenParryWindow(true);
		}

		if (OwnerPlayer->ParrySuccessElapsedTime >= OwnerPlayer->ParrySuccessDefenseCancelOpenTime)
		{
			if (OwnerPlayer->ParrySuccessDefenseHoldElapsedTime >= OwnerPlayer->ParrySuccessDefenseHoldConfirmTime)
			{
				OwnerPlayer->CancelParrySuccessForCancelTransition(OwnerPlayer->ParrySuccessDefenseCancelBlendOutTime);
				EnterGuardLoop();
			}
		}
		else
		{
			OwnerPlayer->ParrySuccessDefenseHoldElapsedTime = 0.f;
		}
		return true;
	}

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::GuardBlock)
	{
		// Re-pressing during a guard hit only refreshes the hold intent.
		// A new GuardStart would overlap the active block reaction.
		SetDefenseHoldIntent(true);
		return true;
	}

	if (OwnerPlayer->CurrentHitState == EJunPlayerHitState::HitReact)
	{
		SetDefenseHoldIntent(true);
		OwnerPlayer->TryCancelHitReactIntoDefense();
		return true;
	}

	return OwnerPlayer->CurrentHitState == EJunPlayerHitState::GuardBreak;
}

bool UJunPlayerDefenseComponent::TryHandleDefenseStartedFromActionState()
{
	if (OwnerPlayer->bIsDodgeAttacking)
	{
		SetDefenseHoldIntent(true);
		OwnerPlayer->TryCancelDodgeAttackIntoDefense();
		return true;
	}

	if (OwnerPlayer->bIsJumpAttacking && OwnerPlayer->JumpAttackState == EJunJumpAttackState::End)
	{
		return !OwnerPlayer->TryCancelJumpAttackEndIntoDefense();
	}

	return false;
}

bool UJunPlayerDefenseComponent::TryHandleDefenseStartedFromAttackState()
{
	if (OwnerPlayer->bIsHeavyAttacking)
	{
		SetDefenseHoldIntent(true);
		OwnerPlayer->BufferHeavyAttackRecoveryAction(EJunBufferedRecoveryAction::Defense);
		return true;
	}

	if (OwnerPlayer->bIsJigenAttacking)
	{
		SetDefenseHoldIntent(true);
		return true;
	}

	return false;
}

void UJunPlayerDefenseComponent::StartDefenseFromNeutralInput()
{
	if (OwnerPlayer->GetCharacterMovement()->IsFalling())
	{
		bDefenseButtonHeld = true;
		bHoldRequestedForCurrentDeflect = false;
		StartAirParrySequence();
		return;
	}

	if (OwnerPlayer->HasGameplayTag(JunGameplayTags::State_Action_Dodge))
	{
		SetDefenseHoldIntent(true);
		if (!OwnerPlayer->TryCancelDodgeIntoDefense())
		{
			bHoldRequestedForCurrentDeflect = false;
		}
		return;
	}

	// Defense is split into:
	// None -> Starting(deflect attempt) -> Guarding(block) -> Ending -> None.
	// Repeated taps restart the deflect attempt immediately, while hold transitions into Guarding
	// only after GuardStart finishes and the button is still held.
	SetDefenseHoldIntent(true);
	if (OwnerPlayer->bIsBasicAttacking ||
		OwnerPlayer->CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Defense))
	{
		if (OwnerPlayer->CanCancelBasicAttackIntoRecoveryAction(EJunBufferedRecoveryAction::Defense))
		{
			OwnerPlayer->BufferBasicAttackRecoveryAction(EJunBufferedRecoveryAction::Defense);
		}
		return;
	}

	if (DefenseState == EJunDefenseState::Guarding)
	{
		return;
	}

	if (DefenseState == EJunDefenseState::Starting ||
		DefenseState == EJunDefenseState::Ending)
	{
		StartDefenseSequence();
		return;
	}

	StartDefenseSequence();
}
