
#include "Weapon/WeaponActor.h"
#include "Character/JunCharacter.h"
#include "Combat/JunCombatHitProcessor.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"

// Sets default values
AWeaponActor::AWeaponActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Keep the mesh as a child so each weapon BP can freely tune relative rotation.
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	RootComponent = WeaponRoot;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponRoot);

	// 무기 메쉬는 충돌 X
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	TraceStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStartPoint"));
	TraceStartPoint->SetupAttachment(WeaponMesh);

	TraceEndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEndPoint"));
	TraceEndPoint->SetupAttachment(WeaponMesh);

	TrailStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Trail_Start"));
	TrailStartPoint->SetupAttachment(WeaponMesh);

	TrailEndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Trail_End"));
	TrailEndPoint->SetupAttachment(WeaponMesh);

	// 충돌 프리셋
	WeaponMesh->SetCollisionProfileName(TEXT("WeaponAttachment"));

}

// Called when the game starts or when spawned
void AWeaponActor::BeginPlay()
{
	Super::BeginPlay();

	CacheWeaponNiagaraComponents();
	InitializeWeaponNiagaraComponents();
}

// Called every frame
void AWeaponActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTraceActive)
	{
		UpdateAttackTrace();
	}
	else if (bShowAttackTraceDebugAlways && TraceStartPoint && TraceEndPoint)
	{
		const FVector TraceStart = TraceStartPoint->GetComponentLocation();
		DrawAttackTraceDebug(
			TraceStart,
			GetCurrentTraceEndLocation(TraceStart),
			false
		);
	}
}

void AWeaponActor::StartAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent)
{
	FAttackTraceConfig TraceConfig;
	TraceConfig.HitReactType = AttackHitReactType;
	TraceConfig.DamageData = AttackDamageData;
	TraceConfig.DefenseKnockbackData = AttackDefenseKnockbackData;
	TraceConfig.DefenseRuleData = AttackDefenseRuleData;
	TraceConfig.OverrideData = ActiveAttackTraceOverrideData;
	TraceConfig.OverrideData.bUseTraceOverride = bAttackTraceOverrideActive;
	TraceConfig.NiagaraComponent = NiagaraComponent;
	StartAttackTrace(TraceConfig);
}

void AWeaponActor::StartAttackTrace(const FAttackTraceConfig& TraceConfig)
{
	if (!TraceStartPoint || !TraceEndPoint)
	{
		StopAttackTrace();
		return;
	}

	if (bTraceActive)
	{
		FinishAttackTrace(ActiveAttackTraceNiagaraComponent);
	}

	ActiveAttackTraceConfig = TraceConfig;
	ApplyAttackTraceOverride(TraceConfig.OverrideData);
	ActiveAttackTraceNiagaraComponent = TraceConfig.NiagaraComponent;

	bTraceActive = true;
	HitActors.Empty();

	PrevTraceStart = TraceStartPoint->GetComponentLocation();
	PrevTraceEnd = GetCurrentTraceEndLocation(PrevTraceStart);

	if (bActivateTrailWithAttackTrace)
	{
		ActivateWeaponNiagara(ActiveAttackTraceNiagaraComponent);
	}
}

void AWeaponActor::EndAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent)
{
	const EJunWeaponNiagaraComponent ComponentToDeactivate = bTraceActive
		? ActiveAttackTraceNiagaraComponent
		: NiagaraComponent;
	FinishAttackTrace(ComponentToDeactivate);
}

void AWeaponActor::StopAttackTrace()
{
	FinishAttackTrace(ActiveAttackTraceNiagaraComponent);
}

void AWeaponActor::FinishAttackTrace(EJunWeaponNiagaraComponent NiagaraComponentToDeactivate)
{
	bTraceActive = false;
	HitActors.Empty();
	ClearAttackTraceOverride();
	ActiveAttackTraceConfig = FAttackTraceConfig();

	if (bActivateTrailWithAttackTrace)
	{
		DeactivateWeaponNiagara(NiagaraComponentToDeactivate);
	}
}

void AWeaponActor::SetTraceSampleCount(const int32 NewTraceSampleCount)
{
	TraceSampleCount = FMath::Max(2, NewTraceSampleCount);
}

