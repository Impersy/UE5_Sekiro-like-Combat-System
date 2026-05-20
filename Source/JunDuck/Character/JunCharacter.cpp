


#include "Character/JunCharacter.h"
#include "JunDefine.h"
#include "JunGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

// Sets default values
AJunCharacter::AJunCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	// 메쉬 충돌 프리셋
	GetMesh()->SetCollisionProfileName(TEXT("CharacterHitMesh"));
}

// Called when the game starts or when spawned
void AJunCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AJunCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AJunCharacter::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	Hp = FMath::Clamp(Hp - Damage, 0, MaxHp);

	if (Hp == 0) 
	{
		AddGameplayTag(JunGameplayTags::State_Condition_Dead);
	}

}

void AJunCharacter::HighLight()
{
	bHighLighted = true;

}

void AJunCharacter::UnHighLight()
{
	bHighLighted = false;


}

void AJunCharacter::HandleGameplayEventNotify(FGameplayTag EventTag)
{
	// 상속 클래스에서 구현
}

void AJunCharacter::HandleDefenseSoundNotify()
{
	PlayDefenseSoundByType(PendingDefenseSoundType);

	PendingDefenseSoundType = EJunDefenseSoundType::None;
}

void AJunCharacter::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, EJunWeaponNiagaraComponent NiagaraComponent, const FJunAttackTraceOverrideData& TraceOverrideData)
{
}

void AJunCharacter::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
}

void AJunCharacter::BeginKickAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData)
{
}

void AJunCharacter::EndKickAttackTraceWindow()
{
}

void AJunCharacter::BeginWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
}

void AJunCharacter::EndWeaponNiagaraWindow(EJunWeaponNiagaraComponent ComponentType)
{
}

FVector AJunCharacter::GetLockOnTargetPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 40.f);
}

bool AJunCharacter::HasGameplayTag(FGameplayTag Tag) const
{
	return OwnedGameplayTags.HasTag(Tag);
}

void AJunCharacter::AddGameplayTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedGameplayTags.AddTag(Tag);
	}
}

void AJunCharacter::RemoveGameplayTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedGameplayTags.RemoveTag(Tag);
	}
}

bool AJunCharacter::Is_Invincible() const
{
	return HasGameplayTag(JunGameplayTags::State_Condition_Invincible);
}

bool AJunCharacter::Is_Dead() const
{
	return HasGameplayTag(JunGameplayTags::State_Condition_Dead);
}

FGameplayTag AJunCharacter::GetTeamTag() const
{
	if (HasGameplayTag(JunGameplayTags::Team_Player))
	{
		return JunGameplayTags::Team_Player;
	}

	if (HasGameplayTag(JunGameplayTags::Team_Enemy))
	{
		return JunGameplayTags::Team_Enemy;
	}

	return JunGameplayTags::Team_Neutral;
}

bool AJunCharacter::IsSameTeam(const AJunCharacter* Other) const
{
	if (!Other)
	{
		return false;
	}

	return GetTeamTag() == Other->GetTeamTag();
}

bool AJunCharacter::IsEnemyTo(const AJunCharacter* Other) const
{
	if (!Other)
	{
		return false;
	}

	const FGameplayTag MyTeam = GetTeamTag();
	const FGameplayTag OtherTeam = Other->GetTeamTag();

	if (MyTeam == JunGameplayTags::Team_Neutral || OtherTeam == JunGameplayTags::Team_Neutral)
	{
		return false;
	}

	return MyTeam != OtherTeam;
}

void AJunCharacter::SetTeamTag(FGameplayTag NewTeamTag)
{
	RemoveGameplayTag(JunGameplayTags::Team_Player);
	RemoveGameplayTag(JunGameplayTags::Team_Enemy);
	RemoveGameplayTag(JunGameplayTags::Team_Neutral);

	AddGameplayTag(NewTeamTag);
}

void AJunCharacter::SetPendingDefenseSoundType(EJunDefenseSoundType NewSoundType, bool bPlayImmediately)
{
	PendingDefenseSoundType = NewSoundType;

	if (bPlayImmediately)
	{
		PlayDefenseSoundByType(PendingDefenseSoundType);
		PendingDefenseSoundType = EJunDefenseSoundType::None;
	}
}

void AJunCharacter::PlayDefenseSoundByType(EJunDefenseSoundType SoundType) const
{
	switch (SoundType)
	{
	case EJunDefenseSoundType::PerfectParry:
		PlayRandomDefenseSound(PerfectParrySounds);
		break;
	case EJunDefenseSoundType::NormalParry:
		PlayRandomDefenseSound(NormalParrySounds);
		break;
	case EJunDefenseSoundType::GuardHit:
		PlayRandomDefenseSound(GuardHitSounds);
		break;
	case EJunDefenseSoundType::None:
	default:
		break;
	}
}

void AJunCharacter::PlayRandomDefenseSound(const TArray<TObjectPtr<USoundBase>>& Sounds) const
{
	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : Sounds)
	{
		if (Sound)
		{
			ValidSounds.Add(Sound);
		}
	}

	if (ValidSounds.Num() <= 0)
	{
		return;
	}

	USoundBase* SoundToPlay = ValidSounds[FMath::RandRange(0, ValidSounds.Num() - 1)];
	UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
}



