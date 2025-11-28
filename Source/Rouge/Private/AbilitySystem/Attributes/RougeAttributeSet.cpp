// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "GameplayEffectExtension.h"
#include <Net/UnrealNetwork.h>

void URougeAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Damage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, XP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, MaxXP, COND_None, REPNOTIFY_Always);
	
	
}

void URougeAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if(Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}

}

void URougeAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{	
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Health, OldHealth);
}

void URougeAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, MaxHealth, OldMaxHealth);
}

void URougeAttributeSet::OnRep_Damage(const FGameplayAttributeData& OldDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Damage, OldDamage);
}

void URougeAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, AttackSpeed, OldAttackSpeed);
}

void URougeAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Armor, OldArmor);
}

void URougeAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldCritChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, CritChance, OldCritChance);
}

void URougeAttributeSet::OnRep_XP(const FGameplayAttributeData& OldXP)
{		
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, XP, OldXP);

}

void URougeAttributeSet::OnRep_MaxXP(const FGameplayAttributeData& OldMaxXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, MaxXP, OldMaxXP);
}
