#include "Character/Monster/JunMonster.h"

#include "AI/JunAIController.h"
#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/Player/JunPlayer.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "PlayerController/JunPlayerController.h"
#include "System/JunBGMManager.h"
#include "System/JunTimeEffectSubsystem.h"
#include "UI/JunMonsterHUDWidget.h"
#include "Weapon/ArrowProjectile.h"
#include "Weapon/WeaponActor.h"

bool AJunMonster::ShouldUseMonsterWorldHUD() const
{
	return !ShouldShowBossCombatHUD();
}

void AJunMonster::SetMonsterWorldHUDRevealed(bool bRevealed)
{
	if (!ShouldUseMonsterWorldHUD())
	{
		bRevealed = false;
	}

	bMonsterWorldHUDRevealed = bRevealed;
	UpdateMonsterWorldHUD();
}

void AJunMonster::UpdateMonsterWorldHUD()
{
	if (!MonsterHUDWidgetComponent)
	{
		return;
	}

	float CameraDistance = 0.f;
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraDistance = FVector::Dist(CameraManager->GetCameraLocation(), GetActorLocation());
	}

	const bool bWithinVisibleDistance =
		MonsterHUDMaxVisibleDistance <= 0.f ||
		CameraDistance <= MonsterHUDMaxVisibleDistance;

	const bool bShouldShow =
		ShouldUseMonsterWorldHUD() &&
		bMonsterWorldHUDRevealed &&
		bWithinVisibleDistance &&
		CurrentState != EMonsterState::Dead &&
		!Is_Dead();

	MonsterHUDWidgetComponent->SetHiddenInGame(!bShouldShow);
	if (bShouldShow)
	{
		float ScaleAlpha = 0.f;
		const float NearDistance = FMath::Max(0.f, MonsterHUDScaleNearDistance);
		const float FarDistance = FMath::Max(NearDistance + KINDA_SMALL_NUMBER, MonsterHUDScaleFarDistance);
		ScaleAlpha = FMath::Clamp((CameraDistance - NearDistance) / (FarDistance - NearDistance), 0.f, 1.f);

		const float TargetScale = FMath::Lerp(
			FMath::Max(0.01f, MonsterHUDNearScale),
			FMath::Max(0.01f, MonsterHUDFarScale),
			ScaleAlpha);
		MonsterHUDWidgetComponent->SetDrawSize(MonsterHUDBaseDrawSize * TargetScale);
	}

	UJunMonsterHUDWidget* MonsterHUDWidget = Cast<UJunMonsterHUDWidget>(MonsterHUDWidgetComponent->GetWidget());
	if (!MonsterHUDWidget)
	{
		return;
	}

	MonsterHUDWidget->SetMonsterHUDVisible(bShouldShow);
	MonsterHUDWidget->SetMonsterHealth(Hp, MaxHp);
	MonsterHUDWidget->SetMonsterPosture(CurrentPosture, MaxPosture);
	MonsterHUDWidget->SetMonsterLifeState(CurrentLifeCount, MaxLifeCount);
}