void AWeaponActor::ApplyAttackTraceOverride(const FJunAttackTraceOverrideData& TraceOverrideData)
{
	bAttackTraceOverrideActive = TraceOverrideData.bUseTraceOverride;
	ActiveAttackTraceOverrideData = TraceOverrideData;
}

void AWeaponActor::ClearAttackTraceOverride()
{
	bAttackTraceOverrideActive = false;
	ActiveAttackTraceOverrideData = FJunAttackTraceOverrideData();
}

void AWeaponActor::SetAttackHitReactType(const EHitReactType NewHitReactType)
{
	AttackHitReactType = NewHitReactType;
}

void AWeaponActor::SetAttackDamageData(const FJunAttackDamageData& NewDamageData)
{
	AttackDamageData = NewDamageData;
}

void AWeaponActor::SetAttackDefenseKnockbackData(const FJunAttackDefenseKnockbackData& NewDefenseKnockbackData)
{
	AttackDefenseKnockbackData = NewDefenseKnockbackData;
}

void AWeaponActor::SetAttackDefenseRuleData(const FJunAttackDefenseRuleData& NewDefenseRuleData)
{
	AttackDefenseRuleData = NewDefenseRuleData;
}

void AWeaponActor::ActivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType)
{
	if (ComponentType == EJunWeaponNiagaraComponent::None)
	{
		return;
	}

	if (!bWeaponEffectsEnabled)
	{
		return;
	}

	if (UNiagaraComponent* NiagaraComponent = GetWeaponNiagaraComponent(ComponentType))
	{
		NiagaraComponent->Activate(true);
	}
}

void AWeaponActor::DeactivateWeaponNiagara(EJunWeaponNiagaraComponent ComponentType)
{
	if (ComponentType == EJunWeaponNiagaraComponent::None)
	{
		return;
	}

	if (UNiagaraComponent* NiagaraComponent = GetWeaponNiagaraComponent(ComponentType))
	{
		NiagaraComponent->Deactivate();
	}
}

void AWeaponActor::DeactivateAllWeaponNiagara()
{
	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (!NiagaraComponent)
		{
			continue;
		}

		NiagaraComponent->DeactivateImmediate();
	}
}

void AWeaponActor::SetWeaponEffectsEnabled(bool bEnabled)
{
	bWeaponEffectsEnabled = bEnabled;
	if (!bWeaponEffectsEnabled)
	{
		DeactivateAllWeaponNiagara();
	}
}

void AWeaponActor::UpdateAttackTrace()
{
	FAttackTraceFrame Frame;
	if (!BuildAttackTraceFrame(Frame))
	{
		StopAttackTrace();
		return;
	}

	SweepAttackTraceFrame(Frame);
	PrevTraceStart = Frame.CurrentStart;
	PrevTraceEnd = Frame.CurrentEnd;
}

bool AWeaponActor::BuildAttackTraceFrame(FAttackTraceFrame& OutFrame) const
{
	if (!GetWorld() || !TraceStartPoint || !TraceEndPoint)
	{
		return false;
	}

	OutFrame.PreviousStart = PrevTraceStart;
	OutFrame.PreviousEnd = PrevTraceEnd;
	OutFrame.CurrentStart = TraceStartPoint->GetComponentLocation();
	OutFrame.CurrentEnd = GetCurrentTraceEndLocation(OutFrame.CurrentStart);
	OutFrame.SwingDirection =
		(((OutFrame.CurrentStart - OutFrame.PreviousStart) +
		  (OutFrame.CurrentEnd - OutFrame.PreviousEnd)) * 0.5f).GetSafeNormal();
	OutFrame.Radius = GetCurrentTraceRadius();
	OutFrame.SampleCount = GetCurrentTraceSampleCount(OutFrame.CurrentStart, OutFrame.CurrentEnd);
	return true;
}

