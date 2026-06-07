


#include "Character/JunCharacter.h"
#include "JunDefine.h"
#include "JunGameplayTags.h"
#include "JunLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Sound/SoundBase.h"
#include "System/JunTimeEffectSubsystem.h"
#include "UObject/UnrealType.h"

namespace
{
	UClass* ResolveNativeComparisonClass(const UObject* Object)
	{
		UClass* Class = Object ? Object->GetClass() : nullptr;
		while (Class && Class->ClassGeneratedBy)
		{
			Class = Class->GetSuperClass();
		}

		return Class;
	}

	FString ExportPropertyValue(const FProperty* Property, const UObject* Object)
	{
		if (!Property || !Object)
		{
			return TEXT("<Invalid>");
		}

		FString Result;
		Property->ExportText_InContainer(0, Result, Object, Object, const_cast<UObject*>(Object), PPF_None);
		return Result.IsEmpty() ? TEXT("<Empty>") : Result;
	}

	FString GetPropertyCategory(const FProperty* Property)
	{
		if (!Property)
		{
			return TEXT("None");
		}

		const FString Category = Property->GetMetaData(TEXT("Category"));
		return Category.IsEmpty() ? TEXT("Uncategorized") : Category;
	}
}

// Sets default values
AJunCharacter::AJunCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	// 메쉬 충돌 프리셋
	GetMesh()->SetCollisionProfileName(TEXT("CharacterHitMesh"));

	PhysicalAnimationComponent = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimationComponent"));

	LightPhysicalHitReactionSettings.RootBoneName = TEXT("spine_02");
	LightPhysicalHitReactionSettings.BlendWeight = 0.32f;
	LightPhysicalHitReactionSettings.Duration = 0.18f;
	LightPhysicalHitReactionSettings.MinImpulseZ = -0.35f;
	LightPhysicalHitReactionSettings.MaxImpulseZ = 0.f;
	LightPhysicalHitReactionSettings.bUseAdditionalRoots = true;
	LightPhysicalHitReactionSettings.AdditionalRootBoneNames = { TEXT("thigh_l"), TEXT("thigh_r") };
	LightPhysicalHitReactionSettings.AdditionalRootBlendWeight = 0.08f;
	LightPhysicalHitReactionSettings.AdditionalRootImpulseScale = 0.08f;
	LightPhysicalHitReactionSettings.bUseDampenedBones = true;
	LightPhysicalHitReactionSettings.DampenedBoneNames = { TEXT("upperarm_l"), TEXT("upperarm_r") };
	LightPhysicalHitReactionSettings.DampenedBoneBlendWeight = 0.06f;

	SuperArmorPhysicalHitReactionSettings.RootBoneName = TEXT("spine_03");
	SuperArmorPhysicalHitReactionSettings.BlendWeight = 0.22f;
	SuperArmorPhysicalHitReactionSettings.Duration = 0.14f;
	SuperArmorPhysicalHitReactionSettings.MinImpulseZ = -0.25f;
	SuperArmorPhysicalHitReactionSettings.MaxImpulseZ = 0.f;
	SuperArmorPhysicalHitReactionSettings.bUseAdditionalRoots = false;
	SuperArmorPhysicalHitReactionSettings.bUseDampenedBones = true;
	SuperArmorPhysicalHitReactionSettings.DampenedBoneNames = { TEXT("upperarm_l"), TEXT("upperarm_r") };
	SuperArmorPhysicalHitReactionSettings.DampenedBoneBlendWeight = 0.04f;
}

// Called when the game starts or when spawned
void AJunCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (PhysicalAnimationComponent)
	{
		PhysicalAnimationComponent->SetSkeletalMeshComponent(GetMesh());
	}

	if (bDumpRuntimeTuningOnBeginPlay)
	{
		DumpRuntimeTuningSettings();
	}
	
}

// Called every frame
void AJunCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePhysicalHitReaction(DeltaTime);
}

