// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/RougeMainGameMode.h"
#include "Engine/GameInstance.h"
#include "Game/RougeCharacterSelectionSubsystem.h"
#include "Actors/RougePackage.h"
#include "Actors/RougeStation.h"
#include "Game/RougeEnemySpawner.h"
#include "Character/EnemyBase.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Character/RougeCharacter.h"
#include "Game/RougeGameState.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "Augments/RougeAugmentComponent.h"
#include "Augments/RougeAugmentPool.h"
#include "Game/PlayerState/RougePlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "RougePlayerController.h"

ARougeMainGameMode::ARougeMainGameMode()
{
	// Shared team XP / level lives on the game state.
	GameStateClass = ARougeGameState::StaticClass();
	PackageClass = ARougePackage::StaticClass();
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
	EndRun(false);
}

// ---------------------------------------------------------------------------
// Package run
// ---------------------------------------------------------------------------

void ARougeMainGameMode::SpawnPackage()
{
	if (!PackageClass || ARougePackage::Find(GetWorld()))
	{
		return;
	}

	// Tagged start actor, else beside the first player start.
	FVector Location = FVector::ZeroVector;
	bool bFound = false;
	for (TActorIterator<AActor> It(GetWorld()); It && !PackageStartTag.IsNone(); ++It)
	{
		if (It->ActorHasTag(PackageStartTag))
		{
			Location = It->GetActorLocation();
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			Location = It->GetActorLocation() + It->GetActorForwardVector() * 250.f;
			bFound = true;
			break;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<ARougePackage>(PackageClass, Location, FRotator::ZeroRotator, SpawnParams);
}

// ---------------------------------------------------------------------------
// Waves
// ---------------------------------------------------------------------------

int32 ARougeMainGameMode::GetWaveSpawnBudget() const
{
	return bStageWaveStarted ? WaveEnemiesToSpawn : 0;
}

bool ARougeMainGameMode::ShouldSpawnerSpawn(const AActor* Spawner) const
{
	const ARougeStation* Station = ActiveStation.Get();
	if (!Spawner || !Station || bAllSpawnersForStage)
	{
		return true;
	}
	return FVector::Dist2D(Spawner->GetActorLocation(), Station->GetActorLocation()) <= StationSpawnerRange;
}

int32 ARougeMainGameMode::GetTotalStages() const
{
	return Stations.Num() + (FinalStation.IsValid() ? 1 : 0);
}

void ARougeMainGameMode::NotifyWaveEnemySpawned()
{
	WaveEnemiesToSpawn = FMath::Max(0, WaveEnemiesToSpawn - 1);
}

int32 ARougeMainGameMode::CountAliveEnemies() const
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

void ARougeMainGameMode::SetupStations()
{
	TArray<ARougeStation*> Found = ARougeStation::FindAll(GetWorld());

	// No stations in the map: spawn the fallback layout so the run can still be played.
	if (Found.Num() == 0 && bSpawnStationsIfMissing)
	{
		auto SpawnStation = [this](const FVector& Wanted, bool bFinal)
		{
			FVector Location = Wanted;
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(StationGround), false);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Wanted + FVector(0.f, 0.f, 1000.f), Wanted - FVector(0.f, 0.f, 2000.f), ECC_Visibility, Params))
			{
				Location = Hit.ImpactPoint;
			}
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ARougeStation* Station = GetWorld()->SpawnActor<ARougeStation>(ARougeStation::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
			if (Station)
			{
				Station->SetIsFinal(bFinal);
			}
			return Station;
		};
		for (const FVector& Location : FallbackStationLocations)
		{
			Found.Add(SpawnStation(Location, false));
		}
		Found.Add(SpawnStation(FallbackFinalLocation, true));
	}

	int32 Number = 0;
	for (ARougeStation* Station : Found)
	{
		if (!Station)
		{
			continue;
		}
		if (Station->IsFinal() && !FinalStation.IsValid())
		{
			FinalStation = Station;
			Station->SetStationNumber(0);
		}
		else
		{
			Stations.Add(Station);
			Station->SetStationNumber(++Number);
		}
		Station->SetState(ERougeStationState::Idle);
	}
}

