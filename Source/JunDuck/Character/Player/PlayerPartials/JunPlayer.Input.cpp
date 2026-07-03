#include "Character/Player/JunPlayer.h"

#include "Character/Player/PlayerComponent/JunPlayerDefenseComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerExecutionComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#include "Interface/JunExecutionTargetInterface.h"
#include "JunGameplayTags.h"
#pragma region Gameplay Events / Attack Trace / Input Entry
// ============================================================================
// Gameplay Events / Attack Trace / Input Entry
// ============================================================================

void AJunPlayer::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	Super::HandleGameplayEventNotify(EventTag);

	if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_BasicAttack_ComboWindow))
	{
		OnBasicAttackComboWindowBegin();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_HeavyAttack_ComboWindow))
	{
		OnHeavyAttackComboWindowBegin();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Jigen_ComboWindow))
	{
		OnJigenComboWindowBegin();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Dodge_Start))
	{
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Defense_ParryWindowBegin))
	{
		// Code-driven parry timing.
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Defense_ParryWindowEnd))
	{
		// Code-driven parry timing.
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Defense_EndBaseRelease))
	{
		// GuardEnd base release is now code-driven.
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Execution_Apply))
	{
		AdvanceExecutionCameraStage();
		if (bCurrentExecutionIsFinish && bLockOnActive)
		{
			EndLockOn();
			if (IsExecuting() && ExecutionCameraTarget)
			{
				CameraMode = EJunCameraMode::Execution;
			}
		}
		if (IsExecuting() && ExecutionComponent && ExecutionComponent->GetCurrentExecutionTarget())
		{
			ExecutionComponent->TriggerExecutionTargetMontage();
		}
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Execution_FinishCameraPullIn))
	{
		AdvanceExecutionFinishCameraPullInStage();
		AActor* CurrentExecutionTarget = ExecutionComponent ? ExecutionComponent->GetCurrentExecutionTarget() : nullptr;
		if (bCurrentExecutionIsFinish && IsExecuting() && CurrentExecutionTarget)
		{
			if (IJunExecutionTargetInterface* ExecutionTarget = Cast<IJunExecutionTargetInterface>(CurrentExecutionTarget))
			{
				ExecutionTarget->ApplyPendingExecutionResult();
			}
		}
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Execution_CameraRestore))
	{
		EndExecutionCamera();
	}
	else if (EventTag.MatchesTagExact(JunGameplayTags::Event_Notify_Death_Present))
	{
		TriggerPendingDeathPresentation();
	}
}

void AJunPlayer::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, const FJunAttackDefenseRuleData& DefenseRuleData, EJunWeaponNiagaraComponent NiagaraComponent, const FJunAttackTraceOverrideData& TraceOverrideData)
{
	if (!EquipmentComponent || !EquipmentComponent->GetEquippedWeapon())
	{
		return;
	}

	FJunAttackDefenseRuleData ResolvedDefenseRuleData = DefenseRuleData;
	if (bJumpCounterFollowUpDefenseBypassAvailable && bIsJumpAttacking)
	{
		ResolvedDefenseRuleData.bCanBeParried = false;
	}

	Super::BeginAttackTraceWindow(HitReactType, DamageData, DefenseKnockbackData, ResolvedDefenseRuleData, NiagaraComponent, TraceOverrideData);

	EquipmentComponent->BeginAttackTrace(
		HitReactType,
		DamageData,
		DefenseKnockbackData,
		ResolvedDefenseRuleData,
		NiagaraComponent,
		TraceOverrideData);
}

void AJunPlayer::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
	Super::EndAttackTraceWindow(NiagaraComponent);

	if (!EquipmentComponent || !EquipmentComponent->GetEquippedWeapon())
	{
		return;
	}

	EquipmentComponent->EndAttackTrace(NiagaraComponent);
}

void AJunPlayer::BeginWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquipmentComponent)
	{
		EquipmentComponent->ActivateWeaponNiagara(ComponentType);
	}
}

void AJunPlayer::EndWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
	if (EquipmentComponent)
	{
		EquipmentComponent->DeactivateWeaponNiagara(ComponentType);
	}
}

void AJunPlayer::OnDefenseStarted()
{
	if (DefenseComponent)
	{
		DefenseComponent->HandleDefenseStarted();
	}
}

void AJunPlayer::OnDefenseReleased()
{
	if (DefenseComponent)
	{
		DefenseComponent->HandleDefenseReleased();
	}
}

void AJunPlayer::AddCameraLookInput(const FVector2D& Input)
{
	PendingCameraLookInput = Input;
}

void AJunPlayer::SnapCameraToLookAt(const FVector& WorldTarget, float Pitch)
{
	if (!Controller)
	{
		bCameraRotationInitialized = false;
		return;
	}

	const FVector CameraBasePoint = GetActorLocation();
	FVector ToTarget = WorldTarget - CameraBasePoint;
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		ToTarget = GetActorForwardVector();
	}

	TargetCameraYaw = ToTarget.Rotation().Yaw;
	TargetCameraPitch = FMath::Clamp(Pitch, MinCameraPitch, MaxCameraPitch);
	PendingCameraLookInput = FVector2D::ZeroVector;
	Controller->SetControlRotation(FRotator(TargetCameraPitch, TargetCameraYaw, 0.f));
	bCameraRotationInitialized = true;
	StartCameraHardSnap();
}

void AJunPlayer::BeginDialogueCamera()
{
	bDialogueCameraActive = true;
	bDialogueCameraRestoring = false;
}

void AJunPlayer::EndDialogueCamera()
{
	bDialogueCameraActive = false;
	bDialogueCameraRestoring = true;
}

void AJunPlayer::ToggleLockOn()
{
	if (bLockOnActive)
	{
		EndLockOn();
		return;
	}

	AJunCharacter* NewTarget = FindBestLockOnTarget();
	if (!NewTarget)
	{
		return;
	}

	StartLockOn(NewTarget);
}

UAnimInstance* AJunPlayer::GetPlayerAnimInstance()
{
	if (!AnimInstance)
	{
		AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	}

	return AnimInstance;
}

#pragma endregion
