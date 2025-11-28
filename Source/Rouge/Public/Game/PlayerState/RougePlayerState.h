// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "RougePlayerState.generated.h"

class URougeAbilitySystemComponent;
class URougeAttributeSet;

/**
 * 
 */
UCLASS()
class ROUGE_API ARougePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()


public:
	ARougePlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const  override;


	UFUNCTION(BlueprintPure)
	URougeAbilitySystemComponent* GetRougeAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure)
	URougeAttributeSet* GetRougeAttributeSet() const;
private:

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URougeAbilitySystemComponent> RougeAbilitySystemComp;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URougeAttributeSet> RougeAttributes;


};
