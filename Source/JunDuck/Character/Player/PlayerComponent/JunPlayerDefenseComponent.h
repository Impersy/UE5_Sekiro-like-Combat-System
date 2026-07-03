#pragma once

#include "CoreMinimal.h"
#include "Character/Player/JunPlayer.h"
#include "Components/ActorComponent.h"
#include "JunPlayerDefenseComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JUNDUCK_API UJunPlayerDefenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunPlayerDefenseComponent();

	virtual void BeginPlay() override;

	void HandleDefenseStarted();
	void HandleDefenseReleased();
	void UpdateDefense(float DeltaTime);
	void OpenParryWindow(bool bCountAsParryAttempt = true);
	void CloseParryWindow();
	void ResetParrySpamPenalty();
	void StartDefenseSequence();
	void StartAirParrySequence();
	void BeginDefenseFromBufferedInput();
	void ResolveCurrentDeflectAttempt();
	void ResetDefenseTransitionRuntime(bool bClearBufferedCancelAction = true);
	void ApplyDefenseStartBlockTags();
	void ApplyAirDefenseStartBlockTags();
	void ApplyGuardOnBlockTags();
	void ClearDefenseBlockTags();
	void EnterGuardLoop();
	void BeginGuardEnd();
	void BeginAirGuardEnd();
	void FinishDefense();
	void FinishDefenseForCancel();
	bool TryProcessBufferedDefenseTransitionCancelAction();
	void ProcessBufferedDefenseTransitionCancelAction();
	void SetBufferedDefenseTransitionCancelAction(EJunBufferedDefenseCancelAction Action);
	void ConsumeBufferedDefenseTransitionRequest();
	void ClearDefenseInputBuffers();
	void NotifyGuardRestartAnchorReached(class UAnimMontage* Montage, float AnchorPosition);
	void StartGuardRestartBlend(class UAnimMontage* Montage, float TargetPosition, float BlendDuration);
	void UpdateGuardRestartBlend(float DeltaTime);
	void FinishGuardRestartBlend(bool bSnapToTarget);
	void ResetGuardRestartAnchor();
	void CancelDefenseForCancelTransition(float BlendOutTime = -1.f);
	float GetGuardStartPlayRate() const { return GuardStartPlayRate; }
	float GetAirGuardStartPlayRate() const { return AirGuardStartPlayRate; }
	float GetGuardEndPlayRate() const { return GuardEndPlayRate; }
	float GetGuardEndStateDuration() const { return GuardEndStateDuration; }
	float GetGuardMoveBlendDuration() const { return GuardMoveBlendDuration; }
	float GetDefaultDeflectResolveTime() const { return DefaultDeflectResolveTime; }
	float GetGuardEnterHoldThreshold() const { return GuardEnterHoldThreshold; }
	float GetGuardEndFinishTimeOffset() const { return GuardEndFinishTimeOffset; }
	float GetGuardEndBaseReleaseTimeOffset() const { return GuardEndBaseReleaseTimeOffset; }
	float GetPostDefenseTransitionCancelBufferDuration() const { return PostDefenseTransitionCancelBufferDuration; }
	float GetGuardExitMoveBlendDuration() const { return GuardExitMoveBlendDuration; }
	float GetGuardExitMoveReleaseDelay() const { return GuardExitMoveReleaseDelay; }
	float GetGuardStartVisualRestartMinInterval() const { return GuardStartVisualRestartMinInterval; }
	float GetGuardStartRestartBlendDuration() const { return GuardStartRestartBlendDuration; }
	float GetAirGuardStartVisualRestartMinInterval() const { return AirGuardStartVisualRestartMinInterval; }
	float GetAirGuardStartRestartBlendDuration() const { return AirGuardStartRestartBlendDuration; }
	float GetAirGuardLandCancelBlendOutTime() const { return AirGuardLandCancelBlendOutTime; }
	float GetGuardStartToEndBlendOutTime() const { return GuardStartToEndBlendOutTime; }
	float GetGuardBlockReleaseBlendOutTime() const { return GuardBlockReleaseBlendOutTime; }
	float GetDefenseCancelBlendOutTime() const { return DefenseCancelBlendOutTime; }
	float GetDefenseMoveCancelBlendOutTime() const { return DefenseMoveCancelBlendOutTime; }
	EJunDefenseState GetDefenseState() const { return DefenseState; }
	int32 GetGuardStartRestartSerial() const { return GuardStartRestartSerial; }
	EJunBufferedDefenseCancelAction GetBufferedDefenseTransitionCancelAction() const
	{
		return PendingDefenseTransitionActionRequest.DefenseCancelAction;
	}
	float GetCurrentDefensePlayRate() const { return CurrentDefensePlayRate; }
	bool IsDefenseButtonHeld() const { return bDefenseButtonHeld; }
	bool IsAirParrySequenceActive() const { return bAirParrySequenceActive; }
	float GetGuardMoveBlendRemainTime() const { return GuardMoveBlendRemainTime; }
	float GetDefenseTransitionElapsedTime() const { return DefenseTransitionElapsedTime; }
	float GetGuardEndFinishRemainTime() const { return GuardEndFinishRemainTime; }
	float GetPostDefenseTransitionCancelBufferRemainTime() const { return PostDefenseTransitionCancelBufferRemainTime; }
	bool ShouldKeepGuardBaseWhileEnding() const { return bKeepGuardBaseWhileEnding; }
	bool IsParryWindowOpen() const { return bParryWindowOpen; }
	float GetParryWindowRemainTime() const { return ParryWindowRemainTime; }
	float GetCurrentParryWindowDuration() const { return CurrentParryWindowDuration; }
	float GetCurrentPerfectParryWindowDuration() const { return CurrentPerfectParryWindowDuration; }
	int32 GetParrySpamPenaltyStack() const { return ParrySpamPenaltyStack; }

