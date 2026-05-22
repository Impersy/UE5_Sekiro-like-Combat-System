#include "Animation/AnimNotifyState_GuardRestartAnchor.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_GuardRestartAnchor::GetNotifyName_Implementation() const
{
	return TEXT("GuardRestartAnchor");
}

void UAnimNotifyState_GuardRestartAnchor::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AJunPlayer* Player = MeshComp ? Cast<AJunPlayer>(MeshComp->GetOwner()) : nullptr;
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Player || !AnimInstance)
	{
		return;
	}

	UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();
	if (!CurrentMontage)
	{
		return;
	}

	Player->NotifyGuardRestartAnchorReached(
		CurrentMontage,
		AnimInstance->Montage_GetPosition(CurrentMontage));
}
