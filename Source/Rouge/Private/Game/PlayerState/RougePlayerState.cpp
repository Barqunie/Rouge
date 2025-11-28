// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerState/RougePlayerState.h"
#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"

ARougePlayerState::ARougePlayerState()
{
	NetUpdateFrequency = 100.f;
	MinNetUpdateFrequency = 66.f;

	RougeAbilitySystemComp = CreateDefaultSubobject<URougeAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	RougeAbilitySystemComp->SetIsReplicated(true);
	RougeAbilitySystemComp->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	RougeAttributes = CreateDefaultSubobject<URougeAttributeSet>(TEXT("AttributeSet"));

}

UAbilitySystemComponent* ARougePlayerState::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

URougeAbilitySystemComponent* ARougePlayerState::GetRougeAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

URougeAttributeSet* ARougePlayerState::GetRougeAttributeSet() const
{
	return RougeAttributes;
}
