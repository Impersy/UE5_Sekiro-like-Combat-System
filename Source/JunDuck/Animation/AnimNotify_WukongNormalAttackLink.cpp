#include "Animation/AnimNotify_WukongNormalAttackLink.h"

#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_WukongNormalAttackLink::GetNotifyName_Implementation() const
{
	return TEXT("WukongNormalAttackLink");
}

void UAnimNotify_WukongNormalAttackLink::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AWukongBoss* Wukong = MeshComp ? Cast<AWukongBoss>(MeshComp->GetOwner()) : nullptr)
	{
		TArray<FWukongNormalAttackLinkCandidate> AttackCandidates;
		if (NextAttackType != EWukongNormalAttackType::None)
		{
			FWukongNormalAttackLinkCandidate DefaultCandidate;
			DefaultCandidate.AttackType = NextAttackType;
			DefaultCandidate.BlendOutTime = BlendOutTime;
			DefaultCandidate.BlendInTime = BlendInTime;
			DefaultCandidate.bRequireRange = bRequireRange;
			DefaultCandidate.bUseTestFilter = bUseTestFilter;
			AttackCandidates.Add(DefaultCandidate);
		}

		for (const EWukongNormalAttackType Candidate : AdditionalAttackCandidates)
		{
			if (Candidate != EWukongNormalAttackType::None)
			{
				FWukongNormalAttackLinkCandidate LegacyCandidate;
				LegacyCandidate.AttackType = Candidate;
				LegacyCandidate.BlendOutTime = BlendOutTime;
				LegacyCandidate.BlendInTime = BlendInTime;
				LegacyCandidate.bRequireRange = bRequireRange;
				LegacyCandidate.bUseTestFilter = bUseTestFilter;
				AttackCandidates.Add(LegacyCandidate);
			}
		}

		for (const FWukongNormalAttackLinkCandidate& Candidate : AdditionalAttackCandidateSettings)
		{
			if (Candidate.AttackType != EWukongNormalAttackType::None)
			{
				AttackCandidates.Add(Candidate);
			}
		}

		bool bStarted = false;
		EWukongNormalAttackType SelectedAttackType = EWukongNormalAttackType::None;
		const float ClampedTriggerChance = FMath::Clamp(TriggerChance, 0.f, 1.f);
		if (AttackCandidates.Num() > 0 && ClampedTriggerChance > 0.f && FMath::FRand() <= ClampedTriggerChance)
		{
			const FWukongNormalAttackLinkCandidate& SelectedCandidate =
				AttackCandidates[FMath::RandRange(0, AttackCandidates.Num() - 1)];
			SelectedAttackType = SelectedCandidate.AttackType;
			bStarted = Wukong->TryStartNormalAttackLinkFromNotify(
				SelectedAttackType,
				1.f,
				SelectedCandidate.BlendOutTime,
				SelectedCandidate.BlendInTime,
				SelectedCandidate.bRequireRange,
				SelectedCandidate.bUseTestFilter
			);
		}

		if (bDebugLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WukongAttackLink] Started=%s SelectedAttack=%d Chance=%.2f Candidates=%d"),
				bStarted ? TEXT("true") : TEXT("false"),
				static_cast<int32>(SelectedAttackType),
				ClampedTriggerChance,
				AttackCandidates.Num());
		}
	}
}
