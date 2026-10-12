// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeGameplayAbility.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "AbilitySystem/Abilities/RougeDamageAbility.h"
#include "ProjectileAbility.generated.h"

/**
 *
 */
UCLASS()
class ROUGE_API UProjectileAbility : public URougeDamageAbility
{
	GENERATED_BODY()

public:
	UProjectileAbility();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectile")
	FGameplayTag ProjectileToSpawnTag;

	// Call on the granted ability instance, never on the CDO (shared by every player).
	void SetProjectileToSpawn(const FGameplayTag& InProjectileTag);

private:
	UPROPERTY()
	TObjectPtr<AActor> AvatarActorFromInfo;

	FProjectileParams CurrentProjectileParams;

	void LoadProjectileParams();

	UFUNCTION(BlueprintCallable)
	void SpawnProjectile();
};