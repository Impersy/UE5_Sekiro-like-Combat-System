#include "Animation/Notifies/Common/AnimNotifyState_MontagePlayRateWindow.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_MontagePlayRateWindow::GetNotifyName_Implementation() const
{
	return TEXT("MontagePlayRateWindow");
}

void UAnimNotifyState_MontagePlayRateWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();
	if (!CurrentMontage)
	{
		return;
	}

	FMontagePlayRateWindowRuntimeData& RuntimeData = ActiveRuntimeDataByMesh.FindOrAdd(MeshComp);
	RuntimeData.Montage = CurrentMontage;
	RuntimeData.PreviousPlayRate = AnimInstance->Montage_GetPlayRate(CurrentMontage);

	AnimInstance->Montage_SetPlayRate(CurrentMontage, PlayRate);
}

void UAnimNotifyState_MontagePlayRateWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	FMontagePlayRateWindowRuntimeData RuntimeData;
	if (!ActiveRuntimeDataByMesh.RemoveAndCopyValue(MeshComp, RuntimeData))
	{
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* Montage = RuntimeData.Montage.Get();
	if (!Montage)
	{
		return;
	}

	const float TargetPlayRate = bRestorePreviousPlayRate ? RuntimeData.PreviousPlayRate : RestorePlayRate;
	AnimInstance->Montage_SetPlayRate(Montage, TargetPlayRate);
}
