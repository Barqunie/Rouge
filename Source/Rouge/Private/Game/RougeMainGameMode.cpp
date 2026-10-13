// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/RougeMainGameMode.h"
#include "Engine/GameInstance.h"
#include "Game/RougeCharacterSelectionSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

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

UCharacterClassInfo* ARougeMainGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

UProjectileInfo* ARougeMainGameMode::GetProjectileInfo() const
{
	return ProjectileInfo;
}
