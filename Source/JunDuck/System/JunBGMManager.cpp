#include "System/JunBGMManager.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "JunLogChannels.h"

AJunBGMManager::AJunBGMManager()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MapBGMComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MapBGMComponent"));
	MapBGMComponent->SetupAttachment(RootComponent);
	MapBGMComponent->bAutoActivate = false;
	MapBGMComponent->bAllowSpatialization = false;

	CombatBGMComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CombatBGMComponent"));
	CombatBGMComponent->SetupAttachment(RootComponent);
	CombatBGMComponent->bAutoActivate = false;
	CombatBGMComponent->bAllowSpatialization = false;
}

void AJunBGMManager::BeginPlay()
{
	Super::BeginPlay();

	if (bPlayMapBGMOnBeginPlay)
	{
		PlayMapBGM();
	}
}

AJunBGMManager* AJunBGMManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AJunBGMManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

void AJunBGMManager::PlayMapBGM()
{
	FadeOutBGMComponent(CombatBGMComponent, CombatBGMFadeOutTime);
	PlayBGMComponent(MapBGMComponent, MapBGM, MapBGMFadeInTime, BGMVolume);
}

void AJunBGMManager::PlayCombatBGM(USoundBase* OverrideCombatBGM)
{
	USoundBase* ResolvedCombatBGM = OverrideCombatBGM ? OverrideCombatBGM : DefaultCombatBGM.Get();
	FadeOutBGMComponent(MapBGMComponent, MapBGMFadeOutTime);
	PlayBGMComponent(CombatBGMComponent, ResolvedCombatBGM, CombatBGMFadeInTime, BGMVolume);
}

void AJunBGMManager::ReturnToMapBGM()
{
	PlayMapBGM();
}

void AJunBGMManager::StopAllBGM()
{
	FadeOutBGMComponent(MapBGMComponent, MapBGMFadeOutTime);
	FadeOutBGMComponent(CombatBGMComponent, CombatBGMFadeOutTime);
}

void AJunBGMManager::SetBGMDucked(bool bDucked)
{
	if (bBGMDucked == bDucked)
	{
		return;
	}

	bBGMDucked = bDucked;
	ApplyCurrentBGMVolume(DuckFadeTime);
}

void AJunBGMManager::PlayBGMComponent(UAudioComponent* AudioComponent, USoundBase* Sound, float FadeInTime, float Volume)
{
	if (!AudioComponent || !Sound)
	{
		UE_LOG(LogJun, Warning, TEXT("BGM play skipped. AudioComponent: %s, Sound: %s"),
			AudioComponent ? TEXT("Valid") : TEXT("None"),
			Sound ? *Sound->GetName() : TEXT("None"));
		return;
	}

	if (AudioComponent->IsPlaying() && AudioComponent->Sound == Sound)
	{
		UE_LOG(LogJun, Log, TEXT("BGM already playing: %s"), *Sound->GetName());
		return;
	}

	const float TargetVolume = bBGMDucked ? DuckedBGMVolume : Volume;
	AudioComponent->SetSound(Sound);
	AudioComponent->bAllowSpatialization = false;
	if (FadeInTime > 0.f)
	{
		AudioComponent->FadeIn(FadeInTime, TargetVolume);
	}
	else
	{
		AudioComponent->SetVolumeMultiplier(TargetVolume);
		AudioComponent->Play();
	}

	UE_LOG(LogJun, Log, TEXT("BGM started: %s, Volume: %.2f, FadeIn: %.2f"),
		*Sound->GetName(),
		TargetVolume,
		FadeInTime);
}

void AJunBGMManager::FadeOutBGMComponent(UAudioComponent* AudioComponent, float FadeOutTime)
{
	if (!AudioComponent || !AudioComponent->IsPlaying())
	{
		return;
	}

	if (FadeOutTime > 0.f)
	{
		AudioComponent->FadeOut(FadeOutTime, 0.f);
	}
	else
	{
		AudioComponent->Stop();
	}
}

void AJunBGMManager::ApplyCurrentBGMVolume(float FadeTime)
{
	const float TargetVolume = bBGMDucked ? DuckedBGMVolume : BGMVolume;

	if (MapBGMComponent && MapBGMComponent->IsPlaying())
	{
		if (FadeTime > 0.f)
		{
			MapBGMComponent->AdjustVolume(FadeTime, TargetVolume);
		}
		else
		{
			MapBGMComponent->SetVolumeMultiplier(TargetVolume);
		}
	}

	if (CombatBGMComponent && CombatBGMComponent->IsPlaying())
	{
		if (FadeTime > 0.f)
		{
			CombatBGMComponent->AdjustVolume(FadeTime, TargetVolume);
		}
		else
		{
			CombatBGMComponent->SetVolumeMultiplier(TargetVolume);
		}
	}
}
