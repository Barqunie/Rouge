// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeDamageAbility.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "Augments/RougeAugmentTypes.h"
#include "RougeSkillAbilities.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UTexture2D;
class ARougeCharacter;
class ARougeTurret;

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

	/** Seconds until the skill can be used again (0 while a charge is ready). */
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownRemaining() const;

	/** Cooldown per charge after augment upgrades. */
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownDuration() const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	int32 GetCurrentCharges() const;

	/** 1 + augment extra charges (e.g. Double Dash). */
	UFUNCTION(BlueprintPure, Category = "Skill")
	int32 GetMaxCharges() const;

	/** Name shown on the HUD skill slot: DisplayName, or a default from the skill type. */
	UFUNCTION(BlueprintPure, Category = "Skill|UI")
	FText GetSkillName() const;

	UFUNCTION(BlueprintPure, Category = "Skill|UI")
	UTexture2D* GetSkillIcon() const { return Icon; }

protected:
	/** HUD name. Empty uses the skill type's default name. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|UI")
	FText DisplayName;

	/** HUD icon. Without one the slot shows the skill name. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|UI")
	TObjectPtr<UTexture2D> Icon;

	virtual FText GetDefaultSkillName() const { return FText::GetEmpty(); }

	/** Augment upgrades for this skill's input slot (owning client and server both have them). */
	FRougeSkillUpgrades GetUpgrades() const;

	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = "0.0"))
	float Cooldown = 6.f;

	/** Optional montage played when the skill is cast. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	FLinearColor IndicatorColor = FLinearColor(1.f, 0.5f, 0.1f, 0.45f);

	/** Impact GameplayCue (RawMagnitude = radius). Empty = the old placeholder ring + ImpactVFX. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|FX")
	FGameplayTag ImpactCueTag;

	/** Placeholder impact effect (default: engine RadialBurst template). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|FX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|FX", meta = (ClampMin = "0.01"))
	float ImpactVFXScale = 1.f;

	/** Marker shown under stunned enemies. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|FX")
	FLinearColor StunMarkerColor = FLinearColor(1.f, 0.9f, 0.1f, 0.6f);

	/** Skill specific logic. Call FinishSkill() when done. */
	virtual void ActivateSkill() {}

	void FinishSkill(bool bWasCancelled = false);

	bool IsServer() const;
	FVector GetAvatarForward2D() const;

	/** Server: damages (and optionally stuns) every alive enemy inside the radius. Returns the number hit. */
	int32 DamageEnemiesInRadius(const FVector& Center, float Radius, float StunDuration = 0.f);

	/** Server: damages (and optionally stuns) one enemy with this ability's DamageEffect / BaseDamage. */
	void DamageEnemy(AActor* Enemy, float StunDuration = 0.f);

	/** Server: shows a fading circle on the ground (telegraph). */
	void SpawnIndicator(const FVector& Center, float Radius, float Duration) const;

	/** Server: impact effect (expanding ring, light pillar and ImpactVFX). */
	void SpawnImpactEffect(const FVector& Center, float Radius) const;

	/** Alive players (the caster included) within Radius (2D) of Center. */
	TArray<ARougeCharacter*> GetAlliesInRadius(const FVector& Center, float Radius) const;

	/**
	 * Server: heals a player by FlatAmount + MissingFraction of their missing HP (never past max HP)
	 * and shows the green number to the caster and the healed player.
	 */
	void HealAlly(ARougeCharacter* Ally, float FlatAmount, float MissingFraction = 0.f) const;

private:
	/** Charges (-1 until first used). Mutable: refilled lazily when queried. */
	mutable int32 Charges = -1;

	/** World time the charge being refilled started recharging. */
	mutable float RechargeStartTime = 0.f;

	/** Refills charges whose cooldown has run out. */
	void UpdateCharges() const;
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
	virtual FText GetDefaultSkillName() const override;

	FVector ImpactCenter = FVector::ZeroVector;
	float ImpactRadius = 0.f;
	float ImpactStun = 0.f;

	/** Server (and owning client, harmless there): what happens when the delay runs out. */
	virtual void Impact();

