// Fill out your copyright notice in the Description page of Project Settings.

#include "Libraries/RougeAbilitySystemLibrary.h"
#include <Game/RougeMainGameMode.h>
#include "GameplayEffectTypes.h"
#include <Kismet/GameplayStatics.h>
#include "AbilitySystemComponent.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include <AbilitySystemBlueprintLibrary.h>
#include "AbilitySystem/RougeGameplayTag.h"


UCharacterClassInfo* URougeAbilitySystemLibrary::GetCharacterClassDefaultInfo(const UObject* WorldContextObject)
{
	if(const ARougeMainGameMode* RougeMainGameMode = Cast<ARougeMainGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return RougeMainGameMode->GetCharacterClassDefaultInfo();
	}

	return nullptr;
}

UProjectileInfo* URougeAbilitySystemLibrary::GetProjectileInfo(const UObject* WorldContextObject)
{
	if (const ARougeMainGameMode* RougeMainGameMode = Cast<ARougeMainGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return RougeMainGameMode->GetProjectileInfo();
	}

	return nullptr;
}

void URougeAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectInfo& DamageEffectInfo)
{
	FGameplayEffectContextHandle ContextHandle = DamageEffectInfo.SourceASC->MakeEffectContext();

	ContextHandle.AddSourceObject(DamageEffectInfo.AvatarActor);

	const FGameplayEffectSpecHandle SpecHandle = DamageEffectInfo.SourceASC->MakeOutgoingSpec(
		DamageEffectInfo.DamageEffect,
		DamageEffectInfo.AbilityLevel,
		ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		 RougeGameplayTags::Combat::Data_Damage,
		DamageEffectInfo.BaseDamage);

	if(IsValid(DamageEffectInfo.TargetASC))
	{
		DamageEffectInfo.TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
