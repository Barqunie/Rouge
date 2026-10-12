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
	if (!IsValid(DamageEffectInfo.SourceASC) || !IsValid(DamageEffectInfo.TargetASC))
	{
		return;
	}

	if (!DamageEffectInfo.DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyDamageEffect: DamageEffect is not set (Avatar: %s)."), *GetNameSafe(DamageEffectInfo.AvatarActor));
		return;
	}

	FGameplayEffectContextHandle ContextHandle = DamageEffectInfo.SourceASC->MakeEffectContext();

	ContextHandle.AddSourceObject(DamageEffectInfo.AvatarActor);

	const FGameplayEffectSpecHandle SpecHandle = DamageEffectInfo.SourceASC->MakeOutgoingSpec(
		DamageEffectInfo.DamageEffect,
		DamageEffectInfo.AbilityLevel,
		ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		 RougeGameplayTags::Combat::Data_Damage,
		DamageEffectInfo.BaseDamage);

	DamageEffectInfo.TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
