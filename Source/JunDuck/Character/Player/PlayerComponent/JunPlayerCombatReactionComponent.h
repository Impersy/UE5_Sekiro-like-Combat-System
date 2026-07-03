#pragma once

#include "CoreMinimal.h"
#include "Character/Player/JunPlayer.h"
#include "Components/ActorComponent.h"
#include "JunPlayerCombatReactionComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JUNDUCK_API UJunPlayerCombatReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunPlayerCombatReactionComponent();

	virtual void BeginPlay() override;

	FJunAttackHitResult ReceiveHit(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseKnockbackData& DefenseKnockbackData,
		const FJunAttackDefenseRuleData& DefenseRuleData,
		bool bCanBuildAttackerPostureOnParry);

	EJunPlayerHitResolveResult ResolveIncomingHitResult(EHitReactType IncomingHitType, const AActor* DamageCauser) const;
	bool CanBeInterruptedBy(EHitReactType IncomingHitType) const;
	bool IsDamageCauserInDefenseAngle(const AActor* DamageCauser) const;
	bool TryHandleSpecialDefense(
		AActor* DamageCauser,
		const FJunAttackDefenseRuleData& DefenseRuleData,
		EJunDefenseOutcome& OutDefenseOutcome);
	bool TryHandleSpecialDefense(
		AActor* DamageCauser,
		const FJunAttackDefenseRuleData& DefenseRuleData)
	{
		EJunDefenseOutcome IgnoredOutcome = EJunDefenseOutcome::None;
		return TryHandleSpecialDefense(DamageCauser, DefenseRuleData, IgnoredOutcome);
	}
	void NotifyDefenseReactionTarget(
		AActor* DamageCauser,
		bool bPerfectParry,
		bool bAirParry,
		float PostureScale,
		const FJunAttackDefenseRuleData& DefenseRuleData,
		bool bCanBuildAttackerPostureOnParry);
	void HandlePerfectParrySuccess(
		AActor* DamageCauser,
		const FJunAttackDefenseRuleData& DefenseRuleData,
		bool bCanBuildAttackerPostureOnParry);
	void HandleNormalParrySuccess(
		AActor* DamageCauser,
		const FJunAttackDefenseRuleData& DefenseRuleData,
		bool bCanBuildAttackerPostureOnParry);
	void HandleGuardBlockSuccess();
	void HandleDamageHitReact(
		EHitReactType HitType,
		float DamageAmount,
		AActor* DamageCauser,
		const FVector& SwingDirection,
		const FJunAttackDefenseRuleData& DefenseRuleData);
	void StartParrySuccess(bool bPerfectParry);
	void StartGuardBlock();
	void StartGuardBreak();

private:
	UPROPERTY()
	TObjectPtr<AJunPlayer> OwnerPlayer = nullptr;
};