private:
	FTimerHandle ImpactTimer;
};

/** Area heal (support Q): heals every player in the circle; with BaseDamage it also hurts enemies there. */
UCLASS()
class ROUGE_API URougeHealAreaSkillAbility : public URougeAreaSkillAbility
{
	GENERATED_BODY()

protected:
	/** Flat HP restored to each player in the area. Potency upgrades multiply the whole heal. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Heal", meta = (ClampMin = "0.0"))
	float HealAmount = 15.f;

	/** Plus this fraction of each player's missing HP (0.25 = a quarter of what they lost), so it matters more when low. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Heal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MissingHealthPercent = 0.25f;

	virtual void Impact() override;
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "HealWave", "Heal Wave"); }
};

/** Team armor buff (support E): every player around the caster gets bonus armor for a while. */
UCLASS()
class ROUGE_API URougeArmorAuraSkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Armor", meta = (ClampMin = "10.0"))
	float Radius = 650.f;

	/** Armor added to each player (Armor = % damage reduction). Potency upgrades add to it. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Armor", meta = (ClampMin = "0.0"))
	float ArmorAmount = 30.f;

	/** Seconds (Duration upgrades add to it). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Armor", meta = (ClampMin = "0.1"))
	float BuffDuration = 6.f;

	virtual void ActivateSkill() override;
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "Bulwark", "Bulwark"); }
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
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "Dash", "Dash"); }

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
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "Shield", "Shield"); }

private:
	FTimerHandle ShieldExpireTimer;
};

/** Zone on the ground around the caster (support E): players inside run faster, enemies inside burn (BaseDamage per tick). */
UCLASS()
class ROUGE_API URougeZoneSkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

public:
	URougeZoneSkillAbility();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone", meta = (ClampMin = "10.0"))
	float Radius = 420.f;

	/** Seconds (Duration upgrades add to it). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone", meta = (ClampMin = "0.1"))
	float ZoneDuration = 5.f;

	/** Speed buff and burn happen once per tick. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone", meta = (ClampMin = "0.1"))
	float TickInterval = 0.5f;

	/** Extra move speed for players inside (0.3 = 30% faster). Potency upgrades add to it. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone", meta = (ClampMin = "0.0"))
	float AllySpeedBonus = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone")
	FLinearColor ZoneColor = FLinearColor(1.f, 0.5f, 0.1f, 0.28f);

	/** Cosmetic effect spawned at random spots in the zone (default: engine SimpleExplosion, small). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone")
	TObjectPtr<UNiagaraSystem> ZoneVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone", meta = (ClampMin = "0.01"))
	float ZoneVFXScale = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Zone", meta = (ClampMin = "0.05"))
	float ZoneVFXInterval = 0.25f;

	virtual void ActivateSkill() override;
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "Wildfire", "Wildfire"); }
};

/**
 * Places a chem turret in front of the caster (ranged E) that shoots the nearest enemy for a while.
 * Upgrades: Damage = shot damage, Duration = longer, Potency = faster fire, ExtraProjectiles = more shots per burst,
 * ExtraCharges = one more turret at a time.
 */
UCLASS()
class ROUGE_API URougeTurretSkillAbility : public URougeSkillAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Turret")
	FProjectileParams Projectile;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Turret", meta = (ClampMin = "0.5"))
	float TurretDuration = 8.f;

	/** Seconds between shots (Potency upgrades shorten it). */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Turret", meta = (ClampMin = "0.05"))
	float FireInterval = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Turret", meta = (ClampMin = "100.0"))
	float Range = 950.f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Turret", meta = (ClampMin = "1"))
	int32 ShotsPerVolley = 1;

	/** How far in front of the caster it is placed. */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Turret", meta = (ClampMin = "0.0"))
	float PlaceDistance = 120.f;

	virtual void ActivateSkill() override;
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "Turret", "Turret"); }

private:
	/** Server: this caster's turrets, oldest first. */
	TArray<TWeakObjectPtr<ARougeTurret>> ActiveTurrets;
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
	virtual FText GetDefaultSkillName() const override { return NSLOCTEXT("RougeSkill", "Volley", "Volley"); }
};
