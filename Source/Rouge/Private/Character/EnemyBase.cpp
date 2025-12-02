// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyBase.h"
#include "AbilitySystem/RougeAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Data/CharacterClassInfo.h"
#include "Net/UnrealNetwork.h"

#include <Libraries/RougeAbilitySystemLibrary.h>

AEnemyBase::AEnemyBase()
{
	bReplicates = true;

	RougeAbilitySystemComp = CreateDefaultSubobject<URougeAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	RougeAbilitySystemComp->SetIsReplicated(true);
	RougeAbilitySystemComp->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	RougeAttributes = CreateDefaultSubobject<URougeAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AEnemyBase::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

void AEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnemyBase, bInitAttributes);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	BindCallbacksToDependencies();
	InitAbilityActorInfo();


}

void AEnemyBase::InitAbilityActorInfo()
{
	if(IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->InitAbilityActorInfo(this, this);
		if(HasAuthority())
		{
			InitClassDefaults();
			BroadcastInitialValues();
		}
	}
}

void AEnemyBase::InitClassDefaults()
{
	if(!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("'%s' CharacterTag is not valid! Please set a valid GameplayTag for the character in the class defaults."), *GetNameSafe(this));
	}
	else if(UCharacterClassInfo* ClassInfo = URougeAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if(const FCharacterClassDefaultInfo* SelectedClass = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if(IsValid(RougeAbilitySystemComp))
			{
				RougeAbilitySystemComp->InitializeDefaultAttributes(SelectedClass->DefaultAttributes);
				RougeAbilitySystemComp->AddCharacterAbilities(SelectedClass->StartingAbilities);
				RougeAbilitySystemComp->AddCharacterPassiveAbilities(SelectedClass->PassiveAbilities);
			}

		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("'%s' Could not find default info for CharacterTag '%s' in CharacterClassInfo data asset."), *GetNameSafe(this), *CharacterTag.ToString());
		}
	}
}

void AEnemyBase::BindCallbacksToDependencies()
{
	if(IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(RougeAttributes->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, RougeAttributes->GetMaxHealth());
			});

		if(HasAuthority())
		{
			RougeAbilitySystemComp->OnAttributesGiven.AddLambda(
			[this]()
			{
					bInitAttributes = true;
			});
		}
	}
}

void AEnemyBase::BroadcastInitialValues()
{
	if(IsValid(RougeAttributes))
	{
		OnHealthChanged(RougeAttributes->GetHealth(), RougeAttributes->GetMaxHealth());
	}
}

void AEnemyBase::OnRep_InitAttributes()
{
	BroadcastInitialValues();
}