void ARougeMainGameMode::ActivateNextStation()
{
	TArray<ARougeStation*> Remaining;
	for (const TWeakObjectPtr<ARougeStation>& Station : Stations)
	{
		if (Station.IsValid() && Station->GetState() != ERougeStationState::Purified)
		{
			Remaining.Add(Station.Get());
		}
	}

	ARougeStation* Next = Remaining.Num() > 0 ? Remaining[FMath::RandRange(0, Remaining.Num() - 1)] : FinalStation.Get();
	ActiveStation = Next;
	bStageWaveStarted = false;
	WaveEnemiesToSpawn = 0;
	++StageNumber;

	if (Next)
	{
		Next->SetState(ERougeStationState::Active);
		Next->SetProgress(0.f);

		// Use the spawners around it; if none is close, all of them.
		bAllSpawnersForStage = true;
		for (TActorIterator<ARougeEnemySpawner> It(GetWorld()); It; ++It)
		{
			if (FVector::Dist2D(It->GetActorLocation(), Next->GetActorLocation()) <= StationSpawnerRange)
			{
				bAllSpawnersForStage = false;
				break;
			}
		}
	}

	if (ARougeGameState* RougeGameState = GetGameState<ARougeGameState>())
	{
		RougeGameState->SetActiveStation(Next);
		RougeGameState->SetWaveState(StageNumber, GetTotalStages(), ERougeWavePhase::Break, 0, 0.f);
	}
}

void ARougeMainGameMode::StartStageWave()
{
	bStageWaveStarted = true;

	// Bigger every station; the final is the next step times FinalWaveMultiplier. Player count scales it like
	// the spawners, so the wave is sized for the team.
	const ARougeStation* Station = ActiveStation.Get();
	const bool bFinal = Station && Station->IsFinal();
	const float Base = FirstWaveEnemies + EnemiesAddedPerWave * (StageNumber - 1);
	const ARougeGameState* RougeGameState = GetGameState<ARougeGameState>();
	const float PlayerScale = RougeGameState ? RougeGameState->GetEnemyCountMultiplier() : 1.f;
	WaveEnemiesToSpawn = FMath::Max(1, FMath::RoundToInt(Base * (bFinal ? FinalWaveMultiplier : 1.f) * PlayerScale));

	OnWaveStarted(StageNumber);
}

void ARougeMainGameMode::DebugCompleteActiveStation()
{
	if (!ActiveStation.IsValid() || bAllStagesDone)
	{
		return;
	}
	WaveEnemiesToSpawn = 0;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	CompleteStage();
}

void ARougeMainGameMode::CompleteStage()
{
	ARougeStation* Station = ActiveStation.Get();
	if (!Station)
	{
		return;
	}

	Station->SetState(ERougeStationState::Purified);
	Station->SetProgress(1.f);
	if (ARougePackage* Package = ARougePackage::Find(GetWorld()))
	{
		Package->Undock();
	}

	if (Station->IsFinal())
	{
		bAllStagesDone = true;
		if (ARougeGameState* RougeGameState = GetGameState<ARougeGameState>())
		{
			RougeGameState->SetWaveState(StageNumber, GetTotalStages(), ERougeWavePhase::Cleared, 0, 0.f);
		}
		EndRun(true);
		return;
	}

	OnStationPurified(StageNumber);
	ActivateNextStation();
}

void ARougeMainGameMode::UpdateStations()
{
	ARougeGameState* RougeGameState = GetGameState<ARougeGameState>();
	ARougeStation* Station = ActiveStation.Get();
	const float Now = GetWorld()->GetTimeSeconds();
	const float DeltaTime = LastStationUpdateTime > 0.f ? Now - LastStationUpdateTime : 0.f;
	LastStationUpdateTime = Now;
	if (!RougeGameState || !Station || bAllStagesDone || RougeGameState->IsRunOver())
	{
		return;
	}

	ARougePackage* Package = ARougePackage::Find(GetWorld());
	if (Package && Package->GetDockedStation() != Station)
	{
		// A player brings it in (or it lies inside after a drop / a killed thief): dock it.
		const bool bInside = FVector::Dist2D(Package->GetActorLocation(), Station->GetActorLocation()) <= Station->GetRadius();
		if (bInside && !Package->IsCarriedByEnemy())
		{
			Package->DockAt(Station, Station->GetDockLocation());
			if (!bStageWaveStarted)
			{
				Station->SetState(ERougeStationState::Purifying);
				StartStageWave();
			}
		}
	}

	// The bar only fills while the package sits on the pedestal.
	if (Package && Package->GetDockedStation() == Station)
	{
		Station->SetProgress(Station->GetProgress() + DeltaTime / FMath::Max(Station->GetPurifyDuration(), 0.1f));
	}

	const int32 Remaining = bStageWaveStarted ? WaveEnemiesToSpawn + CountAliveEnemies() : 0;
	if (bStageWaveStarted && Remaining == 0 && Station->GetProgress() >= 1.f)
	{
		CompleteStage();
		return;
	}

	RougeGameState->SetWaveState(StageNumber, GetTotalStages(),
		bStageWaveStarted ? ERougeWavePhase::Fighting : ERougeWavePhase::Break, Remaining, 0.f);
}

