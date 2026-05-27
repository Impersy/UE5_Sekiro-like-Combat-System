#include "System/JunTimeEffectSubsystem.h"

#include "GameFramework/Actor.h"
#include "JunLogChannels.h"
#include "Kismet/GameplayStatics.h"

void UJunTimeEffectSubsystem::Tick(float DeltaTime)
{
	if (!bTimeEffectActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		CancelTimeEffect();
		return;
	}

	const float CurrentRealTime = World->GetRealTimeSeconds();
	const float ElapsedTime = FMath::Max(0.f, CurrentRealTime - EffectStartRealTime);
	if (ElapsedTime >= RequestedDuration)
	{
		CancelTimeEffect();
		return;
	}

	float TargetScale = RequestedTimeScale;
	if (ActiveEffectType == EJunTimeEffectType::SlowMotion)
	{
		const float RemainTime = FMath::Max(0.f, RequestedDuration - ElapsedTime);
		if (RequestedBlendInTime > KINDA_SMALL_NUMBER && ElapsedTime < RequestedBlendInTime)
		{
			const float Alpha = FMath::Clamp(ElapsedTime / RequestedBlendInTime, 0.f, 1.f);
			TargetScale = FMath::Lerp(OriginalGlobalTimeScale, RequestedTimeScale, Alpha);
		}
		else if (RequestedBlendOutTime > KINDA_SMALL_NUMBER && RemainTime < RequestedBlendOutTime)
		{
			const float Alpha = FMath::Clamp(RemainTime / RequestedBlendOutTime, 0.f, 1.f);
			TargetScale = FMath::Lerp(OriginalGlobalTimeScale, RequestedTimeScale, Alpha);
		}
	}

	if (ActiveEffectType == EJunTimeEffectType::HitStop)
	{
		ApplyActorTimeScale(TargetScale);
	}
	else
	{
		ApplyGlobalTimeScale(TargetScale);
	}
}

TStatId UJunTimeEffectSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UJunTimeEffectSubsystem, STATGROUP_Tickables);
}

bool UJunTimeEffectSubsystem::IsTickable() const
{
	return !IsTemplate() && bTimeEffectActive;
}

void UJunTimeEffectSubsystem::Deinitialize()
{
	CancelTimeEffect();
	Super::Deinitialize();
}

bool UJunTimeEffectSubsystem::RequestHitStop(
	float Duration,
	float TimeScale,
	int32 Priority,
	bool bOverrideLowerOrEqualPriority)
{
	return StartEffect(
		EJunTimeEffectType::HitStop,
		Duration,
		TimeScale,
		0.f,
		0.f,
		Priority,
		bOverrideLowerOrEqualPriority);
}

bool UJunTimeEffectSubsystem::RequestHitStopForActors(
	const TArray<AActor*>& AffectedActors,
	float Duration,
	float TimeScale,
	int32 Priority,
	bool bOverrideLowerOrEqualPriority)
{
	return StartEffect(
		EJunTimeEffectType::HitStop,
		Duration,
		TimeScale,
		0.f,
		0.f,
		Priority,
		bOverrideLowerOrEqualPriority,
		AffectedActors);
}

bool UJunTimeEffectSubsystem::RequestSlowMotion(
	float Duration,
	float TimeScale,
	float BlendInTime,
	float BlendOutTime,
	int32 Priority,
	bool bOverrideLowerOrEqualPriority)
{
	return StartEffect(
		EJunTimeEffectType::SlowMotion,
		Duration,
		TimeScale,
		BlendInTime,
		BlendOutTime,
		Priority,
		bOverrideLowerOrEqualPriority);
}

void UJunTimeEffectSubsystem::CancelTimeEffect()
{
	if (!bTimeEffectActive)
	{
		return;
	}

	RestoreOriginalTimeScale();
	RestoreActorTimeScales();

	bTimeEffectActive = false;
	ActiveEffectType = EJunTimeEffectType::None;
	ActivePriority = 0;
	RequestedDuration = 0.f;
	RequestedTimeScale = 1.f;
	RequestedBlendInTime = 0.f;
	RequestedBlendOutTime = 0.f;
	EffectStartRealTime = 0.f;
	OriginalGlobalTimeScale = 1.f;
}

