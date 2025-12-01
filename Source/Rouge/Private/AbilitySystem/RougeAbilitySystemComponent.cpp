// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/RougeGameplayAbility.h"




void URougeAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilitiesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : AbilitiesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);

		if(URougeGameplayAbility* RougeAbility = Cast<URougeGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.DynamicAbilityTags.AddTag(RougeAbility->InputTag);
			GiveAbility(AbilitySpec);
		}


	}
}

void URougeAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassivesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : PassivesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);
		GiveAbilityAndActivateOnce(AbilitySpec);;

	}
}

void URougeAbilitySystemComponent::InitializeDefaultAttributes(const TSubclassOf<UGameplayEffect>& AttributeEffect)
{
	checkf(
		AttributeEffect,
		TEXT("No Valid Default Attributes For This character: %s"),
		*GetAvatarActor()->GetName()
	);




	const FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(AttributeEffect, 1.f, ContextHandle);
	ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());


}

void URougeAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if(!InputTag.IsValid()){return;}

	ABILITYLIST_SCOPE_LOCK();

	for(const FGameplayAbilitySpec& Spec :GetActivatableAbilities())
	{
			if(Spec.DynamicAbilityTags.HasTagExact(InputTag))
			{
				if(!Spec.IsActive())
				{
					TryActivateAbility(Spec.Handle);
				}
				else
				{
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle,Spec.ActivationInfo.GetActivationPredictionKey());
				}
			}
	}
}

void URougeAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{

	if(!InputTag.IsValid()){return;}
	ABILITYLIST_SCOPE_LOCK();
	for(const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
			if(Spec.DynamicAbilityTags.HasTagExact(InputTag))
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle,Spec.ActivationInfo.GetActivationPredictionKey());
			}
	}
}
