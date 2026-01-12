// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "AbilitySystem/RougeAbilityTypes.h"

struct RougeDamageStatics
{
	//Source captures
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamage);


	//Target captures
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	RougeDamageStatics()
	{	//Source defines
		DEFINE_ATTRIBUTE_CAPTUREDEF(URougeAttributeSet, CritChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(URougeAttributeSet, CritDamage, Source, false);


		//Target defines

		DEFINE_ATTRIBUTE_CAPTUREDEF(URougeAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(URougeAttributeSet, DamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(URougeAttributeSet, Armor, Target, false);
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
	RelevantAttributesToCapture.Add(DamageStatics().CritChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritDamageDef);

	//target captures
	RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();



	FAggregatorEvaluateParameters EvalParameters;	
	EvalParameters.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParameters.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	const FGameplayEffectContextHandle EffectContextHandle = EffectSpec.GetContext();
	FRougeGameplayEffectContext* RougeContext = FRougeGameplayEffectContext::GetEffectContext(EffectContextHandle);

	//Source Captures
	float CritChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritChanceDef, EvalParameters, CritChance);
	CritChance = FMath::Max<float>(CritChance, 0.f);

	float CritDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritDamageDef, EvalParameters, CritDamage);
	CritDamage = FMath::Max<float>(CritDamage, 0.f);


	//Get raw damage value
	float Damage = EffectSpec.GetSetByCallerMagnitude(RougeGameplayTags::Combat::Data_Damage);
	Damage = FMath::Max<float>(Damage, 0.f);


	//target captures
	float Armor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvalParameters, Armor);
	Armor = FMath::Max<float>(Armor, 0.f);

	float DamageReduction = 0.f;	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageReductionDef, EvalParameters, DamageReduction);
	DamageReduction = FMath::Max<float>(DamageReduction, 0.f);




	//Begin Calculations

	const bool bCriticalHit = FMath::RandRange(0, 100) < CritChance;

	Damage = bCriticalHit ? Damage *= CritDamage  : Damage;
	RougeContext->SetIsCriticalHit(bCriticalHit);

	if (Damage > 0.f && Armor > 0.f)
	{
		Damage *= (100 - Armor) / 100.f;
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
	}


	if (Armor <= 0 && Damage >= 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
	}
}
