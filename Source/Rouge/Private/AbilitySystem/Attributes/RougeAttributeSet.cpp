// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "GameplayEffectExtension.h"
#include <Net/UnrealNetwork.h>
#include <AbilitySystem/RougeAbilityTypes.h>

void URougeAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Health
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	//Critical Strike
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);

	//Combat
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Damage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URougeAttributeSet, DamageReduction, COND_None, REPNOTIFY_Always);

	//Experience Points
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


	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Data);
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

void URougeAttributeSet::OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, DamageReduction, OldDamageReduction);
}

void URougeAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, Armor, OldArmor);
}

void URougeAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, AttackSpeed, OldAttackSpeed);
}




void URougeAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldCritChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, CritChance, OldCritChance);
}
void URougeAttributeSet::OnRep_CritDamage(const FGameplayAttributeData& OldCritDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, CritDamage, OldCritDamage);
}




void URougeAttributeSet::OnRep_XP(const FGameplayAttributeData& OldXP)
{		
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, XP, OldXP);

}

void URougeAttributeSet::OnRep_MaxXP(const FGameplayAttributeData& OldMaxXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URougeAttributeSet, MaxXP, OldMaxXP);



}

void URougeAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data)
{
	const float localDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext();
	FRougeGameplayEffectContext* RougeContext = FRougeGameplayEffectContext::GetEffectContext(ContextHandle);

	FColor debugColor = RougeContext->IsCriticalHit() ? FColor::Red : FColor::Green;


	GEngine->AddOnScreenDebugMessage(-1, 5.f, debugColor, FString::Printf(TEXT("Damage Dealt: %d"), FMath::TruncToInt(localDamage)));

	SetHealth(FMath::Clamp(GetHealth() - localDamage, 0.f, GetMaxHealth()));
}
