// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "RougeGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class ROUGE_API URougeGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Input")
	FGameplayTag InputTag;
	
};
