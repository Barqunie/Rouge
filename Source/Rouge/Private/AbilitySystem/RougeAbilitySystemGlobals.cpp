// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/RougeAbilitySystemGlobals.h"
#include "AbilitySystem/RougeAbilityTypes.h"

FGameplayEffectContext* URougeAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FRougeGameplayEffectContext();
}