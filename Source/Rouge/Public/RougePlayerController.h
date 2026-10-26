// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "GameplayTagContainer.h"	
#include "AbilitySystemInterface.h"
#include "Interfaces/RougeAbilitySystemInterface.h"
#include <Input/RougeInputConfig.h>
#include "Augments/RougeAugmentTypes.h"
#include "UI/RougeDamageNumbersWidget.h"
#include "RougePlayerController.generated.h"


class UInputMappingContext;
class UUserWidget;
class UAbilitySystemComponent;
class URougeAbilitySystemComponent;
class URougeAugmentSelectionWidget;


UCLASS()
class ARougePlayerController : public APlayerController, public IAbilitySystemInterface, public IRougeAbilitySystemInterface
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

	virtual void PlayerTick(float DeltaTime) override;

	/** Dev only: set to an input tag (e.g. Input.QAbility) to press it once, as if the key was pressed. Lets tools test skills. */
	UPROPERTY(Transient, EditAnywhere, Category = "Debug")
	FGameplayTag DebugPressInputTag;


	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	void AbilityInputPressed(FGameplayTag InputTag);
	void AbilityInputReleased(FGameplayTag InputTag);

public:
	ARougePlayerController();

	virtual void SetupInputComponent() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/* Implement AbilitySystemInterface */

	virtual void SetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag ,int32 AbilityLevel) override;

	/** Console test command (non-shipping): kills your own character to test death / revive. */
	UFUNCTION(Exec)
	void RougeKillSelf();

	UFUNCTION(Server, Reliable)
	void ServerRougeKillSelf();

	// ---------------- Augment cards ----------------

	/** Server -> owning client: level up, show these cards (the game is paused). */
	UFUNCTION(Client, Reliable)
	void ClientShowAugmentOffers(const TArray<FRougeAugmentOffer>& Offers, int32 TeamLevel);

	/** Server -> owning client: everybody picked, close the cards. */
	UFUNCTION(Client, Reliable)
	void ClientAugmentRoundFinished();

	/** Local: the player clicked card OfferIndex. */
	void ChooseAugment(int32 OfferIndex);

	UFUNCTION(Server, Reliable)
	void ServerChooseAugment(int32 OfferIndex);

	/** Console test command (non-shipping): gives the team enough XP for one level. */
	UFUNCTION(Exec)
	void RougeLevelUp();

	UFUNCTION(Server, Reliable)
	void ServerRougeLevelUp();

	/** Console test command (non-shipping): finishes the active station (bar full, its wave removed). */
	UFUNCTION(Exec)
	void RougeSkipStation();

	UFUNCTION(Server, Reliable)
	void ServerRougeSkipStation();

	// ---------------- Damage numbers ----------------

	/** Server -> owning client: show a floating number at Location (damage this player dealt or took). */
	UFUNCTION(Client, Unreliable)
	void ClientShowDamageNumber(FVector_NetQuantize Location, int32 Amount, ERougeDamageNumberType Type);
private:
	UPROPERTY()	
	TObjectPtr<URougeAbilitySystemComponent> RougeAbilitySystemComp;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Input")
	TObjectPtr<URougeInputConfig> RougeInputConfig;

	/** Card picker shown on level up. */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Augments")
	TSubclassOf<URougeAugmentSelectionWidget> AugmentSelectionWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<URougeAugmentSelectionWidget> AugmentSelectionWidget;

	UPROPERTY(Transient)
	TObjectPtr<URougeDamageNumbersWidget> DamageNumbersWidget;

	/** Package objective HUD (status line, marker, result banner). */
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | HUD")
	TSubclassOf<class URougeObjectiveWidget> ObjectiveWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class URougeObjectiveWidget> ObjectiveWidget;

	void CloseAugmentSelection();

	URougeAbilitySystemComponent* GetRougeAbilitySystemComponent();



};

