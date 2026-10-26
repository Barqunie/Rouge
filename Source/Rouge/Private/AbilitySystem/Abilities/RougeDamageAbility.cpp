// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/RougeDamageAbility.h"

#include "AbilitySystemGlobals.h"
#include "Game/RougeGameState.h"
#include "AbilitySystem/RougeAbilityTypes.h"	

void URougeDamageAbility::CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo)
{
	if(AActor* AvatarActorFromInfo = GetAvatarActorFromActorInfo())
	{
		OutInfo.AvatarActor = AvatarActorFromInfo;
		OutInfo.DamageEffect = DamageEffect;
		OutInfo.BaseDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
		if (const ARougeGameState* RougeGameState = AvatarActorFromInfo->GetWorld() ? AvatarActorFromInfo->GetWorld()->GetGameState<ARougeGameState>() : nullptr)
		{
			OutInfo.BaseDamage *= 1.f + DamagePerTeamLevel * FMath::Max(0, RougeGameState->GetTeamLevel() - 1);
		}
		OutInfo.AbilityLevel = GetAbilityLevel();
		OutInfo.SourceASC = GetAbilitySystemComponentFromActorInfo();
		if(IsValid(TargetActor))
		{
			OutInfo.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		}
	}
}
