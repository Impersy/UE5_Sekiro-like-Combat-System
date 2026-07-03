#include "Character/Monster/Boss/JunBoss.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "JunGameplayTags.h"

bool AJunBoss::ProcessBossAttackExecution(const FBossAttackExecutionRequest& Request)
{
	UAnimMontage* Montage = Request.Selection.Montage.Get();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance)
	{
		return false;
	}

	const float PlayRate = FMath::Max(Request.Selection.PlayRate, KINDA_SMALL_NUMBER);
	ActiveSpecialReaction = EBossSpecialReactionType::None;
	CurrentAttackRuntime = Request.RuntimeState;
	CurrentAttackSelection = Request.Selection;
	CurrentAttackMontage = Montage;
	InitiativeState = Request.InitiativeState;

	bIsAttacking = true;
	bAttackInterruptedByHitReact = Request.bInterruptedByHitReact;
	AttackTime = Montage->GetPlayLength() / PlayRate;
	AttackFacingRemainTime = Request.Selection.FacingDuration;
	CombatMoveInput = FVector2D::ZeroVector;

	AddGameplayTag(JunGameplayTags::State_Condition_ControlLocked);
	AddGameplayTag(JunGameplayTags::State_Block_Move);
	StopAIMovement();
	SetDesiredMoveAxes(0.f, 0.f);

	if (Request.BlendInTime >= 0.f)
	{
		const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, Request.BlendInTime));
		return AnimInstance->Montage_PlayWithBlendSettings(Montage, BlendInSettings, PlayRate) > 0.f;
	}

	return PlayAnimMontage(Montage, PlayRate) > 0.f;
}

bool AJunBoss::ProcessBossSpecialReactionExecution(
	const FBossSpecialReactionExecutionRequest& Request)
{
	if (Request.Type == EBossSpecialReactionType::None)
	{
		return false;
	}

	FBossAttackExecutionRequest MontageRequest;
	MontageRequest.Selection = Request.Selection;
	MontageRequest.RuntimeState.Reset();
	MontageRequest.InitiativeState = Request.InitiativeState;
	MontageRequest.BlendInTime = Request.BlendInTime;
	MontageRequest.bInterruptedByHitReact = true;

	if (!ProcessBossAttackExecution(MontageRequest))
	{
		FinishAttack();
		return false;
	}

	ActiveSpecialReaction = Request.Type;
	return true;
}
