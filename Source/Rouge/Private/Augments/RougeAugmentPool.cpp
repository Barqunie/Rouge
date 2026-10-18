#include "Augments/RougeAugmentPool.h"

#include "AbilitySystem/Abilities/RougePassiveAbilities.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"

namespace
{
	using EFormat = ERougeAugmentValueFormat;

	// Class skill cards show up more often than the generic ones (they are what makes each class different).
	constexpr float ClassSkillWeight = 2.f;
	constexpr float GenericSkillWeight = 0.6f;

	FRougeAugmentDefinition MakeAugment(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
		ERougeAugmentCategory Category, int32 MaxStacks)
	{
		FRougeAugmentDefinition Augment;
		Augment.Id = Id;
		Augment.DisplayName = FText::FromString(Name);
		Augment.Description = FText::FromString(Description);
		Augment.Category = Category;
		Augment.MaxStacks = MaxStacks;
		return Augment;
	}

	/** Description uses {0} for the rolled amount. */
	FRougeAugmentDefinition MakeStat(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
		const FGameplayAttribute& Attribute, float Min, float Max, EFormat Format, int32 MaxStacks = 5)
	{
		FRougeAugmentDefinition Augment = MakeAugment(Id, Name, Description, ERougeAugmentCategory::Stat, MaxStacks);

		FRougeAugmentStatMod Mod;
		Mod.Attribute = Attribute;
		Mod.Amount = Min;
		Mod.AmountMax = Max;
		Mod.Format = Format;
		Augment.StatMods.Add(Mod);
		return Augment;
	}

	FRougeAugmentSkillMod MakeSkillMod(const TCHAR* SkillInput, ERougeSkillUpgrade Upgrade, float Min, float Max, EFormat Format)
	{
		FRougeAugmentSkillMod Mod;
		Mod.SkillInput = FGameplayTag::RequestGameplayTag(SkillInput);
		Mod.Upgrade = Upgrade;
		Mod.Amount = Min;
		Mod.AmountMax = Max;
		Mod.Format = Format;
		return Mod;
	}

	/** ClassTag nullptr = every class. */
	FRougeAugmentDefinition MakeSkill(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description, const TCHAR* ClassTag,
		const TCHAR* SkillInput, ERougeSkillUpgrade Upgrade, float Min, float Max, EFormat Format, int32 MaxStacks = 3)
	{
		FRougeAugmentDefinition Augment = MakeAugment(Id, Name, Description, ERougeAugmentCategory::Skill, MaxStacks);
		Augment.Weight = ClassTag ? ClassSkillWeight : GenericSkillWeight;
		if (ClassTag)
		{
			Augment.RequiredCharacterTags.AddTag(FGameplayTag::RequestGameplayTag(ClassTag));
		}
		Augment.SkillMods.Add(MakeSkillMod(SkillInput, Upgrade, Min, Max, Format));
		return Augment;
	}

	FRougeAugmentDefinition MakePassive(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
		TSubclassOf<UGameplayAbility> Ability, int32 MaxStacks = 3)
	{
		FRougeAugmentDefinition Augment = MakeAugment(Id, Name, Description, ERougeAugmentCategory::Passive, MaxStacks);
		Augment.PassiveAbility = Ability;
		return Augment;
	}

