// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/RougeCharacterSelectionSubsystem.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

FString URougeCharacterSelectionSubsystem::MakeSelectionKey(const APlayerState* PlayerState)
{
	if (!IsValid(PlayerState))
	{
		return FString();
	}

	const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
	if (UniqueId.IsValid())
	{
		return UniqueId.ToString();
	}

	return PlayerState->GetPlayerName();
}

void URougeCharacterSelectionSubsystem::SetSelectedCharacter(const AController* Controller, TSubclassOf<APawn> CharacterClass)
{
	const FString Key = MakeSelectionKey(Controller ? Controller->PlayerState : nullptr);
	if (Key.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("RougeCharacterSelection: SetSelectedCharacter called without a valid player."));
		return;
	}

	if (!CharacterClass)
	{
		SelectedCharacters.Remove(Key);
		return;
	}

	SelectedCharacters.Add(Key, CharacterClass);
}

void URougeCharacterSelectionSubsystem::ClearSelectedCharacter(const AController* Controller)
{
	const FString Key = MakeSelectionKey(Controller ? Controller->PlayerState : nullptr);
	if (!Key.IsEmpty())
	{
		SelectedCharacters.Remove(Key);
	}
}

void URougeCharacterSelectionSubsystem::ClearAllSelections()
{
	SelectedCharacters.Empty();
}

TSubclassOf<APawn> URougeCharacterSelectionSubsystem::GetSelectedCharacter(const AController* Controller) const
{
	return GetSelectedCharacterForPlayerState(Controller ? Controller->PlayerState : nullptr);
}

TSubclassOf<APawn> URougeCharacterSelectionSubsystem::GetSelectedCharacterForPlayerState(const APlayerState* PlayerState) const
{
	const FString Key = MakeSelectionKey(PlayerState);
	if (Key.IsEmpty())
	{
		return nullptr;
	}

	if (const TSubclassOf<APawn>* Found = SelectedCharacters.Find(Key))
	{
		return *Found;
	}

	return nullptr;
}
