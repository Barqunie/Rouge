// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CharacterBase.generated.h"

UCLASS()
class ROUGE_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:

	ACharacterBase();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent)
	void OnXPChanged(float CurrentXP, float MaxXP);

protected:

	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();
	virtual void BindCallbacksToDependencies();
	virtual void InitClassDefaults();

	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValues();

	UPROPERTY(EditAnywhere, Category = "Custom Values | Character Info")
	FGameplayTag CharacterTag;

public:	


};
