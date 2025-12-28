// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystem/RougeGameplayTag.h"

struct RougeDamageStatics
{
	//Source captures


	//Target captures
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingHealthDamage);


	RougeDamageStatics()
	{	//Source defines



		//Target defines

		DEFINE_ATTRIBUTE_CAPTUREDEF(URougeAttributeSet, IncomingHealthDamage, Target, false);
	}
};



static const RougeDamageStatics& DamageStatics()
{
	static RougeDamageStatics DStatics;
	return DStatics;
}


UExecCalc_Damage::UExecCalc_Damage()
{
	//source captures

	//target captures
	RelevantAttributesToCapture.Add(DamageStatics().IncomingHealthDamageDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

	//Get raw damage value
	float Damage = EffectSpec.GetSetByCallerMagnitude(RougeGameplayTags::Combat::Data_Damage);
	Damage = FMath::Max<float>(Damage, 0.f);

	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingHealthDamageProperty, EGameplayModOp::Additive, Damage));
	}
}