void AWeaponActor::SweepAttackTraceFrame(const FAttackTraceFrame& Frame)
{
	if (bShowAttackTraceSweepDebug)
	{
		DrawAttackTraceDebug(
			Frame.CurrentStart,
			Frame.CurrentEnd,
			true,
			Frame.PreviousStart,
			Frame.PreviousEnd);
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	TSet<AActor*> CurrentFrameHitActors;
	for (int32 i = 0; i < Frame.SampleCount; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(Frame.SampleCount - 1);

		const FVector PrevSamplePoint = FMath::Lerp(Frame.PreviousStart, Frame.PreviousEnd, Alpha);
		const FVector CurrSamplePoint = FMath::Lerp(Frame.CurrentStart, Frame.CurrentEnd, Alpha);

		TArray<FHitResult> HitResults;

		const bool bHit = GetWorld()->SweepMultiByChannel(
			HitResults,
			PrevSamplePoint,
			CurrSamplePoint,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(Frame.Radius),
			QueryParams
		);

		if (!bHit)
		{
			continue;
		}

		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor)
			{
				continue;
			}

			// 중복 히트 방지
			if (HitActors.Contains(HitActor))
			{
				continue;
			}

			if (CurrentFrameHitActors.Contains(HitActor))
			{
				continue;
			}

			CurrentFrameHitActors.Add(HitActor);
			HitActors.Add(HitActor);

			ApplyDamageToHitCharacter(Hit, Frame.SwingDirection);
		}
	}
}

FVector AWeaponActor::GetCurrentTraceEndLocation(const FVector& CurrentTraceStart) const
{
	const FVector BaseTraceEnd = TraceEndPoint ? TraceEndPoint->GetComponentLocation() : CurrentTraceStart;
	if (!bAttackTraceOverrideActive || FMath::IsNearlyZero(ActiveAttackTraceOverrideData.TraceEndExtension))
	{
		return BaseTraceEnd;
	}

	const FVector BaseTraceVector = BaseTraceEnd - CurrentTraceStart;
	const float BaseTraceLength = BaseTraceVector.Size();
	if (BaseTraceLength <= KINDA_SMALL_NUMBER)
	{
		return BaseTraceEnd;
	}

	const FVector TraceDirection = BaseTraceVector / BaseTraceLength;
	const float ResolvedTraceLength = FMath::Max(0.f, BaseTraceLength + ActiveAttackTraceOverrideData.TraceEndExtension);
	return CurrentTraceStart + (TraceDirection * ResolvedTraceLength);
}

float AWeaponActor::GetCurrentTraceRadius() const
{
	if (bAttackTraceOverrideActive && ActiveAttackTraceOverrideData.bOverrideTraceRadius)
	{
		return FMath::Max(0.f, ActiveAttackTraceOverrideData.TraceRadius);
	}

	return FMath::Max(0.f, TraceRadius);
}

int32 AWeaponActor::GetCurrentTraceSampleCount(const FVector& CurrentTraceStart, const FVector& CurrentTraceEnd) const
{
	int32 ResolvedSampleCount = FMath::Max(2, TraceSampleCount);

	if (!bAttackTraceOverrideActive)
	{
		return ResolvedSampleCount;
	}

	if (ActiveAttackTraceOverrideData.bOverrideTraceSampleCount)
	{
		ResolvedSampleCount = FMath::Max(2, ActiveAttackTraceOverrideData.TraceSampleCount);
	}

	if (ActiveAttackTraceOverrideData.bAutoAdjustSampleCountForExtension)
	{
		const float TraceLength = FVector::Dist(CurrentTraceStart, CurrentTraceEnd);
		const float SampleSpacing = FMath::Max(1.f, ActiveAttackTraceOverrideData.TraceSampleSpacing);
		const int32 AutoSampleCount = FMath::CeilToInt(TraceLength / SampleSpacing) + 1;
		const int32 MaxAutoSampleCount = FMath::Max(2, ActiveAttackTraceOverrideData.MaxAutoTraceSampleCount);
		ResolvedSampleCount = FMath::Max(ResolvedSampleCount, FMath::Min(AutoSampleCount, MaxAutoSampleCount));
	}

	return ResolvedSampleCount;
}

