// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "GameplayTagContainer.h"	
#include "AbilitySystemInterface.h"
#include <Input/RougeInputConfig.h>
#include "RougePlayerController.generated.h"


class UInputMappingContext;
class UUserWidget;
class UAbilitySystemComponent;
class URougeAbilitySystemComponent;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ARougePlayerController : public APlayerController, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;


	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	void AbilityInputPressed(FGameplayTag InputTag);
	void AbilityInputReleased(FGameplayTag InputTag);

public:
	//ARougePlayerController();

	virtual void SetupInputComponent() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

private:
	UPROPERTY()	
	TObjectPtr<URougeAbilitySystemComponent> RougeAbilitySystemComp;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Input")
	TObjectPtr<URougeInputConfig> RougeInputConfig;

	URougeAbilitySystemComponent* GetRougeAbilitySystemComponent();



};