	TArray<FRougeAugmentDefinition> BuildDefaultAugments()
	{
		const TCHAR* Fighter = TEXT("Characters.Player.Melee");
		const TCHAR* Ranged = TEXT("Characters.Player.Ranged");
		const TCHAR* Tank = TEXT("Characters.Player.Tank");
		const TCHAR* Support = TEXT("Characters.Player.Support");
		const TCHAR* Q = TEXT("Input.QAbility");
		const TCHAR* E = TEXT("Input.EAbility");

		using EUp = ERougeSkillUpgrade;
		TArray<FRougeAugmentDefinition> Augments;

		// ---- Stat cards (every class). Amounts are rolled per offered card. ----
		Augments.Add(MakeStat(TEXT("Stat.AttackSpeed"), TEXT("Swift Strikes"), TEXT("+{0}% attack speed."),
			URougeAttributeSet::GetAttackSpeedAttribute(), 0.1f, 0.2f, EFormat::Fraction));
		Augments.Add(MakeStat(TEXT("Stat.Damage"), TEXT("Sharpened Edge"), TEXT("+{0}% damage."),
			URougeAttributeSet::GetDamageAttribute(), 0.08f, 0.15f, EFormat::Fraction));
		Augments.Add(MakeStat(TEXT("Stat.Armor"), TEXT("Plated Armor"), TEXT("+{0} armor (less damage taken)."),
			URougeAttributeSet::GetArmorAttribute(), 8.f, 15.f, EFormat::Integer));
		Augments.Add(MakeStat(TEXT("Stat.MaxHealth"), TEXT("Vitality"), TEXT("+{0} max HP (and heals the same)."),
			URougeAttributeSet::GetMaxHealthAttribute(), 15.f, 30.f, EFormat::Integer));
		Augments.Add(MakeStat(TEXT("Stat.CritChance"), TEXT("Keen Eye"), TEXT("+{0}% critical chance."),
			URougeAttributeSet::GetCritChanceAttribute(), 5.f, 10.f, EFormat::Integer));
		Augments.Add(MakeStat(TEXT("Stat.CritDamage"), TEXT("Brutality"), TEXT("+{0}% critical damage."),
			URougeAttributeSet::GetCritDamageAttribute(), 0.2f, 0.35f, EFormat::Fraction, 4));
		Augments.Add(MakeStat(TEXT("Stat.LifeSteal"), TEXT("Vampirism"), TEXT("Heal for {0}% of the damage you deal."),
			URougeAttributeSet::GetLifeStealAttribute(), 1.f, 5.f, EFormat::Integer));

		// ---- Skill cards (every class) ----
		Augments.Add(MakeSkill(TEXT("Skill.QCooldown"), TEXT("Focus"), TEXT("Q cooldown {0}% shorter."),
			nullptr, Q, EUp::CooldownReduction, 0.1f, 0.18f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Skill.ECooldown"), TEXT("Haste"), TEXT("E cooldown {0}% shorter."),
			nullptr, E, EUp::CooldownReduction, 0.1f, 0.18f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Skill.QArea"), TEXT("Wide Reach"), TEXT("Q area {0}% bigger."),
			nullptr, Q, EUp::Radius, 0.1f, 0.2f, EFormat::Fraction));

		FRougeAugmentDefinition Empowered = MakeSkill(TEXT("Skill.Damage"), TEXT("Empowered Skills"), TEXT("Q and E deal {0}% more damage."),
			nullptr, Q, EUp::Damage, 0.1f, 0.2f, EFormat::Fraction);
		Empowered.SkillMods.Add(MakeSkillMod(E, EUp::Damage, 0.1f, 0.2f, EFormat::Fraction));
		Augments.Add(Empowered);

		// ---- Fighter: Slam (Q) / Dash (E) ----
		Augments.Add(MakeSkill(TEXT("Fighter.Shockwave"), TEXT("Shockwave"), TEXT("Slam (Q) area {0}% bigger."),
			Fighter, Q, EUp::Radius, 0.15f, 0.3f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Fighter.GroundBreaker"), TEXT("Ground Breaker"), TEXT("Slam (Q) stuns enemies for {0}s."),
			Fighter, Q, EUp::StunDuration, 0.5f, 1.f, EFormat::Decimal, 2));
		Augments.Add(MakeSkill(TEXT("Fighter.HeavySlam"), TEXT("Heavy Slam"), TEXT("Slam (Q) deals {0}% more damage."),
			Fighter, Q, EUp::Damage, 0.25f, 0.45f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Fighter.Momentum"), TEXT("Momentum"), TEXT("Slam (Q) cooldown {0}% shorter."),
			Fighter, Q, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Fighter.DoubleDash"), TEXT("Double Dash"), TEXT("Dash (E) has {0} extra charge."),
			Fighter, E, EUp::ExtraCharges, 1.f, 0.f, EFormat::Integer, 1));
		Augments.Add(MakeSkill(TEXT("Fighter.DashStrike"), TEXT("Dash Strike"), TEXT("Dash (E) deals {0}% more damage."),
			Fighter, E, EUp::Damage, 0.3f, 0.5f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Fighter.QuickStep"), TEXT("Quick Step"), TEXT("Dash (E) cooldown {0}% shorter."),
			Fighter, E, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));

		// ---- Ranged: Blast (Q) / Volley (E) ----
		Augments.Add(MakeSkill(TEXT("Ranged.BigBlast"), TEXT("Big Blast"), TEXT("Blast (Q) area {0}% bigger."),
			Ranged, Q, EUp::Radius, 0.15f, 0.3f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Ranged.HeavyBlast"), TEXT("Heavy Blast"), TEXT("Blast (Q) deals {0}% more damage."),
			Ranged, Q, EUp::Damage, 0.3f, 0.5f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Ranged.ConcussiveBlast"), TEXT("Concussive Blast"), TEXT("Blast (Q) stuns enemies for {0}s."),
			Ranged, Q, EUp::StunDuration, 0.4f, 0.8f, EFormat::Decimal, 2));
		Augments.Add(MakeSkill(TEXT("Ranged.QuickCharge"), TEXT("Quick Charge"), TEXT("Blast (Q) cooldown {0}% shorter."),
			Ranged, Q, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Ranged.WideVolley"), TEXT("Wide Volley"), TEXT("Volley (E) fires {0} more projectiles."),
			Ranged, E, EUp::ExtraProjectiles, 1.f, 3.f, EFormat::Integer));
		Augments.Add(MakeSkill(TEXT("Ranged.SharpVolley"), TEXT("Sharp Volley"), TEXT("Volley (E) deals {0}% more damage."),
			Ranged, E, EUp::Damage, 0.25f, 0.45f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Ranged.RapidVolley"), TEXT("Rapid Volley"), TEXT("Volley (E) cooldown {0}% shorter."),
			Ranged, E, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));

		// ---- Tank: Stun (Q) / Shield (E) ----
		Augments.Add(MakeSkill(TEXT("Tank.Concussion"), TEXT("Concussion"), TEXT("Stun (Q) lasts {0}s longer."),
			Tank, Q, EUp::StunDuration, 0.5f, 1.f, EFormat::Decimal));
		Augments.Add(MakeSkill(TEXT("Tank.Earthquake"), TEXT("Earthquake"), TEXT("Stun (Q) area {0}% bigger."),
			Tank, Q, EUp::Radius, 0.15f, 0.3f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Tank.Crusher"), TEXT("Crusher"), TEXT("Stun (Q) deals {0}% more damage."),
			Tank, Q, EUp::Damage, 0.4f, 0.6f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Tank.Tremor"), TEXT("Tremor"), TEXT("Stun (Q) cooldown {0}% shorter."),
			Tank, Q, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Tank.Fortress"), TEXT("Fortress"), TEXT("Shield (E) absorbs {0} more damage."),
			Tank, E, EUp::ShieldAmount, 20.f, 40.f, EFormat::Integer));
		Augments.Add(MakeSkill(TEXT("Tank.Bulwark"), TEXT("Bulwark"), TEXT("Shield (E) cooldown {0}% shorter."),
			Tank, E, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));

		// ---- Support: Heal Wave (Q) / Wildfire (E) ----
		Augments.Add(MakeSkill(TEXT("Support.Mending"), TEXT("Mending"), TEXT("Heal Wave (Q) heals {0}% more."),
			Support, Q, EUp::Potency, 0.2f, 0.35f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Support.WideWave"), TEXT("Wide Wave"), TEXT("Heal Wave (Q) area {0}% bigger."),
			Support, Q, EUp::Radius, 0.15f, 0.3f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Support.QuickWave"), TEXT("Quick Wave"), TEXT("Heal Wave (Q) cooldown {0}% shorter."),
			Support, Q, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Support.Tailwind"), TEXT("Tailwind"), TEXT("Wildfire (E) speed boost {0}% stronger."),
			Support, E, EUp::Potency, 0.25f, 0.4f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Support.Scorch"), TEXT("Scorch"), TEXT("Wildfire (E) burns for {0}% more damage."),
			Support, E, EUp::Damage, 0.3f, 0.5f, EFormat::Fraction));
		Augments.Add(MakeSkill(TEXT("Support.LingeringFlames"), TEXT("Lingering Flames"), TEXT("Wildfire (E) lasts {0}s longer."),
			Support, E, EUp::Duration, 1.f, 2.f, EFormat::Decimal));
		Augments.Add(MakeSkill(TEXT("Support.Rally"), TEXT("Rally"), TEXT("Wildfire (E) cooldown {0}% shorter."),
			Support, E, EUp::CooldownReduction, 0.15f, 0.25f, EFormat::Fraction));

		// The fighter Blueprint uses Characters.Player.Default, so fighter cards accept that tag too.
		const FGameplayTag FighterTag = FGameplayTag::RequestGameplayTag(Fighter);
		const FGameplayTag DefaultTag = FGameplayTag::RequestGameplayTag(TEXT("Characters.Player.Default"), false);
		for (FRougeAugmentDefinition& Augment : Augments)
		{
			if (DefaultTag.IsValid() && Augment.RequiredCharacterTags.HasTagExact(FighterTag))
			{
				Augment.RequiredCharacterTags.AddTag(DefaultTag);
			}
		}

		// ---- Passive cards (every class) ----
		Augments.Add(MakePassive(TEXT("Passive.Boomerang"), TEXT("Boomerang"),
			TEXT("Every 3s throws a boomerang at a nearby enemy; it comes back and hits again. Each stack adds another boomerang aimed at a different enemy."),
			URougeBoomerangPassiveAbility::StaticClass()));
		Augments.Add(MakePassive(TEXT("Passive.FrostPulse"), TEXT("Frost Pulse"),
			TEXT("Every 4s slows nearby enemies by 40% for 2s. More stacks: bigger and more often."),
			URougeFrostPulsePassiveAbility::StaticClass()));

		// ---- Rarity (unlisted cards are common). Strong / build-defining cards are rarer. ----
		using ERarity = ERougeAugmentRarity;
		const TPair<const TCHAR*, ERarity> Rarities[] =
		{
			{ TEXT("Stat.CritChance"), ERarity::Uncommon },
			{ TEXT("Stat.CritDamage"), ERarity::Uncommon },
			{ TEXT("Stat.LifeSteal"), ERarity::Rare },
			{ TEXT("Skill.Damage"), ERarity::Uncommon },
			{ TEXT("Fighter.HeavySlam"), ERarity::Uncommon },
			{ TEXT("Fighter.DashStrike"), ERarity::Uncommon },
			{ TEXT("Fighter.GroundBreaker"), ERarity::Rare },
			{ TEXT("Fighter.DoubleDash"), ERarity::Legendary },
			{ TEXT("Ranged.HeavyBlast"), ERarity::Uncommon },
			{ TEXT("Ranged.SharpVolley"), ERarity::Uncommon },
			{ TEXT("Ranged.ConcussiveBlast"), ERarity::Rare },
			{ TEXT("Ranged.WideVolley"), ERarity::Rare },
			{ TEXT("Tank.Crusher"), ERarity::Uncommon },
			{ TEXT("Tank.Fortress"), ERarity::Uncommon },
			{ TEXT("Tank.Concussion"), ERarity::Rare },
			{ TEXT("Passive.FrostPulse"), ERarity::Rare },
			{ TEXT("Support.Mending"), ERarity::Uncommon },
			{ TEXT("Support.Scorch"), ERarity::Uncommon },
			{ TEXT("Support.LingeringFlames"), ERarity::Rare },
			{ TEXT("Passive.Boomerang"), ERarity::Legendary },
		};
		for (const TPair<const TCHAR*, ERarity>& Entry : Rarities)
		{
			const FName Id(Entry.Key);
			if (FRougeAugmentDefinition* Augment = Augments.FindByPredicate([&Id](const FRougeAugmentDefinition& A) { return A.Id == Id; }))
			{
				Augment->Rarity = Entry.Value;
			}
		}

		return Augments;
	}