void AJunCharacter::OnDamaged(int32 Damage, TObjectPtr<AJunCharacter> Attacker)
{
	const int32 PreviousHp = Hp;
	Hp = FMath::Clamp(Hp - Damage, 0, MaxHp);
	if (PreviousHp > Hp)
	{
		PlayHitDamageSound();
	}

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

void AJunCharacter::DumpRuntimeTuningSettings() const
{
	const UClass* RuntimeClass = GetClass();
	const UClass* NativeClass = ResolveNativeComparisonClass(this);
	const UObject* NativeDefaultObject = NativeClass ? NativeClass->GetDefaultObject() : nullptr;
	if (!RuntimeClass || !NativeClass || !NativeDefaultObject)
	{
		UE_LOG(LogJun, Warning, TEXT("RuntimeTuningDump | Failed. Actor=%s"), *GetNameSafe(this));
		return;
	}

	UE_LOG(LogJun, Warning, TEXT("========== RuntimeTuningDump Begin | Actor=%s RuntimeClass=%s NativeClass=%s OnlyOverrides=%d =========="),
		*GetNameSafe(this),
		*GetNameSafe(RuntimeClass),
		*GetNameSafe(NativeClass),
		bDumpOnlyOverriddenRuntimeTuning ? 1 : 0);

	int32 PrintedCount = 0;
	int32 ComparedCount = 0;
	int32 SkippedBlueprintOnlyCount = 0;

	for (TFieldIterator<FProperty> It(RuntimeClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		const FProperty* Property = *It;
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit))
		{
			continue;
		}

		const UClass* OwnerClass = Property->GetOwnerClass();
		const bool bCanCompareToNativeDefault =
			OwnerClass &&
			NativeClass->IsChildOf(OwnerClass);

		const FString RuntimeValue = ExportPropertyValue(Property, this);
		if (!bCanCompareToNativeDefault)
		{
			++SkippedBlueprintOnlyCount;
			if (!bDumpOnlyOverriddenRuntimeTuning)
			{
				UE_LOG(LogJun, Warning, TEXT("RuntimeTuningDump | BPOnly | %s | %s = %s"),
					*GetPropertyCategory(Property),
					*Property->GetName(),
					*RuntimeValue);
				++PrintedCount;
			}
			continue;
		}

		++ComparedCount;
		const FString NativeValue = ExportPropertyValue(Property, NativeDefaultObject);
		const bool bDifferentFromNative = RuntimeValue != NativeValue;
		if (bDumpOnlyOverriddenRuntimeTuning && !bDifferentFromNative)
		{
			continue;
		}

		UE_LOG(LogJun, Warning, TEXT("RuntimeTuningDump | %s | %s | C++=%s | Runtime=%s"),
			*GetPropertyCategory(Property),
			*Property->GetName(),
			*NativeValue,
			*RuntimeValue);
		++PrintedCount;
	}

	UE_LOG(LogJun, Warning, TEXT("========== RuntimeTuningDump End | Actor=%s Printed=%d Compared=%d SkippedBPOnly=%d =========="),
		*GetNameSafe(this),
		PrintedCount,
		ComparedCount,
		SkippedBlueprintOnlyCount);
}

void AJunCharacter::BeginAttackTraceWindow(EHitReactType HitReactType, const FJunAttackDamageData& DamageData, const FJunAttackDefenseKnockbackData& DefenseKnockbackData, const FJunAttackDefenseRuleData& DefenseRuleData, EJunWeaponNiagaraComponent NiagaraComponent, const FJunAttackTraceOverrideData& TraceOverrideData)
{
	if (const EJunDangerAttackType* PreviousDangerType = ActiveAttackTraceDangerTypes.Find(NiagaraComponent))
	{
		if (*PreviousDangerType == EJunDangerAttackType::JumpCounter)
		{
			ActiveJumpCounterThreatCount = FMath::Max(0, ActiveJumpCounterThreatCount - 1);
		}
	}

	ActiveAttackTraceDangerTypes.Add(NiagaraComponent, DefenseRuleData.DangerAttackType);
	if (DefenseRuleData.DangerAttackType == EJunDangerAttackType::JumpCounter)
	{
		++ActiveJumpCounterThreatCount;
	}
}

