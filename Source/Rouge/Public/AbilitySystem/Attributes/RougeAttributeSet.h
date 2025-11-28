// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "RougeAttributeSet.generated.h"


#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)	



/**
 * 
 */
UCLASS()
class ROUGE_API URougeAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;	
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;	

	UPROPERTY(BlueprintReadOnly,  ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, MaxHealth);


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_XP)
	FGameplayAttributeData XP;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, XP);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxXP)
	FGameplayAttributeData MaxXP;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, MaxXP);



	UPROPERTY(BlueprintReadOnly,  ReplicatedUsing=OnRep_CritChance)
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, CritChance);


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Damage)
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, Damage);


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed)
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, AttackSpeed);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(URougeAttributeSet, Armor);





private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_Damage(const FGameplayAttributeData& OldDamage);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldArmor);

	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldCritChance);

	UFUNCTION()
	void OnRep_XP(const FGameplayAttributeData& OldXP);

	UFUNCTION()
	void OnRep_MaxXP(const FGameplayAttributeData& OldMaxXP);
	

	
};
