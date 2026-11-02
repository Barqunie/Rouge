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

	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Projectile")
	FGameplayTag ProjectileToSpawnTag;

	/** When enabled, replaces the projectile info's look for this ability's shots (e.g. the Gunner's minigun spray). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Projectile")
	FRougeTracerLook TracerOverride;

	/** > 0: these shots fly at this speed instead of the projectile info's (streaks read best fast). */
	UPROPERTY(EditDefaultsOnly, Category = "Rouge|Projectile", meta = (ClampMin = "0.0"))
	float SpeedOverride = 0.f;

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