void AJunCharacter::EndAttackTraceWindow(EJunWeaponNiagaraComponent NiagaraComponent)
{
	if (const EJunDangerAttackType* PreviousDangerType = ActiveAttackTraceDangerTypes.Find(NiagaraComponent))
	{
		if (*PreviousDangerType == EJunDangerAttackType::JumpCounter)
		{
			ActiveJumpCounterThreatCount = FMath::Max(0, ActiveJumpCounterThreatCount - 1);
		}

		ActiveAttackTraceDangerTypes.Remove(NiagaraComponent);
	}
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

void AJunCharacter::ApplyPhysicalHitReaction(const FVector& WorldDirection, const FJunPhysicalHitReactionSettings& Settings, float StrengthScale, FName BoneName)
{
	if (!Settings.bEnable ||
		!PhysicalAnimationComponent ||
		!GetMesh() ||
		!GetMesh()->GetPhysicsAsset())
	{
		return;
	}

	const FName RootBoneName = BoneName.IsNone() ? Settings.RootBoneName : BoneName;
	if (RootBoneName.IsNone() || GetMesh()->GetBoneIndex(RootBoneName) == INDEX_NONE)
	{
		if (bDebugPhysicalHitReaction)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("PhysicalHit skipped. Invalid RootBone=%s Mesh=%s"),
				*RootBoneName.ToString(),
				*GetNameSafe(GetMesh()));
		}
		return;
	}

	FBodyInstance* RootBodyInstance = GetMesh()->GetBodyInstance(RootBoneName);
	if (!RootBodyInstance)
	{
		if (bDebugPhysicalHitReaction)
		{
			UE_LOG(
				LogJun,
				Warning,
				TEXT("PhysicalHit skipped. No physics body for RootBone=%s PhysicsAsset=%s"),
				*RootBoneName.ToString(),
				GetMesh()->GetPhysicsAsset() ? TEXT("Valid") : TEXT("None"));
		}
		return;
	}

	if (bPhysicalHitReactionActive)
	{
		StopPhysicalHitReaction();
	}

	ActivePhysicalHitRootBoneName = RootBoneName;
	ActivePhysicalHitBlendWeight = Settings.BlendWeight;
	ActivePhysicalHitAdditionalRootBoneNames.Reset();
	ActivePhysicalHitAdditionalRootBlendWeight = Settings.AdditionalRootBlendWeight;
	ActivePhysicalHitDampenedBoneNames.Reset();
	ActivePhysicalHitDampenedBoneBlendWeight = Settings.DampenedBoneBlendWeight;
	PhysicalHitReactionDuration = FMath::Max(Settings.Duration, KINDA_SMALL_NUMBER);
	PhysicalHitReactionRemainTime = PhysicalHitReactionDuration;
	bPhysicalHitReactionActive = true;

	if (!bPhysicalHitMeshCollisionOverridden)
	{
		SavedPhysicalHitMeshCollisionEnabled = GetMesh()->GetCollisionEnabled();
		bPhysicalHitMeshCollisionOverridden = true;
	}
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	FPhysicalAnimationData PhysicalAnimationData;
	PhysicalAnimationData.bIsLocalSimulation = true;
	PhysicalAnimationData.OrientationStrength = Settings.OrientationStrength;
	PhysicalAnimationData.AngularVelocityStrength = Settings.AngularVelocityStrength;
	PhysicalAnimationData.PositionStrength = 0.f;
	PhysicalAnimationData.VelocityStrength = 0.f;
	PhysicalAnimationData.MaxLinearForce = 0.f;
	PhysicalAnimationData.MaxAngularForce = Settings.MaxAngularForce;

	PhysicalAnimationComponent->ApplyPhysicalAnimationSettingsBelow(
		RootBoneName,
		PhysicalAnimationData,
		true);

	GetMesh()->SetAllBodiesBelowSimulatePhysics(RootBoneName, true, true);
	GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(
		RootBoneName,
		Settings.BlendWeight,
		false,
		true);
	GetMesh()->WakeAllRigidBodies();

	FVector ImpulseDirection = WorldDirection;
	ImpulseDirection.Z = FMath::Clamp(ImpulseDirection.Z, Settings.MinImpulseZ, Settings.MaxImpulseZ);
	ImpulseDirection = ImpulseDirection.GetSafeNormal();
	if (ImpulseDirection.IsNearlyZero())
	{
		ImpulseDirection = -GetActorForwardVector();
	}

	GetMesh()->AddImpulseToAllBodiesBelow(
		ImpulseDirection * Settings.ImpulseStrength * FMath::Max(0.f, StrengthScale),
		RootBoneName,
		true,
		true);

	if (Settings.bUseAdditionalRoots)
	{
		for (const FName& AdditionalRootBoneName : Settings.AdditionalRootBoneNames)
		{
			if (AdditionalRootBoneName.IsNone() ||
				AdditionalRootBoneName == RootBoneName ||
				GetMesh()->GetBoneIndex(AdditionalRootBoneName) == INDEX_NONE ||
				!GetMesh()->GetBodyInstance(AdditionalRootBoneName))
			{
				continue;
			}

			PhysicalAnimationComponent->ApplyPhysicalAnimationSettingsBelow(
				AdditionalRootBoneName,
				PhysicalAnimationData,
				true);

			GetMesh()->SetAllBodiesBelowSimulatePhysics(AdditionalRootBoneName, true, true);
			GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(
				AdditionalRootBoneName,
				Settings.AdditionalRootBlendWeight,
				false,
				true);

			if (Settings.AdditionalRootImpulseScale > 0.f)
			{
				GetMesh()->AddImpulseToAllBodiesBelow(
					ImpulseDirection * Settings.ImpulseStrength * Settings.AdditionalRootImpulseScale * FMath::Max(0.f, StrengthScale),
					AdditionalRootBoneName,
					true,
					true);
			}

			ActivePhysicalHitAdditionalRootBoneNames.Add(AdditionalRootBoneName);
		}
	}

	if (Settings.bUseDampenedBones)
	{
		for (const FName& DampenedBoneName : Settings.DampenedBoneNames)
		{
			if (DampenedBoneName.IsNone() ||
				GetMesh()->GetBoneIndex(DampenedBoneName) == INDEX_NONE ||
				!GetMesh()->GetBodyInstance(DampenedBoneName))
			{
				continue;
			}

			GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(
				DampenedBoneName,
				Settings.DampenedBoneBlendWeight,
				false,
				true);

			ActivePhysicalHitDampenedBoneNames.Add(DampenedBoneName);
		}
	}

	if (bDebugPhysicalHitReaction)
	{
		const FBodyInstance* UpdatedRootBodyInstance = GetMesh()->GetBodyInstance(RootBoneName);
		UE_LOG(
			LogJun,
			Warning,
			TEXT("PhysicalHit applied. Actor=%s RootBone=%s BodyValid=%d RootSim=%d AnySim=%d Blend=%.2f Duration=%.2f Impulse=%s"),
			*GetNameSafe(this),
			*RootBoneName.ToString(),
			UpdatedRootBodyInstance ? 1 : 0,
			UpdatedRootBodyInstance && UpdatedRootBodyInstance->IsInstanceSimulatingPhysics() ? 1 : 0,
			GetMesh()->IsAnySimulatingPhysics() ? 1 : 0,
			Settings.BlendWeight,
			Settings.Duration,
			*(ImpulseDirection * Settings.ImpulseStrength * FMath::Max(0.f, StrengthScale)).ToCompactString());
	}
}

