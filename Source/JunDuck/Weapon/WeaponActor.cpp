
#include "Weapon/WeaponActor.h"
#include "Character/JunCharacter.h"
#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
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
	if (bWarmupWeaponNiagaraOnBeginPlay)
	{
		WarmupWeaponNiagaraComponents();
	}
	else
	{
		DeactivateAllWeaponNiagara();
	}
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
	bTraceActive = true;
	HitActors.Empty();
	ActiveAttackTraceNiagaraComponent = NiagaraComponent;

	PrevTraceStart = TraceStartPoint->GetComponentLocation();
	PrevTraceEnd = GetCurrentTraceEndLocation(PrevTraceStart);

	if (bActivateTrailWithAttackTrace)
	{
		ActivateWeaponNiagara(NiagaraComponent);
	}
}

void AWeaponActor::EndAttackTrace(EJunWeaponNiagaraComponent NiagaraComponent)
{
	bTraceActive = false;
	HitActors.Empty();
	ActiveAttackTraceNiagaraComponent = NiagaraComponent;
	ClearAttackTraceOverride();

	if (bActivateTrailWithAttackTrace)
	{
		DeactivateWeaponNiagara(NiagaraComponent);
	}
}

void AWeaponActor::StopAttackTrace()
{
	if (!bTraceActive)
	{
		return;
	}

	bTraceActive = false;
	HitActors.Empty();
	ClearAttackTraceOverride();

	if (bActivateTrailWithAttackTrace)
	{
		DeactivateWeaponNiagara(ActiveAttackTraceNiagaraComponent);
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
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::Trail);
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::SpecialTrail);
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::Aura);
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::Jigen);
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::LightingAura);
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::LightingTrail);
	DeactivateWeaponNiagara(EJunWeaponNiagaraComponent::LightingSlash);
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
	if (!TraceStartPoint || !TraceEndPoint)
	{
		return;
	}

	// 현재 프레임과 이전 프레임의 trace 위치를 비교해서
	// "이번 공격이 어느 방향으로 스윙됐는지"를 같이 계산한다.

	const FVector CurrentTraceStart = TraceStartPoint->GetComponentLocation();
	const FVector CurrentTraceEnd = GetCurrentTraceEndLocation(CurrentTraceStart);
	const FVector SwingDirection = (((CurrentTraceStart - PrevTraceStart) + (CurrentTraceEnd - PrevTraceEnd)) * 0.5f).GetSafeNormal();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	TSet<AActor*> CurrentFrameHitActors;

	const int32 SampleCount = GetCurrentTraceSampleCount(CurrentTraceStart, CurrentTraceEnd);
	if (bShowAttackTraceSweepDebug)
	{
		DrawAttackTraceDebug(CurrentTraceStart, CurrentTraceEnd, true, PrevTraceStart, PrevTraceEnd);
	}

	for (int32 i = 0; i < SampleCount; ++i)
	{
		const float Alpha = (float)i / (float)(SampleCount - 1);

		const FVector PrevSamplePoint = FMath::Lerp(PrevTraceStart, PrevTraceEnd, Alpha);
		const FVector CurrSamplePoint = FMath::Lerp(CurrentTraceStart, CurrentTraceEnd, Alpha);

		TArray<FHitResult> HitResults;

		const bool bHit = GetWorld()->SweepMultiByChannel(
			HitResults,
			PrevSamplePoint,
			CurrSamplePoint,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(GetCurrentTraceRadius()),
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

			ApplyDamageToHitCharacter(HitActor, SwingDirection);
		}
	}

	PrevTraceStart = CurrentTraceStart;
	PrevTraceEnd = CurrentTraceEnd;


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

void AWeaponActor::ApplyDamageToHitCharacter(AActor* HitActor, const FVector& SwingDirection)
{
	AJunCharacter* VictimCharacter = Cast<AJunCharacter>(HitActor);   // 맞은 대상
	AJunCharacter* AttackerCharacter = Cast<AJunCharacter>(GetOwner()); // 공격한 대상

	// 무기 쪽은 팀/무적/중복 체크까지만 하고,
	// 실제 피격 반응 선택은 각 캐릭터(특히 몬스터)에게 위임한다.
	if (VictimCharacter && AttackerCharacter)
	{
		if (!VictimCharacter || !AttackerCharacter)
		{
			return;
		}

		// 자기 자신이면 무시
		if (VictimCharacter == AttackerCharacter)
		{
			return;
		}

		// 생존 체크
		if (VictimCharacter->Is_Dead())
		{
			return;
		}
		
		// Dodge invincibility only ignores attacks that explicitly allow it.
		if (AttackDefenseRuleData.bCanBeDodgedByInvincibility && VictimCharacter->Is_Invincible())
		{
			return;
		}

		// 팀 체크
		if (!AttackerCharacter->IsEnemyTo(VictimCharacter))
		{
			return;
		}

		
		const float FinalDamage = AttackDamageData.GetFinalDamage();
		AJunMonster* HitMonster = Cast<AJunMonster>(VictimCharacter);

		if (HitMonster)
		{
			HitMonster->ReceiveHit(
				AttackHitReactType,
				FinalDamage,
				AttackerCharacter,
				SwingDirection,
				AttackDefenseKnockbackData,
				AttackDefenseRuleData,
				AttackDamageData.PoiseDamage);
		}
		else if (AJunPlayer* HitPlayer = Cast<AJunPlayer>(VictimCharacter))
		{
			HitPlayer->ReceiveHit(AttackHitReactType, FinalDamage, AttackerCharacter, SwingDirection, AttackDefenseKnockbackData, AttackDefenseRuleData);
		}
		else
		{
			// 일단 플레이어 등 다른 캐릭터는 기존 방식 유지
			VictimCharacter->OnDamaged(FMath::RoundToInt(FinalDamage), AttackerCharacter);
		}
	}
}
