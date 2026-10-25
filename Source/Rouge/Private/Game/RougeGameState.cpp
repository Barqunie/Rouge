// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/RougeGameState.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"

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
	DOREPLIFETIME(ARougeGameState, RunResult);
	DOREPLIFETIME(ARougeGameState, CurrentWave);
	DOREPLIFETIME(ARougeGameState, TotalWaves);
	DOREPLIFETIME(ARougeGameState, EnemiesRemaining);
	DOREPLIFETIME(ARougeGameState, WavePhase);
	DOREPLIFETIME(ARougeGameState, PhaseEndServerTime);
}

float ARougeGameState::GetBreakTimeRemaining() const
{
	return WavePhase == ERougeWavePhase::Break ? FMath::Max(0.f, PhaseEndServerTime - GetServerWorldTimeSeconds()) : 0.f;
}

void ARougeGameState::SetWaveState(int32 InCurrentWave, int32 InTotalWaves, ERougeWavePhase InPhase, int32 InEnemiesRemaining, float InPhaseEndServerTime)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentWave = InCurrentWave;
	TotalWaves = InTotalWaves;
	WavePhase = InPhase;
	EnemiesRemaining = InEnemiesRemaining;
	PhaseEndServerTime = InPhaseEndServerTime;
}

void ARougeGameState::SetRunResult(ERougeRunResult NewResult)
{
	if (!HasAuthority() || RunResult == NewResult)
	{
		return;
	}
	RunResult = NewResult;
	ForceNetUpdate();
	OnRunResultChanged.Broadcast(RunResult);
}

void ARougeGameState::MulticastChemCue_Implementation(FGameplayTag CueTag, FVector_NetQuantize Location, float RawMagnitude)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
	{
		FGameplayCueParameters Parameters;
		Parameters.Location = Location;
		Parameters.RawMagnitude = RawMagnitude;
		CueManager->ExecuteGameplayCue_NonReplicated(this, CueTag, Parameters);
	}
}

void ARougeGameState::OnRep_RunResult()
{
	OnRunResultChanged.Broadcast(RunResult);
}

float ARougeGameState::GetXPToNextLevel() const
{
	const float PlayerScale = 1.f + XPPerExtraPlayer * (GetPlayerCount() - 1);
	return (BaseXPToLevel + XPGrowthPerLevel * (TeamLevel - 1)) * PlayerScale;
}

int32 ARougeGameState::GetPlayerCount() const
{
	return FMath::Max(1, PlayerArray.Num());
}

float ARougeGameState::GetEnemyHealthMultiplier() const
{
	return (1.f + EnemyHealthPerLevel * (TeamLevel - 1)) * (1.f + EnemyHealthPerExtraPlayer * (GetPlayerCount() - 1));
}

float ARougeGameState::GetEnemyDamageMultiplier() const
{
	return 1.f + EnemyDamagePerLevel * (TeamLevel - 1);
}

float ARougeGameState::GetSpawnRateMultiplier() const
{
	return FMath::Min(1.f + SpawnRatePerLevel * (TeamLevel - 1), MaxSpawnRateMultiplier);
}

float ARougeGameState::GetEnemyCapMultiplier() const
{
	return FMath::Min(1.f + EnemyCapPerLevel * (TeamLevel - 1), MaxEnemyCapMultiplier);
}

float ARougeGameState::GetEnemyCountMultiplier() const
{
	return 1.f + EnemyCountPerExtraPlayer * (GetPlayerCount() - 1);
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
					RougeCharacter->RefreshStats();
				}
			}
		}
	}
}