void AJunCharacter::ApplyPhysicalHitReaction(const FVector& WorldDirection, float StrengthScale, FName BoneName)
{
	ApplyPhysicalHitReaction(WorldDirection, LightPhysicalHitReactionSettings, StrengthScale, BoneName);
}

void AJunCharacter::UpdatePhysicalHitReaction(float DeltaTime)
{
	if (!bPhysicalHitReactionActive || !GetMesh())
	{
		return;
	}

	PhysicalHitReactionRemainTime = FMath::Max(0.f, PhysicalHitReactionRemainTime - DeltaTime);

	const float Alpha = PhysicalHitReactionDuration > 0.f
		? FMath::Clamp(PhysicalHitReactionRemainTime / PhysicalHitReactionDuration, 0.f, 1.f)
		: 0.f;
	GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(
		ActivePhysicalHitRootBoneName,
		ActivePhysicalHitBlendWeight * Alpha,
		false,
		true);

	for (const FName& AdditionalRootBoneName : ActivePhysicalHitAdditionalRootBoneNames)
	{
		GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(
			AdditionalRootBoneName,
			ActivePhysicalHitAdditionalRootBlendWeight * Alpha,
			false,
			true);
	}

	for (const FName& DampenedBoneName : ActivePhysicalHitDampenedBoneNames)
	{
		GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(
			DampenedBoneName,
			ActivePhysicalHitDampenedBoneBlendWeight * Alpha,
			false,
			true);
	}

	if (PhysicalHitReactionRemainTime <= 0.f)
	{
		StopPhysicalHitReaction();
	}
}

