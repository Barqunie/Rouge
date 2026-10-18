// Augment (level-up card) data types.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "RougeAugmentTypes.generated.h"

class UGameplayAbility;

UENUM(BlueprintType)
enum class ERougeAugmentCategory : uint8
{
	/** Permanent attribute bonus (attack speed, damage, armor...). */
	Stat,
	/** Upgrades one of the class's Q/E skills. */
	Skill,
	/** Grants a passive ability that runs on its own (boomerang, frost pulse...). */
	Passive
};

UENUM(BlueprintType)
enum class ERougeSkillUpgrade : uint8
{
	/** Amount = fraction removed from the cooldown (0.15 = 15% shorter). */
	CooldownReduction,
	/** Amount = fraction added to the area radius (0.25 = 25% bigger). */
	Radius,
	/** Amount = seconds added to the stun. */
	StunDuration,
	/** Amount = extra charges (dash twice...). */
	ExtraCharges,
	/** Amount = extra projectiles. */
	ExtraProjectiles,
	/** Amount = shield points added. */
	ShieldAmount,
	/** Amount = fraction added to the skill's damage (0.2 = 20% more). */
	Damage
};

/** Adds Amount to an attribute's base value (server, once per pick). */
USTRUCT(BlueprintType)
struct FRougeAugmentStatMod
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	float Amount = 0.f;
};

USTRUCT(BlueprintType)
struct FRougeAugmentSkillMod
{
	GENERATED_BODY()

	/** Which skill slot it upgrades (Input.QAbility / Input.EAbility). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment", meta = (Categories = "Input"))
	FGameplayTag SkillInput;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	ERougeSkillUpgrade Upgrade = ERougeSkillUpgrade::CooldownReduction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	float Amount = 0.f;
};

/** One card in the augment pool. */
USTRUCT(BlueprintType)
struct FRougeAugmentDefinition
{
	GENERATED_BODY()

	/** Unique key (sent over the network when a card is offered / picked). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment")
	ERougeAugmentCategory Category = ERougeAugmentCategory::Stat;

	/** Only offered to these classes (CharacterTag matches any; e.g. Characters.Player.Tank). Empty = every class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment", meta = (Categories = "Characters.Player"))
	FGameplayTagContainer RequiredCharacterTags;

	/** How many times one player can pick it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment", meta = (ClampMin = "1"))
	int32 MaxStacks = 5;

	/** Relative chance to be offered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment|Effects")
	TArray<FRougeAugmentStatMod> StatMods;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment|Effects")
	TArray<FRougeAugmentSkillMod> SkillMods;

	/** Granted (and auto activated) on the first pick; later picks raise its ability level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Augment|Effects")
	TSubclassOf<UGameplayAbility> PassiveAbility;
};

/** What the client needs to draw a card. */
USTRUCT(BlueprintType)
struct FRougeAugmentOffer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	ERougeAugmentCategory Category = ERougeAugmentCategory::Stat;

	/** Times the player already has it. */
	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	int32 CurrentStacks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	int32 MaxStacks = 1;
};

/** Summed skill upgrades for one skill slot. */
USTRUCT(BlueprintType)
struct FRougeSkillUpgrades
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	float CooldownMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	float RadiusMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	float StunBonus = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	int32 ExtraCharges = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	int32 ExtraProjectiles = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	float ShieldBonus = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	float DamageMultiplier = 1.f;
};

USTRUCT(BlueprintType)
struct FRougeSkillUpgradeEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FGameplayTag SkillInput;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FRougeSkillUpgrades Upgrades;
};

USTRUCT(BlueprintType)
struct FRougeOwnedAugment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	int32 Stacks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	int32 MaxStacks = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Augment")
	ERougeAugmentCategory Category = ERougeAugmentCategory::Stat;
};
