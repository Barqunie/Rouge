// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
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

	/** CharacterTag of the character this player is playing (Character.Melee / Ranged / Tank). Replicated to everyone. */
	UFUNCTION(BlueprintPure, Category = "Character Selection")
	FGameplayTag GetSelectedCharacterTag() const { return SelectedCharacterTag; }

	/** Server only. Set when the player's character is possessed. */
	void SetSelectedCharacterTag(const FGameplayTag& NewTag);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: name set/changed. Notifies the pawn so its overhead name updates. */
	virtual void SetPlayerName(const FString& S) override;

	/** Clients: name replicated. Notifies the pawn so its overhead name updates. */
	virtual void OnRep_PlayerName() override;

protected:
	/** Keeps the selection through seamless travel. */
	virtual void CopyProperties(APlayerState* PlayerState) override;

private:
	UPROPERTY(Replicated)
	FGameplayTag SelectedCharacterTag;


	UPROPERTY(VisibleAnywhere ,BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URougeAbilitySystemComponent> RougeAbilitySystemComp;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URougeAttributeSet> RougeAttributes;


};
