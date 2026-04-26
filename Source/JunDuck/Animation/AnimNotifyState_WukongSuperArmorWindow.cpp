#include "Animation/AnimNotifyState_WukongSuperArmorWindow.h"

#include "Character/WukongBoss.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotifyState_WukongSuperArmorWindow::GetNotifyName_Implementation() const
{
	return TEXT("WukongSuperArmorWindow");
}

void UAnimNotifyState_WukongSuperArmorWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->BeginAttackSuperArmorWindow();
	}
}

void UAnimNotifyState_WukongSuperArmorWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		Wukong->EndAttackSuperArmorWindow();
	}
}
