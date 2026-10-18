/// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/RougeGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool URougeGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (ActorInfo)
	{
		if (const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			if (ASC->HasMatchingGameplayTag(RougeGameplayTags::State::Dead))
			{
				return false;
			}
		}
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

float URougeGameplayAbility::GetAttackSpeedMultiplier() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasAttributeSetForAttribute(URougeAttributeSet::GetAttackSpeedAttribute()))
	{
		return 1.f;
	}

	// 0 (attribute never set) means normal speed.
	const float AttackSpeed = ASC->GetNumericAttribute(URougeAttributeSet::GetAttackSpeedAttribute());
	return AttackSpeed > 0.f ? FMath::Clamp(AttackSpeed, 0.2f, 5.f) : 1.f;
}

float URougeGameplayAbility::GetAttackInterval(float BaseInterval) const
{
	return FMath::Max(0.05f, BaseInterval / GetAttackSpeedMultiplier());
}

void URougeGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
