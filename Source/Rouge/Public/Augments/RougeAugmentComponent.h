// Augments a player has picked. Lives on the PlayerState (next to the ASC), so it survives pawn changes.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Augments/RougeAugmentTypes.h"
#include "RougeAugmentComponent.generated.h"

class UAbilitySystemComponent;

/**
 * Server applies picked augments: stat mods go to the attribute base values, skill mods are summed per skill slot
 * (replicated, read by the Q/E skills on both the owning client and the server) and passives are granted as abilities.
 */
UCLASS(ClassGroup = (Rouge), meta = (BlueprintSpawnableComponent))
class ROUGE_API URougeAugmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URougeAugmentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only. Applies one stack of the augment. */
	void ApplyAugment(const FRougeAugmentDefinition& Augment);

	UFUNCTION(BlueprintPure, Category = "Augments")
	int32 GetStacks(FName AugmentId) const;

	UFUNCTION(BlueprintPure, Category = "Augments")
	const TArray<FRougeOwnedAugment>& GetOwnedAugments() const { return OwnedAugments; }

	/** Summed upgrades for a skill slot (Input.QAbility / Input.EAbility). Defaults when nothing was picked. */
	UFUNCTION(BlueprintPure, Category = "Augments")
	FRougeSkillUpgrades GetSkillUpgrades(FGameplayTag SkillInput) const;

	/** Finds the component for an ability's ASC owner (the PlayerState). */
	static const URougeAugmentComponent* FindForASC(const UAbilitySystemComponent* ASC);

private:
	UPROPERTY(Replicated)
	TArray<FRougeOwnedAugment> OwnedAugments;

	UPROPERTY(Replicated)
	TArray<FRougeSkillUpgradeEntry> SkillUpgrades;

	void ApplyStatMods(UAbilitySystemComponent* ASC, const FRougeAugmentDefinition& Augment);
	void ApplySkillMods(const FRougeAugmentDefinition& Augment);
	void ApplyPassive(UAbilitySystemComponent* ASC, const FRougeAugmentDefinition& Augment, int32 NewStacks);
};
