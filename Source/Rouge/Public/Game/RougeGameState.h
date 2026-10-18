// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "RougeGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeamXPChanged, float, CurrentXP, float, XPToNextLevel, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamLevelUp, int32, NewLevel);

/**
 * Shared team progression (LoL Swarm style): one XP bar and level for the whole team.
 * Server adds XP; values replicate to everyone. Local player characters get OnXPChanged so the HUD XP bar updates.
 */
UCLASS()
class ROUGE_API ARougeGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only. Adds XP to the team, handling any number of level ups. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Team XP")
	void AddTeamXP(float Amount);

	UFUNCTION(BlueprintPure, Category = "Team XP")
	float GetTeamXP() const { return TeamXP; }

	UFUNCTION(BlueprintPure, Category = "Team XP")
	int32 GetTeamLevel() const { return TeamLevel; }

	UFUNCTION(BlueprintPure, Category = "Team XP")
	float GetXPToNextLevel() const;

	/** Fires on every machine whenever team XP or level changes. */
	UPROPERTY(BlueprintAssignable, Category = "Team XP")
	FOnTeamXPChanged OnTeamXPChanged;

	/** Fires on every machine when the team levels up (augment card selection hooks in here). */
	UPROPERTY(BlueprintAssignable, Category = "Team XP")
	FOnTeamLevelUp OnTeamLevelUp;

	/** Pushes the current values to delegates and local player characters (HUD XP bar). */
	void BroadcastTeamXP();

	// ---------------- Difficulty ----------------
	// Enemies get tougher with the team level and with every extra player, spawns speed up with the level.
	// Read by enemies when they spawn and by the spawners for each batch (server).

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetPlayerCount() const;

	/** Applied to an enemy's max health when it spawns. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyHealthMultiplier() const;

	/** Applied to an enemy's attack damage when it spawns. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyDamageMultiplier() const;

	/** Spawn intervals are divided by this. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetSpawnRateMultiplier() const;

	/** Spawner alive caps are multiplied by this. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetEnemyCapMultiplier() const;

protected:
	/** XP needed for level 1 -> 2. */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "1.0"))
	float BaseXPToLevel = 100.f;

	/** Extra XP needed per level after that (linear growth). */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "0.0"))
	float XPGrowthPerLevel = 50.f;

	/** More players kill faster, so each extra player raises the XP needed per level by this fraction. */
	UPROPERTY(EditDefaultsOnly, Category = "Team XP", meta = (ClampMin = "0.0"))
	float XPPerExtraPlayer = 0.4f;

	/** Enemy health +this fraction per team level above 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyHealthPerLevel = 0.15f;

	/** Enemy health +this fraction per player above 1 (spawners also spawn more enemies per player). */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyHealthPerExtraPlayer = 0.35f;

	/** Enemy damage +this fraction per team level above 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyDamagePerLevel = 0.08f;

	/** Spawn rate +this fraction per team level above 1, up to MaxSpawnRateMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float SpawnRatePerLevel = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float MaxSpawnRateMultiplier = 3.f;

	/** Alive cap +this fraction per team level above 1, up to MaxEnemyCapMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float EnemyCapPerLevel = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float MaxEnemyCapMultiplier = 2.5f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamProgress)
	float TeamXP = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_TeamProgress)
	int32 TeamLevel = 1;

	/** Last level this machine announced, so clients can detect level ups from replication. */
	int32 LastBroadcastLevel = 1;

	UFUNCTION()
	void OnRep_TeamProgress();

	void HandleLevelChange();
};
