#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RougeDamageAbility.h"
#include "AbilitySystem/RougeAbilityTypes.h"
#include "MeleeDamageAbility.generated.h"

class UPrimitiveComponent;
class UAbilitySystemComponent;

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

	UFUNCTION(BlueprintCallable, Category = "Melee Damage")
	void DamageWindowOpen();

	UFUNCTION(BlueprintCallable, Category = "Melee Damage")
	void DamageWindowClose();

	UFUNCTION()
	void OnMeleeOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UPROPERTY(BlueprintReadWrite, Category = "Melee Damage")
	FDamageEffectInfo DamageEffectInfo;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	bool bEnableMeleeDamage = true;

	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	FName EnemyTag = FName("Enemy");

	UPROPERTY(EditDefaultsOnly, Category = "Melee Damage")
	FName HitComponentTag = FName("MeleeCapsule");

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	bool InitMeleeData();

private:
	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;
};