UNiagaraComponent* AWeaponActor::GetWeaponNiagaraComponent(EJunWeaponNiagaraComponent ComponentType) const
{
	switch (ComponentType)
	{
	case EJunWeaponNiagaraComponent::Trail:
		if (!CachedTrailNiagaraComponent)
		{
			CachedTrailNiagaraComponent = FindNiagaraComponentByName(TrailNiagaraComponentName);
		}
		return CachedTrailNiagaraComponent;
	case EJunWeaponNiagaraComponent::SpecialTrail:
		if (!CachedSpecialTrailNiagaraComponent)
		{
			CachedSpecialTrailNiagaraComponent = FindNiagaraComponentByName(SpecialTrailNiagaraComponentName);
		}
		return CachedSpecialTrailNiagaraComponent;
	case EJunWeaponNiagaraComponent::Aura:
		if (!CachedAuraNiagaraComponent)
		{
			CachedAuraNiagaraComponent = FindNiagaraComponentByName(AuraNiagaraComponentName);
		}
		return CachedAuraNiagaraComponent;
	case EJunWeaponNiagaraComponent::Jigen:
		if (!CachedJigenNiagaraComponent)
		{
			CachedJigenNiagaraComponent = FindNiagaraComponentByName(JigenNiagaraComponentName);
		}
		return CachedJigenNiagaraComponent;
	case EJunWeaponNiagaraComponent::LightingAura:
		if (!CachedLightingAuraNiagaraComponent)
		{
			CachedLightingAuraNiagaraComponent = FindNiagaraComponentByName(LightingAuraNiagaraComponentName);
		}
		return CachedLightingAuraNiagaraComponent;
	case EJunWeaponNiagaraComponent::LightingTrail:
		if (!CachedLightingTrailNiagaraComponent)
		{
			CachedLightingTrailNiagaraComponent = FindNiagaraComponentByName(LightingTrailNiagaraComponentName);
		}
		return CachedLightingTrailNiagaraComponent;
	case EJunWeaponNiagaraComponent::LightingSlash:
		if (!CachedLightingSlashNiagaraComponent)
		{
			CachedLightingSlashNiagaraComponent = FindNiagaraComponentByName(LightingSlashNiagaraComponentName);
		}
		return CachedLightingSlashNiagaraComponent;
	case EJunWeaponNiagaraComponent::BloodTrail:
		if (!CachedBloodTrailNiagaraComponent)
		{
			CachedBloodTrailNiagaraComponent = FindNiagaraComponentByName(BloodTrailNiagaraComponentName);
		}
		return CachedBloodTrailNiagaraComponent;
	default:
		return nullptr;
	}
}

UNiagaraComponent* AWeaponActor::FindNiagaraComponentByName(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent && NiagaraComponent->GetFName() == ComponentName)
		{
			return NiagaraComponent;
		}
	}

	return nullptr;
}

void AWeaponActor::CacheWeaponNiagaraComponents()
{
	CachedTrailNiagaraComponent = FindNiagaraComponentByName(TrailNiagaraComponentName);
	CachedSpecialTrailNiagaraComponent = FindNiagaraComponentByName(SpecialTrailNiagaraComponentName);
	CachedAuraNiagaraComponent = FindNiagaraComponentByName(AuraNiagaraComponentName);
	CachedJigenNiagaraComponent = FindNiagaraComponentByName(JigenNiagaraComponentName);
	CachedLightingAuraNiagaraComponent = FindNiagaraComponentByName(LightingAuraNiagaraComponentName);
	CachedLightingTrailNiagaraComponent = FindNiagaraComponentByName(LightingTrailNiagaraComponentName);
	CachedLightingSlashNiagaraComponent = FindNiagaraComponentByName(LightingSlashNiagaraComponentName);
	CachedBloodTrailNiagaraComponent = FindNiagaraComponentByName(BloodTrailNiagaraComponentName);
}

void AWeaponActor::InitializeWeaponNiagaraComponents()
{
	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (!NiagaraComponent)
		{
			continue;
		}

		NiagaraComponent->SetAutoActivate(false);
		NiagaraComponent->DeactivateImmediate();
	}
}

