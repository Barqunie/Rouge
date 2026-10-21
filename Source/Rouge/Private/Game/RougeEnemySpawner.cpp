// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/RougeEnemySpawner.h"

#include "Character/EnemyBase.h"
#include "Game/RougeGameState.h"
#include "Game/RougeMainGameMode.h"
#include "Character/RougeCharacter.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

ARougeEnemySpawner::ARougeEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->SetupAttachment(RootComponent);
	}
#endif
}

void ARougeEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bStartOnBeginPlay)
	{
		StartSpawning();
	}
}

void ARougeEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	Super::EndPlay(EndPlayReason);
}

void ARougeEnemySpawner::StartSpawning()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(SpawnTimer, this, &ThisClass::HandleSpawnTimer, FMath::Max(0.01f, InitialDelay), false);
}

void ARougeEnemySpawner::HandleSpawnTimer()
{
	SpawnBatch();

	// Level and player count both shorten the interval (a smooth rate, unlike rounding batch sizes).
	const ARougeGameState* RougeGameState = GetWorld()->GetGameState<ARougeGameState>();
	const float RateMultiplier = RougeGameState ? RougeGameState->GetSpawnRateMultiplier() : 1.f;
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &ThisClass::HandleSpawnTimer,
		FMath::Max(0.25f, SpawnInterval / (RateMultiplier * GetPlayerScale())), false);
}

void ARougeEnemySpawner::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
}

float ARougeEnemySpawner::GetPlayerScale() const
{
	const ARougeGameState* RougeGameState = GetWorld()->GetGameState<ARougeGameState>();
	return RougeGameState ? RougeGameState->GetEnemyCountMultiplier() : 1.f;
}

int32 ARougeEnemySpawner::CountAliveEnemies() const
{
	int32 Count = 0;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			++Count;
		}
	}
	return Count;
}

TSubclassOf<AEnemyBase> ARougeEnemySpawner::PickEnemyClass() const
{
	float TotalWeight = 0.f;
	for (const FRougeEnemySpawnEntry& Entry : EnemyTypes)
	{
		if (Entry.EnemyClass && Entry.Weight > 0.f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRougeEnemySpawnEntry& Entry : EnemyTypes)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0.f)
		{
			continue;
		}

		Roll -= Entry.Weight;
		if (Roll <= 0.f)
		{
			return Entry.EnemyClass;
		}
	}

	return nullptr;
}

bool ARougeEnemySpawner::IsTooCloseToPlayers(const FVector& Location) const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		return false;
	}

	const float MinDistSq = FMath::Square(MinDistanceFromPlayers);
	for (const APlayerState* OtherPlayerState : GameState->PlayerArray)
	{
		const APawn* Pawn = OtherPlayerState ? OtherPlayerState->GetPawn() : nullptr;
		if (Pawn && FVector::DistSquared2D(Pawn->GetActorLocation(), Location) < MinDistSq)
		{
			return true;
		}
	}

	return false;
}

bool ARougeEnemySpawner::FindSpawnLocation(TSubclassOf<AEnemyBase> EnemyClass, FVector& OutLocation) const
{
	float HalfHeight = 90.f;
	if (const AEnemyBase* EnemyCDO = EnemyClass ? EnemyClass->GetDefaultObject<AEnemyBase>() : nullptr)
	{
		if (const UCapsuleComponent* Capsule = EnemyCDO->GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		}
	}

	const FVector Center = GetActorLocation();

	for (int32 Attempt = 0; Attempt < MaxPointAttempts; ++Attempt)
	{
		const FVector2D Offset = FMath::RandPointInCircle(SpawnRadius);
		const FVector Candidate = Center + FVector(Offset.X, Offset.Y, 0.f);

		if (IsTooCloseToPlayers(Candidate))
		{
			continue;
		}

		// Snap to the ground under the point.
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySpawnGround), false, this);
		const FVector TraceStart = Candidate + FVector(0.f, 0.f, 500.f);
		const FVector TraceEnd = Candidate - FVector(0.f, 0.f, 2000.f);

		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			OutLocation = Hit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
			return true;
		}
	}

	return false;
}

void ARougeEnemySpawner::SpawnBatch()
{
	if (!HasAuthority())
	{
		return;
	}

	const float PlayerScale = GetPlayerScale();
	const ARougeGameState* RougeGameState = GetWorld()->GetGameState<ARougeGameState>();
	const float CapMultiplier = RougeGameState ? RougeGameState->GetEnemyCapMultiplier() : 1.f;
	int32 AliveCap = FMath::RoundToInt(MaxAliveEnemies * PlayerScale * CapMultiplier);
	if (RougeGameState)
	{
		AliveCap = FMath::Min(AliveCap, RougeGameState->GetMaxAliveEnemiesTotal());
	}
	// Player count already shortens the interval; the batch stays the same size.
	const int32 BatchSize = EnemiesPerSpawn;

	const int32 FreeSlots = AliveCap - CountAliveEnemies();
	int32 ToSpawn = FMath::Min(BatchSize, FreeSlots);

	// Waves: only what is left of the current wave (nothing during breaks).
	ARougeMainGameMode* GameMode = GetWorld()->GetAuthGameMode<ARougeMainGameMode>();
	if (GameMode)
	{
		ToSpawn = FMath::Min(ToSpawn, GameMode->GetWaveSpawnBudget());
	}

	for (int32 Index = 0; Index < ToSpawn; ++Index)
	{
		const TSubclassOf<AEnemyBase> EnemyClass = PickEnemyClass();
		if (!EnemyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("RougeEnemySpawner %s: EnemyTypes is empty."), *GetName());
			return;
		}

		FVector SpawnLocation;
		if (!FindSpawnLocation(EnemyClass, SpawnLocation))
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		const FRotator SpawnRotation(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
		if (GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLocation, SpawnRotation, SpawnParams) && GameMode)
		{
			GameMode->NotifyWaveEnemySpawned();
		}
	}
}
