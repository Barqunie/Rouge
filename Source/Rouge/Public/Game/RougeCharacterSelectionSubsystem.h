// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RougeCharacterSelectionSubsystem.generated.h"

class AController;
class APawn;
class APlayerState;

/**
 * Server-side store of each player's selected character class.
 * Lives on the GameInstance, so it survives ServerTravel (seamless or not) from the lobby to the game map.
 * Players are keyed by their unique net id (Steam id), falling back to player name when no id is available.
 */
UCLASS()
class ROUGE_API URougeCharacterSelectionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Server only. Call from the lobby GameMode when a player picks a character.
	 * CharacterClass must be a Pawn class (Actor class accepted so lobby Actor-class arrays plug in directly).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Character Selection")
	void SetSelectedCharacter(const AController* Controller, TSubclassOf<AActor> CharacterClass);

	/** Server only. Optional cleanup, e.g. when a player leaves the lobby. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Character Selection")
	void ClearSelectedCharacter(const AController* Controller);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Character Selection")
	void ClearAllSelections();

	/** Returns the selected class, or null if the player has not selected one. */
	UFUNCTION(BlueprintPure, Category = "Character Selection")
	TSubclassOf<APawn> GetSelectedCharacter(const AController* Controller) const;

	TSubclassOf<APawn> GetSelectedCharacterForPlayerState(const APlayerState* PlayerState) const;

private:
	static FString MakeSelectionKey(const APlayerState* PlayerState);

	UPROPERTY()
	TMap<FString, TSubclassOf<APawn>> SelectedCharacters;
};
