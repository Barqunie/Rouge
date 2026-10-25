// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "EnemyBase.generated.h"

class URougeAbilitySystemComponent;
class URougeAttributeSet;
class ARougeCharacter;
class UAnimMontage;
class UGameplayEffect;
class ARougeXPOrb;
class ARougePackage;

UENUM(BlueprintType)
enum class EEnemyAttackType : uint8
{
	/** Runs straight at the target and hits it up close. */
	Melee,
	/** Keeps PreferredDistance from the target and shoots projectiles. */
	Ranged
};

/**
 * Swarm enemy. AI runs on the server only: chase the nearest alive player in a straight line and attack.
 * Movement and rotation replicate through CharacterMovement; attack montages through a multicast.
 */
UCLASS()
class ROUGE_API AEnemyBase : public ACharacterBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaTime) override;

	/** Server only. Health reached 0. */
	virtual void HandleHealthDepleted() override;

	virtual bool IsDead() const override { return bDead; }

	/** Server only. Enemy stops moving and attacking for Duration seconds. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI")
	void ApplyStun(float Duration);

	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsStunned() const;

	/**
	 * Server only. Pops the enemy along Direction (2D) with a little lift, so it actually flies back instead of
	 * being stopped by ground friction. KnockbackResistance reduces it.
	 */
	void ApplyKnockback(const FVector& Direction, float Strength, float Lift);

	/** Server only. Walk speed is multiplied by SpeedMultiplier for Duration seconds (strongest slow wins). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI")
	void ApplySlow(float SpeedMultiplier, float Duration);

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;
	virtual void InitClassDefaults() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BroadcastInitialValues() override;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI")
	EEnemyAttackType AttackType = EEnemyAttackType::Melee;

	/** Edge-to-edge distance (capsule to capsule) at which the enemy can attack. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.0"))
	float AttackRange = 80.f;

	/** Ranged only: stops moving once this close (edge-to-edge). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.0", EditCondition = "AttackType == EEnemyAttackType::Ranged"))
	float PreferredDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.05"))
	float AttackCooldown = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.f;

	/** 0 = full knockback, 1 = immune (bosses). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Hit Feel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float KnockbackResistance = 0.f;

	/** This enemy type's health compared to the shared default attributes (ranged 0.7 = squishier). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.05"))
	float HealthMultiplier = 1.f;

	/** Melee only: delay between the attack starting and the hit landing (target must still be in range). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.0", EditCondition = "AttackType == EEnemyAttackType::Melee"))
	float MeleeHitDelay = 0.3f;

	/** Damage GameplayEffect (e.g. GE_Damage, uses the Combat.Data.Damage SetByCaller). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (EditCondition = "AttackType == EEnemyAttackType::Ranged"))
	FProjectileParams RangedProjectile;

	/** Optional montage played on every machine when attacking. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.05"))
	float TargetSearchInterval = 0.25f;

	/**
	 * Package focus: within this distance the enemy goes for the package instead of the nearest player
	 * (attacks the carrier, or grabs it from the ground and runs away from the players). 0 = ignore the package.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.0"))
	float PackageFocusRadius = 900.f;

	/** How fast the enemy turns to face its target (0 = instant). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | AI", meta = (ClampMin = "0.0"))
	float TurnInterpSpeed = 10.f;

	/** Team XP dropped as an orb on death. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Rewards", meta = (ClampMin = "0.0"))
	float XPReward = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Rewards")
	TSubclassOf<ARougeXPOrb> XPOrbClass;

private:
	UPROPERTY(ReplicatedUsing=OnRep_InitAttributes)
	bool bInitAttributes = false;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URougeAbilitySystemComponent> RougeAbilitySystemComp;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URougeAttributeSet> RougeAttributes;

	UFUNCTION()
	void OnRep_InitAttributes();

	// --- AI (server only) ---
	TWeakObjectPtr<ARougeCharacter> CurrentTarget;
	TWeakObjectPtr<ARougePackage> CachedPackage;
	float NextTargetSearchTime = 0.f;
	float NextAttackTime = 0.f;
	bool bDead = false;
	float StunnedUntilTime = 0.f;
	FTimerHandle MeleeHitTimer;

	float SlowedUntilTime = 0.f;
	float SlowSpeedMultiplier = 1.f;
	/** Walk speed before any slow (-1 until the first slow). */
	float UnslowedWalkSpeed = -1.f;

	void UpdateSlow();

	/** Server, after the default attributes: health / damage from the GameState difficulty curve. */
	void ApplyDifficultyScaling();

	void UpdateTarget();
	void TickAI(float DeltaTime);

	/** Package behaviour; returns true when it took over movement this frame. */
	bool TickPackageAI(float DeltaTime);
	void MoveAndFace(const FVector& Direction, float DeltaTime);
	float GetEdgeDistanceTo(const AActor* Other) const;
	void StartAttack();
	void ApplyMeleeHit();
	void FireProjectile();
	void ApplyDamageTo(AActor* Target);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackMontage();
};