void AWeaponActor::WarmupWeaponNiagaraComponents()
{
	TArray<UNiagaraComponent*> ComponentsToWarmup;
	if (CachedTrailNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedTrailNiagaraComponent);
	}
	if (CachedSpecialTrailNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedSpecialTrailNiagaraComponent);
	}
	if (CachedAuraNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedAuraNiagaraComponent);
	}
	if (CachedJigenNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedJigenNiagaraComponent);
	}
	if (CachedLightingAuraNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedLightingAuraNiagaraComponent);
	}
	if (CachedLightingTrailNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedLightingTrailNiagaraComponent);
	}
	if (CachedLightingSlashNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedLightingSlashNiagaraComponent);
	}
	if (CachedBloodTrailNiagaraComponent)
	{
		ComponentsToWarmup.Add(CachedBloodTrailNiagaraComponent);
	}

	if (ComponentsToWarmup.IsEmpty())
	{
		return;
	}

	for (UNiagaraComponent* NiagaraComponent : ComponentsToWarmup)
	{
		if (NiagaraComponent)
		{
			NiagaraComponent->Activate(true);
		}
	}

	FTimerDelegate DeactivateDelegate;
	DeactivateDelegate.BindWeakLambda(this, [this]()
	{
		DeactivateAllWeaponNiagara();
	});

	FTimerHandle DeactivateTimerHandle;
	GetWorldTimerManager().SetTimer(
		DeactivateTimerHandle,
		DeactivateDelegate,
		WeaponNiagaraWarmupDeactivateDelay,
		false
	);
}

void AWeaponActor::DrawAttackTraceDebug(const FVector& TraceStart, const FVector& TraceEnd, const bool bSweepDebug, const FVector& PrevStart, const FVector& PrevEnd) const
{
	if (!GetWorld())
	{
		return;
	}

	const int32 SampleCount = GetCurrentTraceSampleCount(TraceStart, TraceEnd);
	const float DebugTraceRadius = GetCurrentTraceRadius();

	if (!bSweepDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.f, 0, 1.0f);
	}

	for (int32 i = 0; i < SampleCount; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(SampleCount - 1);
		const FVector CurrSamplePoint = FMath::Lerp(TraceStart, TraceEnd, Alpha);

		if (bSweepDebug)
		{
			const FVector PrevSamplePoint = FMath::Lerp(PrevStart, PrevEnd, Alpha);
			DrawDebugLine(GetWorld(), PrevSamplePoint, CurrSamplePoint, FColor::Yellow, false, 0.05f, 0, 1.0f);
			DrawDebugSphere(GetWorld(), CurrSamplePoint, DebugTraceRadius, 12, FColor::Cyan, false, 0.05f);
		}
		else
		{
			DrawDebugSphere(GetWorld(), CurrSamplePoint, DebugTraceRadius, 12, FColor::Cyan, false, 0.f);
		}
	}
}

void AWeaponActor::ApplyDamageToHitCharacter(const FHitResult& HitResult, const FVector& SwingDirection)
{
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(GetOwner());
	AJunCharacter* VictimCharacter = ResolveValidHitTarget(HitResult);
	if (!AttackerCharacter || !VictimCharacter)
	{
		return;
	}

	FJunCombatHitProcessor::ProcessHit(
		*VictimCharacter,
		BuildAttackHitContext(HitResult, SwingDirection, AttackerCharacter));
}

AJunCharacter* AWeaponActor::ResolveValidHitTarget(const FHitResult& HitResult) const
{
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(GetOwner());
	AJunCharacter* VictimCharacter = Cast<AJunCharacter>(HitResult.GetActor());
	if (!AttackerCharacter || !VictimCharacter ||
		VictimCharacter == AttackerCharacter ||
		VictimCharacter->Is_Dead() ||
		!AttackerCharacter->IsEnemyTo(VictimCharacter))
	{
		return nullptr;
	}

	if (ActiveAttackTraceConfig.DefenseRuleData.bCanBeDodgedByInvincibility && VictimCharacter->Is_Invincible())
	{
		return nullptr;
	}

	return VictimCharacter;
}

FJunAttackHitContext AWeaponActor::BuildAttackHitContext(
	const FHitResult& HitResult,
	const FVector& SwingDirection,
	AJunCharacter* AttackerCharacter) const
{
	FJunAttackHitContext HitContext;
	HitContext.HitReactType = ActiveAttackTraceConfig.HitReactType;
	HitContext.DamageData = ActiveAttackTraceConfig.DamageData;
	HitContext.Attacker = AttackerCharacter;
	HitContext.SwingDirection = SwingDirection;
	HitContext.DefenseKnockbackData = ActiveAttackTraceConfig.DefenseKnockbackData;
	HitContext.DefenseRuleData = ActiveAttackTraceConfig.DefenseRuleData;
	HitContext.HitResult = HitResult;
	return HitContext;
}
