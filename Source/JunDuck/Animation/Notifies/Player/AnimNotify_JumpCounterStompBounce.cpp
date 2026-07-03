#include "Animation/Notifies/Player/AnimNotify_JumpCounterStompBounce.h"

#include "Character/Player/JunPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "JunLogChannels.h"

FString UAnimNotify_JumpCounterStompBounce::GetNotifyName_Implementation() const
{
	return TEXT("JumpCounterStompBounce");
}

void UAnimNotify_JumpCounterStompBounce::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AJunPlayer* Player = Cast<AJunPlayer>(MeshComp->GetOwner()))
	{
		UE_LOG(
			LogJun,
			Warning,
			TEXT("JumpCounterStompBounceNotify | Player=%s Animation=%s Up=%.1f Back=%.1f Duration=%.3f"),
			*GetNameSafe(Player),
			*GetNameSafe(Animation),
			UpVelocity,
			BackwardVelocity,
			BackwardMoveDuration);

		Player->ApplyJumpCounterStompHit(HitReactType, DamageData, DefenseKnockbackData);
		Player->ApplyJumpCounterStompBounce(UpVelocity, BackwardVelocity, BackwardMoveDuration);
	}
}
