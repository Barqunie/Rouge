// Passive abilities granted by augment cards. They run on the server on a timer while the owner is alive.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeDamageAbility.h"
#include "RougePassiveAbilities.generated.h"

class ARougeBoomerang;

/**
 * Base for timer driven passives. Granted with the augment's stack count as the ability level.
 * Server only; anything visible (projectiles, indicators) replicates on its own.
 */
UCLASS(Abstract)
class ROUGE_API URougePassiveAbility : public URougeDamageAbility
{
	GENERATED_BODY()

public:
	URougePassiveAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	/** Seconds between pulses at ability level 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (ClampMin = "0.1"))
	float BaseInterval = 3.f;

	/** Each level above 1 multiplies the interval by this (0.85 = 15% faster per stack). */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float IntervalScalePerLevel = 0.85f;

	/** Each level above 1 adds this fraction of damage. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive", meta = (ClampMin = "0.0"))
	float DamageBonusPerLevel = 0.3f;

	/** One pulse (owner is alive). */
	virtual void Pulse() {}

	float GetInterval() const;
	float GetDamageScale() const;
	bool IsOwnerAlive() const;

	/** Applies this ability's damage (scaled by level) to an enemy. */
	void DamageEnemy(AActor* Enemy);

	/** Alive enemies within Radius of Center, closest first. */
	TArray<class AEnemyBase*> FindEnemiesInRadius(const FVector& Center, float Radius) const;

private:
	FTimerHandle PulseTimer;

	void SchedulePulse();
	void HandlePulseTimer();
};

/** Throws a boomerang at the nearest enemy; it flies out and back, hitting everything on both legs. */
UCLASS()
class ROUGE_API URougeBoomerangPassiveAbility : public URougePassiveAbility
{
	GENERATED_BODY()

public:
	URougeBoomerangPassiveAbility();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Boomerang", meta = (ClampMin = "100.0"))
	float Range = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boomerang", meta = (ClampMin = "100.0"))
	float Speed = 1100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boomerang")
	TSubclassOf<ARougeBoomerang> BoomerangClass;

	virtual void Pulse() override;
};

/** Slows (and lightly damages) enemies around the owner. */
UCLASS()
class ROUGE_API URougeFrostPulsePassiveAbility : public URougePassiveAbility
{
	GENERATED_BODY()

public:
	URougeFrostPulsePassiveAbility();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Frost Pulse", meta = (ClampMin = "50.0"))
	float Radius = 350.f;

	/** Each level above 1 adds this fraction to the radius. */
	UPROPERTY(EditDefaultsOnly, Category = "Frost Pulse", meta = (ClampMin = "0.0"))
	float RadiusBonusPerLevel = 0.15f;

	/** Enemy speed while slowed (0.6 = 40% slower). */
	UPROPERTY(EditDefaultsOnly, Category = "Frost Pulse", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SlowSpeedMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Frost Pulse", meta = (ClampMin = "0.1"))
	float SlowDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Frost Pulse")
	FLinearColor PulseColor = FLinearColor(0.35f, 0.8f, 1.f, 0.5f);

	virtual void Pulse() override;
};
