// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeGameplayAbility.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "RougeDamageAbility.generated.h"

class UGameplayEffect;

/**
 * 
 */
UCLASS()
class ROUGE_API URougeDamageAbility : public URougeGameplayAbility
{
	GENERATED_BODY()
	



public:

	void CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Damage Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Damage Effect")
	FScalableFloat BaseDamage;
};
