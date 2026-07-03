#include "Animation/Notifies/Boss/AnimNotifyState_BossSuperArmorWindow.h"

#include "Character/Monster/Boss/JunBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_BossSuperArmorWindow::GetNotifyName_Implementation() const
{
	return TEXT("BossSuperArmorWindow");
}

void UAnimNotifyState_BossSuperArmorWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->BeginAttackSuperArmorWindow();
	}
}

void UAnimNotifyState_BossSuperArmorWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AJunBoss* Boss = MeshComp ? Cast<AJunBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Boss->EndAttackSuperArmorWindow();
	}
}
