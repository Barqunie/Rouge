// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/RougeMainGameMode.h"
#include "Engine/GameInstance.h"
#include "Game/RougeCharacterSelectionSubsystem.h"
#include "Actors/RougePackage.h"
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

void ARougeMainGameMode::HandlePackageDelivered()
{
	EndRun(true);
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
