// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RougeEnemySpawner.generated.h"

class AEnemyBase;
class UBillboardComponent;

USTRUCT(BlueprintType)
struct FRougeEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AEnemyBase> EnemyClass;

	/** Relative chance to pick this entry (e.g. 7 and 3 = 70% / 30%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

/**
 * Place in the level. Server only: every SpawnInterval it spawns random enemies at random ground points
 * inside SpawnRadius, away from players, until the world-wide alive cap is reached.
 * Spawn count and cap scale with the number of players.
 */
UCLASS()
class ROUGE_API ARougeEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	ARougeEnemySpawner();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Spawn")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Spawn")
	void StopSpawning();

	/** Spawns one batch right now (ignores the timer, respects the alive cap). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Spawn")
	void SpawnBatch();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TArray<FRougeEnemySpawnEntry> EnemyTypes;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	bool bStartOnBeginPlay = true;

	/** Quiet start so players can get moving. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.0"))
	float InitialDelay = 4.f;

	/** Seconds between batches for one player at level 1 (divided by the level and player count multipliers). */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.1"))
	float SpawnInterval = 6.f;

	/** Enemies per batch for one player. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1"))
	int32 EnemiesPerSpawn = 1;

	/** Max enemies alive in the whole world (all spawners together) for one player at level 1. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1"))
	int32 MaxAliveEnemies = 10;

	/** Enemies spawn at random points inside this radius around the spawner. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnRadius = 800.f;

	/** No spawning closer than this to any player (at least the ranged enemies' attack range). */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.0"))
	float MinDistanceFromPlayers = 950.f;

	/** Random points tried per enemy before giving up. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1"))
	int32 MaxPointAttempts = 8;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;
#endif

private:
	FTimerHandle SpawnTimer;

	/** Spawns a batch, then schedules the next one (the interval shrinks as the team levels up). */
	void HandleSpawnTimer();

	float GetPlayerScale() const;
	int32 CountAliveEnemies() const;
	TSubclassOf<AEnemyBase> PickEnemyClass() const;
	bool FindSpawnLocation(TSubclassOf<AEnemyBase> EnemyClass, FVector& OutLocation) const;
	bool IsTooCloseToPlayers(const FVector& Location) const;
};