private:
	void SetDefenseHoldIntent(bool bHoldRequested);
	bool TryHandleDefenseStartedFromHitState();
	bool TryHandleDefenseStartedFromActionState();
	bool TryHandleDefenseStartedFromAttackState();
	void StartDefenseFromNeutralInput();

	UPROPERTY()
	TObjectPtr<class AJunPlayer> OwnerPlayer = nullptr;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardStartPlayRate = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float AirGuardStartPlayRate = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardEndPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardEndStateDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardMoveBlendDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float DefaultDeflectResolveTime = 0.156f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardEnterHoldThreshold = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardEndFinishTimeOffset = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardEndBaseReleaseTimeOffset = 0.48f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float PostDefenseTransitionCancelBufferDuration = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardExitMoveBlendDuration = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Timeline")
	float GuardExitMoveReleaseDelay = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float GuardStartVisualRestartMinInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float GuardStartRestartBlendDuration = 0.035f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float AirGuardStartVisualRestartMinInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float AirGuardStartRestartBlendDuration = 0.035f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float AirGuardLandCancelBlendOutTime = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float GuardStartToEndBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float GuardBlockReleaseBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float DefenseCancelBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Blend")
	float DefenseMoveCancelBlendOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Parry")
	float DefaultParryWindowDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Parry")
	float PerfectParryWindowDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Parry")
	float ParrySpamPenaltyResetDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Parry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParrySpamPenaltyPerStack = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Parry")
	float MinParryWindowDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Parry")
	float MinPerfectParryWindowDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Parry")
	bool bParryWindowOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Parry")
	float ParryWindowRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Parry")
	float CurrentParryWindowDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Parry")
	float CurrentPerfectParryWindowDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Parry")
	float ParrySpamPenaltyResetRemainTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Parry")
	int32 ParrySpamPenaltyStack = 0;

	EJunDefenseState DefenseState = EJunDefenseState::None;
	int32 GuardStartRestartSerial = 0;
	FJunPendingActionRequest PendingDefenseTransitionActionRequest;
	float CurrentDefensePlayRate = 1.f;
	bool bDefenseButtonHeld = false;
	bool bHoldRequestedForCurrentDeflect = false;
	bool bDeflectResolveReceived = false;
	float DeflectResolveRemainTime = 0.f;
	float CurrentDeflectHeldDuration = 0.f;
	float DefenseTransitionElapsedTime = 0.f;
	float GuardEndFinishRemainTime = 0.f;
	float GuardEndBaseReleaseRemainTime = 0.f;
	float PostDefenseTransitionCancelBufferRemainTime = 0.f;
	bool bKeepGuardBaseWhileEnding = false;
	float GuardMoveBlendRemainTime = 0.f;
	float GuardExitMoveReleaseRemainTime = 0.f;
	bool bAirParrySequenceActive = false;
	TObjectPtr<class UAnimMontage> GuardRestartAnchorMontage = nullptr;
	float GuardRestartAnchorPosition = 0.f;
	bool bGuardRestartAnchorReached = false;
	TObjectPtr<class UAnimMontage> GuardRestartBlendMontage = nullptr;
	float GuardRestartBlendStartPosition = 0.f;
	float GuardRestartBlendTargetPosition = 0.f;
	float GuardRestartBlendElapsedTime = 0.f;
	float GuardRestartBlendDuration = 0.f;
	float GuardRestartBlendRestorePlayRate = 1.f;
	bool bGuardRestartBlendActive = false;
};
