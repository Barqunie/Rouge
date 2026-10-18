// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"

#include "ExecCalc_Damage.generated.h"

/**
 * 
 */
namespace RougeArmor
{
	/** Armor needed for 50% less damage. */
	constexpr float HalfReductionArmor = 100.f;

	/** Damage taken multiplier for an armor value: K / (K + Armor). */
	inline float GetDamageMultiplier(float Armor)
	{
		return HalfReductionArmor / (HalfReductionArmor + FMath::Max(Armor, 0.f));
	}
}

UCLASS()
class ROUGE_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
	
};
