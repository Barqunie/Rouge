// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/RougeMainGameMode.h"
#include "Engine/GameInstance.h"
#include "Game/RougeCharacterSelectionSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Character/RougeCharacter.h"
#include "Game/RougeGameState.h"

ARougeMainGameMode::ARougeMainGameMode()
{
	// Shared team XP / level lives on the game state.
	GameStateClass = ARougeGameState::StaticClass();
}

UClass* ARougeMainGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (IsValid(InController))
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const URougeCharacterSelectionSubsystem* Selection = GameInstance->GetSubsystem<URougeCharacterSelectionSubsystem>())
			{
				if (UClass* SelectedClass = Selection->GetSelectedCharacter(InController))
				{
					return SelectedClass;
				}
			}
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ARougeMainGameMode::NotifyPlayerDied(ARougeCharacter* DeadCharacter)
{
	if (!GameState)
	{
		return;
	}

	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ARougeCharacter* PlayerCharacter = PlayerState ? PlayerState->GetPawn<ARougeCharacter>() : nullptr;
		if (PlayerCharacter && !PlayerCharacter->IsDead())
		{
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("RougeMainGameMode: all players are dead."));
	OnAllPlayersDead();
}

UCharacterClassInfo* ARougeMainGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

UProjectileInfo* ARougeMainGameMode::GetProjectileInfo() const
{
	return ProjectileInfo;
}
