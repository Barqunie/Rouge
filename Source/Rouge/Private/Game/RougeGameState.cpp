// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/RougeGameState.h"

#include "Character/RougeCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

void ARougeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARougeGameState, TeamXP);
	DOREPLIFETIME(ARougeGameState, TeamLevel);
}

float ARougeGameState::GetXPToNextLevel() const
{
	return BaseXPToLevel + XPGrowthPerLevel * (TeamLevel - 1);
}

void ARougeGameState::AddTeamXP(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f)
	{
		return;
	}

	TeamXP += Amount;

	while (TeamXP >= GetXPToNextLevel())
	{
		TeamXP -= GetXPToNextLevel();
		++TeamLevel;
	}

	ForceNetUpdate();

	// Server doesn't get OnReps.
	HandleLevelChange();
	BroadcastTeamXP();
}

void ARougeGameState::OnRep_TeamProgress()
{
	HandleLevelChange();
	BroadcastTeamXP();
}

void ARougeGameState::HandleLevelChange()
{
	if (TeamLevel <= LastBroadcastLevel)
	{
		LastBroadcastLevel = TeamLevel;
		return;
	}

	LastBroadcastLevel = TeamLevel;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("LEVEL UP! Team level %d"), TeamLevel));
	}

	OnTeamLevelUp.Broadcast(TeamLevel);
}

void ARougeGameState::BroadcastTeamXP()
{
	const float XPToNext = GetXPToNextLevel();

	OnTeamXPChanged.Broadcast(TeamXP, XPToNext, TeamLevel);

	// Feed the existing HUD XP bar (character OnXPChanged -> WB_Hud.SetXpBar) for local players.
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			if (PlayerController && PlayerController->IsLocalController())
			{
				if (ARougeCharacter* RougeCharacter = PlayerController->GetPawn<ARougeCharacter>())
				{
					RougeCharacter->OnXPChanged(TeamXP, XPToNext);
				}
			}
		}
	}
}
