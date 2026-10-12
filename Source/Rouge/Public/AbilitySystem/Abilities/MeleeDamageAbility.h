#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeDamageAbility.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "MeleeDamageAbility.generated.h"

class UPrimitiveComponent;
class UAbilitySystemComponent;

/**
 * Melee hit registration done with a server-side shape query in front of the character.
 *
 * Hit shape: the avatar's component tagged HitComponentTag (e.g. the "MeleeCapsule" capsule in BP_RougeMelee,
 * so it can be positioned visually in the BP viewport). If none is found, a horizontal capsule in front of
 * the actor is used (Fallback* values).
 *
 * Timing (any of these):
 *  - BP calls DamageWindowOpen / DamageWindowClose (e.g. from timers)
 *  - AnimNotifyState_MeleeDamageWindow in the montage
 *  - BP calls PerformMeleeHit, or AnimNotify_MeleeHit in the montage, for a single instant hit
 *
 * Damage is applied only on the server. Each enemy is hit at most once per window.
 */
UCLASS()
class ROUGE_API UMeleeDamageAbility : public URougeDamageAbility
{
	GENERATED_BODY()

public:
	UMeleeDamageAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/** Hits everything in the shape now and keeps checking every HitCheckInterval until DamageWindowClose. */
	UFUNCTION(BlueprintCallable, Category = "Melee Damage")
	void DamageWindowOpen();

	UFUNCTION(BlueprintCallable, Category = "Melee Damage")
	void DamageWindowClose();

	/** Single instant hit check (does not need an open window). */
	UFUNCTION(BlueprintCallable, Category = "Melee Damage")
	void PerformMeleeHit();

	/** Optional override. If DamageEffect is set here it is used; otherwise the ability's DamageEffect / BaseDamage defaults are used. */
	UPROPERTY(BlueprintReadWrite, Category = "Melee Damage")
	FDamageEffectInfo DamageEffectInfo;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	bool bEnableMeleeDamage = true;

	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	FName EnemyTag = FName("Enemy");

	/** Component on the avatar whose collision shape is used as the hit area. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	FName HitComponentTag = FName("MeleeCapsule");

	/** How often the shape is re-checked while a damage window is open. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage", meta = (ClampMin = "0.01"))
	float HitCheckInterval = 0.05f;

	/** Fallback capsule (used when no tagged component is found): length along the forward direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage|Fallback Shape", meta = (ClampMin = "1.0"))
	float FallbackHitLength = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage|Fallback Shape", meta = (ClampMin = "1.0"))
	float FallbackHitRadius = 80.f;

	/** Distance from the actor location to where the fallback capsule starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage|Fallback Shape")
	float FallbackHitForwardOffset = 40.f;

	/** Listen for Event.Melee.WindowOpen / WindowClose / Hit sent by the melee anim notifies. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	bool bListenForAnimNotifyEvents = true;

	/** Draws the hit shape on the server (visible on the listen-server host). */
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage|Debug")
	bool bDrawDebugHitShape = false;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	bool InitMeleeData();

private:
	void CheckMeleeHits();
	bool GetHitShape(FVector& OutLocation, FQuat& OutRotation, FCollisionShape& OutShape) const;
	void ApplyMeleeDamage(AActor* TargetActor);
	void DrawDebugHitShape(const FVector& Location, const FQuat& Rotation, const FCollisionShape& Shape, bool bHitSomething) const;

	UFUNCTION()
	void OnWindowOpenEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnWindowCloseEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

	FTimerHandle HitCheckTimer;
};
