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
	float NextTargetSearchTime = 0.f;
	float NextAttackTime = 0.f;
	bool bDead = false;
	FTimerHandle MeleeHitTimer;

	void UpdateTarget();
	void TickAI(float DeltaTime);
	float GetEdgeDistanceTo(const AActor* Other) const;
	void StartAttack();
	void ApplyMeleeHit();
	void FireProjectile();
	void ApplyDamageTo(AActor* Target);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackMontage();
};