void AJunCharacter::StopPhysicalHitReaction()
{
	if (GetMesh() && !ActivePhysicalHitRootBoneName.IsNone())
	{
		GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(ActivePhysicalHitRootBoneName, 0.f, false, true);
		GetMesh()->SetAllBodiesBelowSimulatePhysics(ActivePhysicalHitRootBoneName, false, true);
	}

	if (GetMesh())
	{
		for (const FName& AdditionalRootBoneName : ActivePhysicalHitAdditionalRootBoneNames)
		{
			GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(AdditionalRootBoneName, 0.f, false, true);
			GetMesh()->SetAllBodiesBelowSimulatePhysics(AdditionalRootBoneName, false, true);
		}
	}

	if (GetMesh())
	{
		for (const FName& DampenedBoneName : ActivePhysicalHitDampenedBoneNames)
		{
			GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(DampenedBoneName, 0.f, false, true);
		}
	}

	if (GetMesh() && bPhysicalHitMeshCollisionOverridden)
	{
		GetMesh()->SetCollisionEnabled(SavedPhysicalHitMeshCollisionEnabled);
	}

	bPhysicalHitReactionActive = false;
	PhysicalHitReactionRemainTime = 0.f;
	PhysicalHitReactionDuration = 0.f;
	ActivePhysicalHitRootBoneName = NAME_None;
	ActivePhysicalHitBlendWeight = 0.f;
	ActivePhysicalHitAdditionalRootBoneNames.Reset();
	ActivePhysicalHitAdditionalRootBlendWeight = 0.f;
	ActivePhysicalHitDampenedBoneNames.Reset();
	ActivePhysicalHitDampenedBoneBlendWeight = 0.f;
	bPhysicalHitMeshCollisionOverridden = false;
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

bool AJunCharacter::IsMikiriCounterThreatActive() const
{
	return ActiveMikiriCounterThreatCount > 0;
}

bool AJunCharacter::IsJumpCounterThreatActive() const
{
	return ActiveJumpCounterThreatCount > 0;
}

void AJunCharacter::SetTeamTag(FGameplayTag NewTeamTag)
{
	RemoveGameplayTag(JunGameplayTags::Team_Player);
	RemoveGameplayTag(JunGameplayTags::Team_Enemy);
	RemoveGameplayTag(JunGameplayTags::Team_Neutral);

	AddGameplayTag(NewTeamTag);
}

void AJunCharacter::RequestPlayerAttackHitStop(AJunCharacter* AttackerCharacter)
{
	if (!bEnablePlayerAttackHitStop ||
		!AttackerCharacter ||
		AttackerCharacter == this ||
		!AttackerCharacter->HasGameplayTag(JunGameplayTags::Team_Player) ||
		HasGameplayTag(JunGameplayTags::Team_Player) ||
		!GetWorld())
	{
		return;
	}

	if (UJunTimeEffectSubsystem* TimeEffectSubsystem = GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
	{
		TimeEffectSubsystem->RequestHitStopForActors(
			{ AttackerCharacter, this },
			PlayerAttackHitStopDuration,
			PlayerAttackHitStopTimeScale,
			PlayerAttackHitStopPriority);
	}
}

void AJunCharacter::RequestDefenseHitStop(AJunCharacter* OtherCharacter)
{
	if (!bEnableDefenseHitStop ||
		!OtherCharacter ||
		OtherCharacter == this ||
		!GetWorld())
	{
		return;
	}

	if (UJunTimeEffectSubsystem* TimeEffectSubsystem = GetWorld()->GetSubsystem<UJunTimeEffectSubsystem>())
	{
		TimeEffectSubsystem->RequestHitStopForActors(
			{ this, OtherCharacter },
			DefenseHitStopDuration,
			DefenseHitStopTimeScale,
			DefenseHitStopPriority);
	}
}

void AJunCharacter::BeginMikiriCounterCommandWindow()
{
	++ActiveMikiriCounterThreatCount;
}

void AJunCharacter::EndMikiriCounterCommandWindow()
{
	ActiveMikiriCounterThreatCount = FMath::Max(0, ActiveMikiriCounterThreatCount - 1);
}

void AJunCharacter::SetPendingDefenseSoundType(EJunDefenseSoundType NewSoundType, bool bPlayImmediately)
{
	PendingDefenseSoundType = NewSoundType;

	if (bPlayImmediately)
	{
		PlayDefenseSound(PendingDefenseSoundType);
		PendingDefenseSoundType = EJunDefenseSoundType::None;
	}
}

void AJunCharacter::PlayDefenseSound(EJunDefenseSoundType SoundType)
{
	PlayDefenseSoundByType(SoundType);
	PendingDefenseSoundType = EJunDefenseSoundType::None;
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

void AJunCharacter::PlayHitDamageSound() const
{
	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : HitDamageSounds)
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
	UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation(), HitDamageSoundVolume);
}



