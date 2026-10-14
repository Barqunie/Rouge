// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeDamageAbility.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "RougeSkillAbilities.generated.h"

class UAnimMontage;

/**
 * Base for active Q/E skills. Handles cooldown, the cast montage (replicated through the ASC) and shared helpers.
 * Gameplay effects of a skill (damage, stun, shield, projectiles) run on the server only.
 * Blueprint children only set values (InputTag, damage, cooldown, montage...).
 */
UCLASS(Abstract)
class ROUGE_API URougeSkillAbility : public URougeDamageAbility
{
	GENERATED_BODY()

public:
	URougeSkillAbility();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Seconds until the skill can be used again (for UI). */
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownRemaining() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = "0.0"))
	float Cooldown = 6.f;

	/** Optional montage played when the skill is cast. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	FLinearColor IndicatorColor = FLinearColor(1.f, 0.5f, 0.1f, 0.45f);

	/** Skill specific logic. Call FinishSkill() when done. */
	virtual void ActivateSkill() {}

	void FinishSkill(bool bWasCancelled = false);

	bool IsServer() const;
	FVector GetAvatarForward2D() const;

	/** Server: damages (and optionally stuns) every alive enemy inside the radius. Returns the number hit. */
	int32 DamageEnemiesInRadius(const FVector& Center, float Radius, float StunDuration = 0.f);

	/** Server: damages (and optionally stuns) one enemy with this ability's DamageEffect / BaseDamage. */
	void DamageEnemy(AActor* Enemy, float StunDuration = 0.f);

	/** Server: shows a fading circle on the ground. */
	void SpawnIndicator(const FVector& Center, float Radius, float Duration) const;

private:
	float LastActivationTime = -1000.f;
};

UENUM(BlueprintType)
enum class ERougeAreaCenter : uint8
{
	/** Around the caster. */
	Self,
	/** ForwardDistance in front of the caster (where they are aiming). */
	InFront
};

/** Area hit after a short delay: fighter slam, ranged blast, tank stun. */
UCLASS()
class ROUGE_API URougeAreaSkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Area")
	ERougeAreaCenter Center = ERougeAreaCenter::Self;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Area", meta = (ClampMin = "0.0", EditCondition = "Center == ERougeAreaCenter::InFront"))
	float ForwardDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Area", meta = (ClampMin = "10.0"))
	float Radius = 300.f;

	/** Time between cast and impact (the circle is shown during this time). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Area", meta = (ClampMin = "0.0"))
	float ImpactDelay = 0.25f;

	/** Stun applied to enemies hit (0 = no stun). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Area", meta = (ClampMin = "0.0"))
	float StunDuration = 0.f;

	virtual void ActivateSkill() override;

private:
	FVector ImpactCenter = FVector::ZeroVector;
	FTimerHandle ImpactTimer;

	void Impact();
};

/** Fast dash forward that damages enemies along the path (fighter). Movement uses a predicted root motion force. */
UCLASS()
class ROUGE_API URougeDashSkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

public:
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Dash", meta = (ClampMin = "50.0"))
	float DashDistance = 650.f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.22f;

	/** Width of the damage path. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Dash", meta = (ClampMin = "10.0"))
	float HitRadius = 100.f;

	virtual void ActivateSkill() override;

private:
	bool bIgnoringPawns = false;

	UFUNCTION()
	void OnDashFinished();

	void SetIgnorePawns(bool bIgnore);
};

/** Damage-absorbing shield on the caster for a while (tank). Uses the Shield attribute. */
UCLASS()
class ROUGE_API URougeShieldSkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Shield", meta = (ClampMin = "1.0"))
	float ShieldAmount = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Shield", meta = (ClampMin = "0.1"))
	float ShieldDuration = 5.f;

	virtual void ActivateSkill() override;

private:
	FTimerHandle ShieldExpireTimer;
};

/** Fan of projectiles toward the aim direction (ranged). */
UCLASS()
class ROUGE_API URougeVolleySkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Volley")
	FProjectileParams Projectile;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Volley", meta = (ClampMin = "1"))
	int32 NumProjectiles = 5;

	/** Total angle of the fan in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Volley", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float SpreadAngle = 40.f;

	virtual void ActivateSkill() override;
};
