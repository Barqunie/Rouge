// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/RougeDamageAbility.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/RougeAbilityTypes.h"	

void URougeDamageAbility::CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo)
{
	if(AActor* AvatarActorFromInfo = GetAvatarActorFromActorInfo())
	{
		OutInfo.AvatarActor = AvatarActorFromInfo;
		OutInfo.DamageEffect = DamageEffect;
		OutInfo.BaseDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
		OutInfo.AbilityLevel = GetAbilityLevel();
		OutInfo.SourceASC = GetAbilitySystemComponentFromActorInfo();
		if(IsValid(TargetActor))
		{
			OutInfo.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		}
	}
}