	FString FormatAmount(float Amount, ERougeAugmentValueFormat Format)
	{
		switch (Format)
		{
		case ERougeAugmentValueFormat::Fraction:
			return FString::FromInt(FMath::RoundToInt(Amount * 100.f));
		case ERougeAugmentValueFormat::Decimal:
			return FString::Printf(TEXT("%.1f"), Amount);
		default:
			return FString::FromInt(FMath::RoundToInt(Amount));
		}
	}
}

const TArray<FRougeAugmentDefinition>& URougeAugmentPool::GetDefaultAugments()
{
	// Built on first use (gameplay tags must be registered by then).
	static const TArray<FRougeAugmentDefinition> Defaults = BuildDefaultAugments();
	return Defaults;
}

void URougeAugmentPool::FillWithDefaults()
{
	Modify();
	Augments = GetDefaultAugments();
}

float RougeAugments::ResolveAmount(float Min, float Max, ERougeAugmentValueFormat Format, float Roll)
{
	const float Raw = Max > Min ? FMath::Lerp(Min, Max, FMath::Clamp(Roll, 0.f, 1.f)) : Min;

	// Same rounding as the card text, so what you read is what you get.
	switch (Format)
	{
	case ERougeAugmentValueFormat::Fraction:
		return FMath::RoundToFloat(Raw * 100.f) / 100.f;
	case ERougeAugmentValueFormat::Decimal:
		return FMath::RoundToFloat(Raw * 10.f) / 10.f;
	default:
		return FMath::RoundToFloat(Raw);
	}
}

FText RougeAugments::FormatDescription(const FRougeAugmentDefinition& Augment, float Roll)
{
	FString Value;
	if (Augment.StatMods.Num() > 0)
	{
		const FRougeAugmentStatMod& Mod = Augment.StatMods[0];
		Value = FormatAmount(ResolveAmount(Mod.Amount, Mod.AmountMax, Mod.Format, Roll), Mod.Format);
	}
	else if (Augment.SkillMods.Num() > 0)
	{
		const FRougeAugmentSkillMod& Mod = Augment.SkillMods[0];
		Value = FormatAmount(ResolveAmount(Mod.Amount, Mod.AmountMax, Mod.Format, Roll), Mod.Format);
	}

	return Value.IsEmpty() ? Augment.Description : FText::Format(Augment.Description, FText::FromString(Value));
}

float RougeAugments::GetRarityWeight(ERougeAugmentRarity Rarity)
{
	switch (Rarity)
	{
	case ERougeAugmentRarity::Uncommon:
		return 0.75f;
	case ERougeAugmentRarity::Rare:
		return 0.5f;
	case ERougeAugmentRarity::Legendary:
		return 0.3f;
	default:
		return 1.f;
	}
}
