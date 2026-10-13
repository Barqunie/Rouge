// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerState/RougePlayerState.h"
#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Net/UnrealNetwork.h"

ARougePlayerState::ARougePlayerState()
{
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(66.f);

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

void ARougePlayerState::SetSelectedCharacterTag(const FGameplayTag& NewTag)
{
	if (HasAuthority())
	{
		SelectedCharacterTag = NewTag;
	}
}

void ARougePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARougePlayerState, SelectedCharacterTag);
}

void ARougePlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ARougePlayerState* RougePlayerState = Cast<ARougePlayerState>(PlayerState))
	{
		RougePlayerState->SelectedCharacterTag = SelectedCharacterTag;
	}
}
