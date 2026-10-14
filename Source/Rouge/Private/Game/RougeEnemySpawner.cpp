// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/RougeEnemySpawner.h"

#include "Character/EnemyBase.h"
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

	GetWorldTimerManager().SetTimer(SpawnTimer, this, &ThisClass::SpawnBatch, SpawnInterval, true, InitialDelay);
}

void ARougeEnemySpawner::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
}

float ARougeEnemySpawner::GetPlayerScale() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const int32 PlayerCount = GameState ? FMath::Max(1, GameState->PlayerArray.Num()) : 1;
	return 1.f + ExtraPlayerScale * (PlayerCount - 1);
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
	const int32 AliveCap = FMath::CeilToInt(MaxAliveEnemies * PlayerScale);
	const int32 BatchSize = FMath::CeilToInt(EnemiesPerSpawn * PlayerScale);

	const int32 FreeSlots = AliveCap - CountAliveEnemies();
	const int32 ToSpawn = FMath::Min(BatchSize, FreeSlots);

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
		GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLocation, SpawnRotation, SpawnParams);
	}
}
