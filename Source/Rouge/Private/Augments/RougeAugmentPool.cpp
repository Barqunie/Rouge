#include "Augments/RougeAugmentPool.h"

#include "AbilitySystem/Abilities/RougePassiveAbilities.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"

namespace
{
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

	FRougeAugmentDefinition MakeStat(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
		const FGameplayAttribute& Attribute, float Amount, int32 MaxStacks = 5)
	{
		FRougeAugmentDefinition Augment = MakeAugment(Id, Name, Description, ERougeAugmentCategory::Stat, MaxStacks);
		Augment.StatMods.Add({ Attribute, Amount });
		return Augment;
	}

	// Class skill cards show up more often than the generic ones (they are what makes each class different).
	constexpr float ClassSkillWeight = 2.f;
	constexpr float GenericSkillWeight = 0.5f;

	FRougeAugmentDefinition MakeSkill(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
		const TCHAR* ClassTag, const TCHAR* SkillInput, ERougeSkillUpgrade Upgrade, float Amount, int32 MaxStacks = 3)
	{
		FRougeAugmentDefinition Augment = MakeAugment(Id, Name, Description, ERougeAugmentCategory::Skill, MaxStacks);
		Augment.Weight = ClassTag ? ClassSkillWeight : GenericSkillWeight;
		if (ClassTag)
		{
			Augment.RequiredCharacterTags.AddTag(FGameplayTag::RequestGameplayTag(ClassTag));
		}

		FRougeAugmentSkillMod Mod;
		Mod.SkillInput = FGameplayTag::RequestGameplayTag(SkillInput);
		Mod.Upgrade = Upgrade;
		Mod.Amount = Amount;
		Augment.SkillMods.Add(Mod);
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
		const TCHAR* Q = TEXT("Input.QAbility");
		const TCHAR* E = TEXT("Input.EAbility");

		TArray<FRougeAugmentDefinition> Augments;

		// ---- Stat cards (every class) ----
		Augments.Add(MakeStat(TEXT("Stat.AttackSpeed"), TEXT("Swift Strikes"), TEXT("+15% attack speed."),
			URougeAttributeSet::GetAttackSpeedAttribute(), 0.15f));
		Augments.Add(MakeStat(TEXT("Stat.Damage"), TEXT("Sharpened Edge"), TEXT("+10% damage."),
			URougeAttributeSet::GetDamageAttribute(), 0.1f));
		Augments.Add(MakeStat(TEXT("Stat.Armor"), TEXT("Plated Armor"), TEXT("+5 armor."),
			URougeAttributeSet::GetArmorAttribute(), 5.f));
		Augments.Add(MakeStat(TEXT("Stat.MaxHealth"), TEXT("Vitality"), TEXT("+20 max HP (and heals 20)."),
			URougeAttributeSet::GetMaxHealthAttribute(), 20.f));
		Augments.Add(MakeStat(TEXT("Stat.CritChance"), TEXT("Keen Eye"), TEXT("+5% critical chance."),
			URougeAttributeSet::GetCritChanceAttribute(), 5.f));
		Augments.Add(MakeStat(TEXT("Stat.CritDamage"), TEXT("Brutality"), TEXT("+25% critical damage."),
			URougeAttributeSet::GetCritDamageAttribute(), 0.25f, 4));
		Augments.Add(MakeStat(TEXT("Stat.DamageReduction"), TEXT("Toughness"), TEXT("Take 6% less damage."),
			URougeAttributeSet::GetDamageReductionAttribute(), 6.f, 4));

		// ---- Skill cards (every class) ----
		Augments.Add(MakeSkill(TEXT("Skill.QCooldown"), TEXT("Focus"), TEXT("Q cooldown 15% shorter."),
			nullptr, Q, ERougeSkillUpgrade::CooldownReduction, 0.15f));
		Augments.Add(MakeSkill(TEXT("Skill.ECooldown"), TEXT("Haste"), TEXT("E cooldown 15% shorter."),
			nullptr, E, ERougeSkillUpgrade::CooldownReduction, 0.15f));

		FRougeAugmentDefinition Empowered = MakeSkill(TEXT("Skill.Damage"), TEXT("Empowered Skills"), TEXT("Q and E deal 20% more damage."),
			nullptr, Q, ERougeSkillUpgrade::Damage, 0.2f);
		FRougeAugmentSkillMod EDamage = Empowered.SkillMods[0];
		EDamage.SkillInput = FGameplayTag::RequestGameplayTag(E);
		Empowered.SkillMods.Add(EDamage);
		Augments.Add(Empowered);

		// ---- Fighter: Slam (Q) / Dash (E) ----
		Augments.Add(MakeSkill(TEXT("Fighter.DoubleDash"), TEXT("Double Dash"), TEXT("Dash (E) has 2 charges."),
			Fighter, E, ERougeSkillUpgrade::ExtraCharges, 1.f, 1));
		Augments.Add(MakeSkill(TEXT("Fighter.DashStrike"), TEXT("Dash Strike"), TEXT("Dash (E) deals 40% more damage."),
			Fighter, E, ERougeSkillUpgrade::Damage, 0.4f));
		Augments.Add(MakeSkill(TEXT("Fighter.QuickStep"), TEXT("Quick Step"), TEXT("Dash (E) cooldown 20% shorter."),
			Fighter, E, ERougeSkillUpgrade::CooldownReduction, 0.2f));
		Augments.Add(MakeSkill(TEXT("Fighter.Shockwave"), TEXT("Shockwave"), TEXT("Slam (Q) radius +25%."),
			Fighter, Q, ERougeSkillUpgrade::Radius, 0.25f));
		Augments.Add(MakeSkill(TEXT("Fighter.GroundBreaker"), TEXT("Ground Breaker"), TEXT("Slam (Q) stuns enemies for 0.75s."),
			Fighter, Q, ERougeSkillUpgrade::StunDuration, 0.75f, 2));

		// ---- Ranged: Blast (Q) / Volley (E) ----
		Augments.Add(MakeSkill(TEXT("Ranged.WideVolley"), TEXT("Wide Volley"), TEXT("Volley (E) fires 2 more projectiles."),
			Ranged, E, ERougeSkillUpgrade::ExtraProjectiles, 2.f));
		Augments.Add(MakeSkill(TEXT("Ranged.RapidVolley"), TEXT("Rapid Volley"), TEXT("Volley (E) cooldown 20% shorter."),
			Ranged, E, ERougeSkillUpgrade::CooldownReduction, 0.2f));
		Augments.Add(MakeSkill(TEXT("Ranged.BigBlast"), TEXT("Big Blast"), TEXT("Blast (Q) radius +25%."),
			Ranged, Q, ERougeSkillUpgrade::Radius, 0.25f));
		Augments.Add(MakeSkill(TEXT("Ranged.HeavyBlast"), TEXT("Heavy Blast"), TEXT("Blast (Q) deals 40% more damage."),
			Ranged, Q, ERougeSkillUpgrade::Damage, 0.4f));
		Augments.Add(MakeSkill(TEXT("Ranged.ConcussiveBlast"), TEXT("Concussive Blast"), TEXT("Blast (Q) stuns enemies for 0.5s."),
			Ranged, Q, ERougeSkillUpgrade::StunDuration, 0.5f, 2));

		// ---- Tank: Stun (Q) / Shield (E) ----
		Augments.Add(MakeSkill(TEXT("Tank.Concussion"), TEXT("Concussion"), TEXT("Stun (Q) lasts 1s longer."),
			Tank, Q, ERougeSkillUpgrade::StunDuration, 1.f));
		Augments.Add(MakeSkill(TEXT("Tank.Earthquake"), TEXT("Earthquake"), TEXT("Stun (Q) radius +25%."),
			Tank, Q, ERougeSkillUpgrade::Radius, 0.25f));
		Augments.Add(MakeSkill(TEXT("Tank.Crusher"), TEXT("Crusher"), TEXT("Stun (Q) deals 50% more damage."),
			Tank, Q, ERougeSkillUpgrade::Damage, 0.5f));
		Augments.Add(MakeSkill(TEXT("Tank.Fortress"), TEXT("Fortress"), TEXT("Shield (E) absorbs 30 more damage."),
			Tank, E, ERougeSkillUpgrade::ShieldAmount, 30.f));
		Augments.Add(MakeSkill(TEXT("Tank.Bulwark"), TEXT("Bulwark"), TEXT("Shield (E) cooldown 20% shorter."),
			Tank, E, ERougeSkillUpgrade::CooldownReduction, 0.2f));

		// ---- Passive cards (every class) ----
		Augments.Add(MakePassive(TEXT("Passive.Boomerang"), TEXT("Boomerang"),
			TEXT("Every 3s throws a boomerang at the nearest enemy. It comes back and hits on the way, too. More stacks: faster and stronger."),
			URougeBoomerangPassiveAbility::StaticClass()));
		Augments.Add(MakePassive(TEXT("Passive.FrostPulse"), TEXT("Frost Pulse"),
			TEXT("Every 4s slows nearby enemies by 40% for 2s. More stacks: bigger and more often."),
			URougeFrostPulsePassiveAbility::StaticClass()));

		return Augments;
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
