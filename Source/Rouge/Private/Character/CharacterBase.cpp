// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CharacterBase.h"


ACharacterBase::ACharacterBase()
{

	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;	

}


void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}
void ACharacterBase::InitAbilityActorInfo()
{

	
}

void ACharacterBase::BindCallbacksToDependencies()
{
}

void ACharacterBase::InitClassDefaults()
{
}

void ACharacterBase::BroadcastInitialValues()
{
}	