void ARougeMainGameMode::EndRun(bool bWon)
{
	ARougeGameState* RougeGameState = GetGameState<ARougeGameState>();
	if (!RougeGameState || RougeGameState->IsRunOver())
	{
		return;
	}

	RougeGameState->SetRunResult(bWon ? ERougeRunResult::Won : ERougeRunResult::Lost);

	for (TActorIterator<ARougeEnemySpawner> It(GetWorld()); It; ++It)
	{
		It->StopSpawning();
	}

	// Win: clear the field. Loss: everybody is down already.
	if (bWon)
	{
		TArray<AEnemyBase*> Enemies;
		for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
		{
			Enemies.Add(*It);
		}
		for (AEnemyBase* Enemy : Enemies)
		{
			Enemy->Destroy();
		}
	}

	OnRunEnded(bWon);

	if (!LobbyMap.IsEmpty())
	{
		GetWorldTimerManager().SetTimer(ReturnToLobbyTimer, this, &ThisClass::ReturnToLobby, FMath::Max(0.1f, ReturnToLobbyDelay), false);
	}
}

void ARougeMainGameMode::ReturnToLobby()
{
	GetWorld()->ServerTravel(LobbyMap + TEXT("?listen"));
}

// ---------------------------------------------------------------------------
// Augments
// ---------------------------------------------------------------------------

void ARougeMainGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnPackage();

	if (ARougeGameState* RougeGameState = GetGameState<ARougeGameState>())
	{
		LastHandledLevel = RougeGameState->GetTeamLevel();
		RougeGameState->OnTeamLevelUp.AddDynamic(this, &ThisClass::HandleTeamLevelUp);
	}

	SetupStations();
	if (GetTotalStages() > 0)
	{
		ActivateNextStation();
		GetWorldTimerManager().SetTimer(StationUpdateTimer, this, &ThisClass::UpdateStations, 0.1f, true);
	}
	else if (ARougeGameState* RougeGameState = GetGameState<ARougeGameState>())
	{
		// No stations at all: nothing to do but survive.
		RougeGameState->SetWaveState(0, 0, ERougeWavePhase::Cleared, 0, 0.f);
	}
}

void ARougeMainGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Don't wait for a player who left.
	if (bAugmentRoundActive && PendingOffers.Remove(Cast<ARougePlayerController>(Exiting)) > 0 && PendingOffers.Num() == 0)
	{
		FinishAugmentRound();
	}
}

const TArray<FRougeAugmentDefinition>& ARougeMainGameMode::GetAugments() const
{
	return AugmentPool && AugmentPool->Augments.Num() > 0 ? AugmentPool->Augments : URougeAugmentPool::GetDefaultAugments();
}

const FRougeAugmentDefinition* ARougeMainGameMode::FindAugment(FName AugmentId) const
{
	return GetAugments().FindByPredicate([AugmentId](const FRougeAugmentDefinition& Augment)
	{
		return Augment.Id == AugmentId;
	});
}

void ARougeMainGameMode::HandleTeamLevelUp(int32 NewLevel)
{
	// Burst on every player (shows behind the card picker and finishes when the game resumes).
	for (TActorIterator<ARougeCharacter> It(GetWorld()); It; ++It)
	{
		if (UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = It->GetActorLocation();
			ASC->ExecuteGameplayCue(RougeGameplayTags::Cue::Chem_LevelUp, CueParameters);
		}
	}

	PendingAugmentRounds += FMath::Max(0, NewLevel - LastHandledLevel);
	LastHandledLevel = FMath::Max(LastHandledLevel, NewLevel);

	if (!bAugmentRoundActive)
	{
		StartAugmentRound();
	}
}

TArray<FRougeRolledAugment> ARougeMainGameMode::RollOffers(const ARougePlayerController* PlayerController) const
{
	TArray<FRougeRolledAugment> Offers;

	const ARougePlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<ARougePlayerState>() : nullptr;
	const URougeAugmentComponent* Augments = PlayerState ? PlayerState->GetAugmentComponent() : nullptr;
	if (!Augments)
	{
		return Offers;
	}

	FGameplayTag ClassTag = PlayerState->GetSelectedCharacterTag();
	if (const ACharacterBase* Character = PlayerController->GetPawn<ACharacterBase>())
	{
		if (Character->GetCharacterTag().IsValid())
		{
			ClassTag = Character->GetCharacterTag();
		}
	}

	TArray<const FRougeAugmentDefinition*> Candidates;
	for (const FRougeAugmentDefinition& Augment : GetAugments())
	{
		const bool bClassAllowed = Augment.RequiredCharacterTags.IsEmpty() || ClassTag.MatchesAny(Augment.RequiredCharacterTags);
		if (Augment.Id != NAME_None && Augment.Weight > 0.f && bClassAllowed && Augments->GetStacks(Augment.Id) < Augment.MaxStacks)
		{
			Candidates.Add(&Augment);
		}
	}

	// Weighted pick without repeats.
	while (Offers.Num() < CardsPerOffer && Candidates.Num() > 0)
	{
		float TotalWeight = 0.f;
		for (const FRougeAugmentDefinition* Candidate : Candidates)
		{
			TotalWeight += Candidate->Weight * RougeAugments::GetRarityWeight(Candidate->Rarity);
		}

		float Roll = FMath::FRandRange(0.f, TotalWeight);
		int32 Picked = Candidates.Num() - 1;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Roll -= Candidates[Index]->Weight * RougeAugments::GetRarityWeight(Candidates[Index]->Rarity);
			if (Roll <= 0.f)
			{
				Picked = Index;
				break;
			}
		}

		Offers.Add({ Candidates[Picked]->Id, FMath::FRand() });
		Candidates.RemoveAtSwap(Picked);
	}

	return Offers;
}