bool UJunTimeEffectSubsystem::CanStartEffect(int32 Priority, bool bOverrideLowerOrEqualPriority) const
{
	if (!bTimeEffectActive)
	{
		return true;
	}

	return bOverrideLowerOrEqualPriority
		? Priority >= ActivePriority
		: Priority > ActivePriority;
}

bool UJunTimeEffectSubsystem::StartEffect(
	EJunTimeEffectType EffectType,
	float Duration,
	float TimeScale,
	float BlendInTime,
	float BlendOutTime,
	int32 Priority,
	bool bOverrideLowerOrEqualPriority,
	const TArray<AActor*>& AffectedActors)
{
	UWorld* World = GetWorld();
	if (!World || Duration <= 0.f || EffectType == EJunTimeEffectType::None)
	{
		return false;
	}

	if (EffectType == EJunTimeEffectType::HitStop && AffectedActors.IsEmpty())
	{
		return false;
	}

	if (!CanStartEffect(Priority, bOverrideLowerOrEqualPriority))
	{
		return false;
	}

	if (bTimeEffectActive)
	{
		RestoreOriginalTimeScale();
		RestoreActorTimeScales();
	}

	if (!bTimeEffectActive || OriginalGlobalTimeScale <= 0.f)
	{
		OriginalGlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(World);
	}

	bTimeEffectActive = true;
	ActiveEffectType = EffectType;
	ActivePriority = Priority;
	RequestedDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	RequestedTimeScale = FMath::Clamp(TimeScale, 0.001f, 10.f);
	RequestedBlendInTime = FMath::Clamp(BlendInTime, 0.f, RequestedDuration);
	RequestedBlendOutTime = FMath::Clamp(BlendOutTime, 0.f, RequestedDuration);
	EffectStartRealTime = World->GetRealTimeSeconds();

	if (ActiveEffectType == EJunTimeEffectType::HitStop)
	{
		OriginalActorCustomTimeScales.Reset();
		for (AActor* Actor : AffectedActors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			OriginalActorCustomTimeScales.FindOrAdd(Actor, Actor->CustomTimeDilation);
		}

		if (OriginalActorCustomTimeScales.IsEmpty())
		{
			bTimeEffectActive = false;
			ActiveEffectType = EJunTimeEffectType::None;
			return false;
		}

		ApplyActorTimeScale(RequestedTimeScale);
	}
	else if (RequestedBlendInTime <= KINDA_SMALL_NUMBER)
	{
		ApplyGlobalTimeScale(RequestedTimeScale);
	}

	UE_LOG(
		LogJun,
		Verbose,
		TEXT("TimeEffect started. Type=%d Duration=%.3f Scale=%.3f Priority=%d"),
		static_cast<int32>(ActiveEffectType),
		RequestedDuration,
		RequestedTimeScale,
		ActivePriority);

	return true;
}

void UJunTimeEffectSubsystem::ApplyGlobalTimeScale(float TimeScale)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FMath::Clamp(TimeScale, 0.001f, 10.f));
	}
}

void UJunTimeEffectSubsystem::RestoreOriginalTimeScale()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FMath::Clamp(OriginalGlobalTimeScale, 0.001f, 10.f));
	}
}

void UJunTimeEffectSubsystem::ApplyActorTimeScale(float TimeScale)
{
	const float ClampedTimeScale = FMath::Clamp(TimeScale, 0.001f, 10.f);
	for (auto It = OriginalActorCustomTimeScales.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (!IsValid(Actor))
		{
			It.RemoveCurrent();
			continue;
		}

		Actor->CustomTimeDilation = ClampedTimeScale;
	}
}

void UJunTimeEffectSubsystem::RestoreActorTimeScales()
{
	for (auto It = OriginalActorCustomTimeScales.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (IsValid(Actor))
		{
			Actor->CustomTimeDilation = It.Value();
		}
	}

	OriginalActorCustomTimeScales.Reset();
}
