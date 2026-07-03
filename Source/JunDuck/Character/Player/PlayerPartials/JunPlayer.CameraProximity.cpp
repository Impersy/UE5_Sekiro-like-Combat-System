#include "Character/Player/JunPlayer.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/Player/PlayerComponent/JunPlayerEquipmentComponent.h"
#pragma region Camera Proximity Visibility
// ============================================================================
// Camera Proximity Visibility
// ============================================================================

void AJunPlayer::UpdateCameraProximityVisibility()
{
	if (!bEnableCameraProximityHide || !Camera || !GetMesh())
	{
		if (bCameraProximityHidden)
		{
			SetCameraProximityHidden(false);
		}
		return;
	}

	const float HideDistance = FMath::Max(0.f, CameraProximityHideDistance);
	const float ShowDistance = FMath::Max(HideDistance, CameraProximityShowDistance);
	const float CameraDistance = FVector::Dist(Camera->GetComponentLocation(), GetMesh()->Bounds.Origin);
	const bool bShouldHide = bCameraProximityHidden
		? CameraDistance < ShowDistance
		: CameraDistance <= HideDistance;

	if (bShouldHide != bCameraProximityHidden)
	{
		SetCameraProximityHidden(bShouldHide);
	}
}

void AJunPlayer::SetCameraProximityHidden(bool bShouldHide)
{
	if (bShouldHide == bCameraProximityHidden)
	{
		return;
	}

	if (bShouldHide)
	{
		bCameraProximitySavedMeshVisible = GetMesh() ? GetMesh()->IsVisible() : true;

		if (GetMesh())
		{
			GetMesh()->SetVisibility(false, false);
		}
		if (EquipmentComponent)
		{
			EquipmentComponent->SetCameraProximityHidden(true);
		}
	}
	else
	{
		if (GetMesh())
		{
			GetMesh()->SetVisibility(bCameraProximitySavedMeshVisible, false);
		}
		if (EquipmentComponent)
		{
			EquipmentComponent->SetCameraProximityHidden(false);
		}
	}

	bCameraProximityHidden = bShouldHide;
}

#pragma endregion
