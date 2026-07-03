#include "Animation/Notifies/Common/AnimNotify_CameraShake.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

FString UAnimNotify_CameraShake::GetNotifyName_Implementation() const
{
	return TEXT("CameraShake");
}

void UAnimNotify_CameraShake::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !CameraShakeClass)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return;
	}

	PlayerController->ClientStartCameraShake(CameraShakeClass, Scale, PlaySpace, UserPlaySpaceRot);
}
