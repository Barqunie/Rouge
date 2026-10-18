#include "Augments/RougeAugmentComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "Augments/RougeAugmentPool.h"
#include "Net/UnrealNetwork.h"

URougeAugmentComponent::URougeAugmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URougeAugmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URougeAugmentComponent, OwnedAugments);
	DOREPLIFETIME(URougeAugmentComponent, SkillUpgrades);
}

const URougeAugmentComponent* URougeAugmentComponent::FindForASC(const UAbilitySystemComponent* ASC)
{
	const AActor* Owner = ASC ? ASC->GetOwnerActor() : nullptr;
	return Owner ? Owner->FindComponentByClass<URougeAugmentComponent>() : nullptr;
}

int32 URougeAugmentComponent::GetStacks(FName AugmentId) const
{
	const FRougeOwnedAugment* Owned = OwnedAugments.FindByPredicate([AugmentId](const FRougeOwnedAugment& Entry)
	{
		return Entry.Id == AugmentId;
	});
	return Owned ? Owned->Stacks : 0;
}

FRougeSkillUpgrades URougeAugmentComponent::GetSkillUpgrades(FGameplayTag SkillInput) const
{
	const FRougeSkillUpgradeEntry* Entry = SkillUpgrades.FindByPredicate([&SkillInput](const FRougeSkillUpgradeEntry& Candidate)
	{
		return Candidate.SkillInput == SkillInput;
	});
	return Entry ? Entry->Upgrades : FRougeSkillUpgrades();
}

void URougeAugmentComponent::ApplyAugment(const FRougeAugmentDefinition& Augment, float Roll)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	FRougeOwnedAugment* Owned = OwnedAugments.FindByPredicate([&Augment](const FRougeOwnedAugment& Entry)
	{
		return Entry.Id == Augment.Id;
	});
	if (!Owned)
	{
		Owned = &OwnedAugments.AddDefaulted_GetRef();
		Owned->Id = Augment.Id;
	}
	++Owned->Stacks;
	Owned->MaxStacks = Augment.MaxStacks;
	Owned->DisplayName = Augment.DisplayName;
	Owned->Description = RougeAugments::FormatDescription(Augment, Roll);
	Owned->Category = Augment.Category;
	Owned->Rarity = Augment.Rarity;
	const int32 NewStacks = Owned->Stacks;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);

	ApplyStatMods(ASC, Augment, Roll);
	ApplySkillMods(Augment, Roll);
	ApplyPassive(ASC, Augment, NewStacks);

	Owner->ForceNetUpdate();
}

void URougeAugmentComponent::ApplyStatMods(UAbilitySystemComponent* ASC, const FRougeAugmentDefinition& Augment, float Roll)
{
	if (!ASC)
	{
		return;
	}

	for (const FRougeAugmentStatMod& Mod : Augment.StatMods)
	{
		const float Amount = RougeAugments::ResolveAmount(Mod.Amount, Mod.AmountMax, Mod.Format, Roll);
		if (!Mod.Attribute.IsValid() || FMath::IsNearlyZero(Amount))
		{
			continue;
		}

		// Permanent for the run: goes on the base value.
		ASC->ApplyModToAttribute(Mod.Attribute, EGameplayModOp::Additive, Amount);

		// More max HP also fills the new part of the bar.
		if (Mod.Attribute == URougeAttributeSet::GetMaxHealthAttribute() && Amount > 0.f)
		{
			ASC->ApplyModToAttribute(URougeAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, Amount);
		}
	}
}

void URougeAugmentComponent::ApplySkillMods(const FRougeAugmentDefinition& Augment, float Roll)
{
	for (const FRougeAugmentSkillMod& SourceMod : Augment.SkillMods)
	{
		if (!SourceMod.SkillInput.IsValid())
		{
			continue;
		}

		// Same switch as before, with the rolled amount.
		FRougeAugmentSkillMod Mod = SourceMod;
		Mod.Amount = RougeAugments::ResolveAmount(SourceMod.Amount, SourceMod.AmountMax, SourceMod.Format, Roll);

		FRougeSkillUpgradeEntry* Entry = SkillUpgrades.FindByPredicate([&Mod](const FRougeSkillUpgradeEntry& Candidate)
		{
			return Candidate.SkillInput == Mod.SkillInput;
		});
		if (!Entry)
		{
			Entry = &SkillUpgrades.AddDefaulted_GetRef();
			Entry->SkillInput = Mod.SkillInput;
		}

		FRougeSkillUpgrades& Upgrades = Entry->Upgrades;
		switch (Mod.Upgrade)
		{
		case ERougeSkillUpgrade::CooldownReduction:
			// Multiplicative so stacking never reaches zero.
			Upgrades.CooldownMultiplier *= FMath::Clamp(1.f - Mod.Amount, 0.1f, 1.f);
			break;
		case ERougeSkillUpgrade::Radius:
			Upgrades.RadiusMultiplier += Mod.Amount;
			break;
		case ERougeSkillUpgrade::StunDuration:
			Upgrades.StunBonus += Mod.Amount;
			break;
		case ERougeSkillUpgrade::ExtraCharges:
			Upgrades.ExtraCharges += FMath::RoundToInt(Mod.Amount);
			break;
		case ERougeSkillUpgrade::ExtraProjectiles:
			Upgrades.ExtraProjectiles += FMath::RoundToInt(Mod.Amount);
			break;
		case ERougeSkillUpgrade::ShieldAmount:
			Upgrades.ShieldBonus += Mod.Amount;
			break;
		case ERougeSkillUpgrade::Damage:
			Upgrades.DamageMultiplier += Mod.Amount;
			break;
		}
	}
}

void URougeAugmentComponent::ApplyPassive(UAbilitySystemComponent* ASC, const FRougeAugmentDefinition& Augment, int32 NewStacks)
{
	if (URougeAbilitySystemComponent* RougeASC = Cast<URougeAbilitySystemComponent>(ASC))
	{
		if (Augment.PassiveAbility)
		{
			RougeASC->GrantPassiveAbility(Augment.PassiveAbility, NewStacks);
		}
	}
}
