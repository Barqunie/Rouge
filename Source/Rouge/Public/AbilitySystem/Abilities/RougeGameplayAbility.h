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

	/** Owner's AttackSpeed attribute as a multiplier (1 = normal, 1.15 = 15% faster). Use as montage play rate. */
	UFUNCTION(BlueprintPure, Category = "Attack")
	float GetAttackSpeedMultiplier() const;

	/** BaseInterval shortened by attack speed (time between auto attacks). */
	UFUNCTION(BlueprintPure, Category = "Attack")
	float GetAttackInterval(float BaseInterval = 1.f) const;

	/** Dead characters (State.Dead) can't activate abilities. */
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Also clears timers the Blueprint set on this ability (e.g. looping auto-attack timers), so they stop when the ability is cancelled. */
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
};