FRougeAugmentOffer ARougeMainGameMode::MakeOffer(const FRougeAugmentDefinition& Augment, float Roll, const ARougePlayerState* PlayerState) const
{
	FRougeAugmentOffer Offer;
	Offer.Id = Augment.Id;
	Offer.DisplayName = Augment.DisplayName;
	Offer.Description = RougeAugments::FormatDescription(Augment, Roll);
	Offer.Category = Augment.Category;
	Offer.Rarity = Augment.Rarity;
	Offer.MaxStacks = Augment.MaxStacks;

	const URougeAugmentComponent* Augments = PlayerState ? PlayerState->GetAugmentComponent() : nullptr;
	Offer.CurrentStacks = Augments ? Augments->GetStacks(Augment.Id) : 0;
	return Offer;
}

void ARougeMainGameMode::StartAugmentRound()
{
	while (PendingAugmentRounds > 0 && !bAugmentRoundActive)
	{
		--PendingAugmentRounds;
		PendingOffers.Reset();

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ARougePlayerController* PlayerController = Cast<ARougePlayerController>(It->Get());
			const TArray<FRougeRolledAugment> Rolled = RollOffers(PlayerController);
			if (Rolled.Num() == 0)
			{
				continue;
			}

			TArray<FRougeAugmentOffer> Offers;
			for (const FRougeRolledAugment& Card : Rolled)
			{
				if (const FRougeAugmentDefinition* Augment = FindAugment(Card.Id))
				{
					Offers.Add(MakeOffer(*Augment, Card.Roll, PlayerController->GetPlayerState<ARougePlayerState>()));
				}
			}

			PendingOffers.Add(PlayerController, Rolled);
			PlayerController->ClientShowAugmentOffers(Offers, LastHandledLevel - PendingAugmentRounds);
		}

		bAugmentRoundActive = PendingOffers.Num() > 0;
	}

	SetAugmentPause(bAugmentRoundActive);
}

void ARougeMainGameMode::HandleAugmentChosen(ARougePlayerController* PlayerController, int32 OfferIndex)
{
	if (!bAugmentRoundActive || !PlayerController)
	{
		return;
	}

	const TArray<FRougeRolledAugment>* Offers = PendingOffers.Find(PlayerController);
	if (!Offers || !Offers->IsValidIndex(OfferIndex))
	{
		return;
	}

	const FRougeRolledAugment Card = (*Offers)[OfferIndex];
	ARougePlayerState* PlayerState = PlayerController->GetPlayerState<ARougePlayerState>();
	const FRougeAugmentDefinition* Augment = FindAugment(Card.Id);
	if (PlayerState && PlayerState->GetAugmentComponent() && Augment)
	{
		PlayerState->GetAugmentComponent()->ApplyAugment(*Augment, Card.Roll);
	}

	PendingOffers.Remove(PlayerController);

	// Drop players that disappeared without a logout.
	for (auto It = PendingOffers.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (PendingOffers.Num() == 0)
	{
		FinishAugmentRound();
	}
}

void ARougeMainGameMode::FinishAugmentRound()
{
	bAugmentRoundActive = false;
	PendingOffers.Reset();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ARougePlayerController* PlayerController = Cast<ARougePlayerController>(It->Get()))
		{
			PlayerController->ClientAugmentRoundFinished();
		}
	}

	// Next level's cards right away if more than one level was gained; otherwise resume.
	StartAugmentRound();
}

void ARougeMainGameMode::SetAugmentPause(bool bPaused)
{
	if (UGameplayStatics::IsGamePaused(this) != bPaused)
	{
		UGameplayStatics::SetGamePaused(this, bPaused);
	}
}

UCharacterClassInfo* ARougeMainGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

UProjectileInfo* ARougeMainGameMode::GetProjectileInfo() const
{
	return ProjectileInfo;
}
