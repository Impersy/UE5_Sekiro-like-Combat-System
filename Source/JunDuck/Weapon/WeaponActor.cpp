
#include "Weapon/WeaponActor.h"
#include "Character/JunCharacter.h"
#include "Character/JunMonster.h"
#include "Character/JunPlayer.h"
#include "DrawDebugHelpers.h"

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

	// 충돌 프리셋
	WeaponMesh->SetCollisionProfileName(TEXT("WeaponAttachment"));

}

// Called when the game starts or when spawned
void AWeaponActor::BeginPlay()
{
	Super::BeginPlay();
	
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
		DrawAttackTraceDebug(
			TraceStartPoint->GetComponentLocation(),
			TraceEndPoint->GetComponentLocation(),
			false
		);
	}
}

void AWeaponActor::StartAttackTrace()
{
	bTraceActive = true;
	HitActors.Empty();

	PrevTraceStart = TraceStartPoint->GetComponentLocation();
	PrevTraceEnd = TraceEndPoint->GetComponentLocation();
}

void AWeaponActor::EndAttackTrace()
{
	bTraceActive = false;
	HitActors.Empty();
}

void AWeaponActor::StopAttackTrace()
{
	if (!bTraceActive)
	{
		return;
	}

	bTraceActive = false;
	HitActors.Empty();
}

void AWeaponActor::SetTraceSampleCount(const int32 NewTraceSampleCount)
{
	TraceSampleCount = FMath::Max(2, NewTraceSampleCount);
}

void AWeaponActor::SetAttackHitReactType(const EHitReactType NewHitReactType)
{
	AttackHitReactType = NewHitReactType;
}

void AWeaponActor::SetAttackDefenseKnockbackData(const FJunAttackDefenseKnockbackData& NewDefenseKnockbackData)
{
	AttackDefenseKnockbackData = NewDefenseKnockbackData;
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
	const FVector CurrentTraceEnd = TraceEndPoint->GetComponentLocation();
	const FVector SwingDirection = (((CurrentTraceStart - PrevTraceStart) + (CurrentTraceEnd - PrevTraceEnd)) * 0.5f).GetSafeNormal();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	TSet<AActor*> CurrentFrameHitActors;

	const int32 SampleCount = FMath::Max(2, TraceSampleCount);
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
			FCollisionShape::MakeSphere(TraceRadius),
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

void AWeaponActor::DrawAttackTraceDebug(const FVector& TraceStart, const FVector& TraceEnd, const bool bSweepDebug, const FVector& PrevStart, const FVector& PrevEnd) const
{
	if (!GetWorld())
	{
		return;
	}

	const int32 SampleCount = FMath::Max(2, TraceSampleCount);

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
			DrawDebugSphere(GetWorld(), CurrSamplePoint, TraceRadius, 12, FColor::Cyan, false, 0.05f);
		}
		else
		{
			DrawDebugSphere(GetWorld(), CurrSamplePoint, TraceRadius, 12, FColor::Cyan, false, 0.f);
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
		
		// 무적 체크
		if (VictimCharacter->Is_Invincible())
		{
			return;
		}

		// 팀 체크
		if (!AttackerCharacter->IsEnemyTo(VictimCharacter))
		{
			return;
		}

		
		// 아니면 데미지 적용
		// 나중엔 기본뎀이 아닌 최종 뎀을 여기서 계산해서 넘기자.
		AJunMonster* HitMonster = Cast<AJunMonster>(VictimCharacter);

		if (HitMonster)
		{
			HitMonster->ReceiveHit(AttackHitReactType, BasicDamage, AttackerCharacter, SwingDirection);
		}
		else if (AJunPlayer* HitPlayer = Cast<AJunPlayer>(VictimCharacter))
		{
			HitPlayer->ReceiveHit(AttackHitReactType, BasicDamage, AttackerCharacter, SwingDirection, AttackDefenseKnockbackData);
		}
		else
		{
			// 일단 플레이어 등 다른 캐릭터는 기존 방식 유지
			VictimCharacter->OnDamaged(BasicDamage, AttackerCharacter);
		}
	}
}

