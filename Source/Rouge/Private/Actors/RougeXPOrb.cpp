// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/RougeXPOrb.h"

#include "Character/RougeCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/RougeGameState.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ARougeXPOrb::ARougeXPOrb()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(20.f);

	// Uncollected orbs eventually disappear.
	InitialLifeSpan = 60.f;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(45.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(PickupSphere);

	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(PickupSphere);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbMesh->SetCastShadow(false);
	OrbMesh->SetRelativeScale3D(FVector(0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		OrbMesh->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (ColorMaterial.Succeeded())
	{
		OrbMaterial = ColorMaterial.Object;
	}
}

void ARougeXPOrb::BeginPlay()
{
	Super::BeginPlay();

	if (OrbMesh && OrbMaterial)
	{
		OrbMesh->SetMaterial(0, OrbMaterial);
		if (UMaterialInstanceDynamic* MID = OrbMesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), OrbColor);
		}
	}

	if (HasAuthority())
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPickupOverlap);
	}
	else
	{
		// Clients only show the orb; the server moves and collects it.
		SetActorTickEnabled(false);
	}
}

ARougeCharacter* ARougeXPOrb::FindMagnetTarget() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();
	ARougeCharacter* Closest = nullptr;
	float ClosestDistSq = FMath::Square(MagnetRadius);

	for (const APlayerState* OtherPlayerState : GameState->PlayerArray)
	{
		ARougeCharacter* Player = OtherPlayerState ? OtherPlayerState->GetPawn<ARougeCharacter>() : nullptr;
		if (!Player || Player->IsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(Player->GetActorLocation(), MyLocation);
		if (DistSq <= ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Player;
		}
	}

	return Closest;
}

void ARougeXPOrb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bCollected)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	ARougeCharacter* Target = MagnetTarget.Get();

	// Once pulled, keep following that player; otherwise look for one a few times per second.
	if ((!Target || Target->IsDead()) && Now >= NextMagnetSearchTime)
	{
		Target = FindMagnetTarget();
		MagnetTarget = Target;
		NextMagnetSearchTime = Now + 0.2f;
		CurrentPullSpeed = MagnetSpeed;
	}

	if (!Target || Target->IsDead())
	{
		MagnetTarget = nullptr;
		return;
	}

	CurrentPullSpeed += MagnetAcceleration * DeltaTime;

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float Distance = ToTarget.Size();
	const float Step = CurrentPullSpeed * DeltaTime;

	if (Distance <= Step)
	{
		TryCollect(Target);
		return;
	}

	SetActorLocation(GetActorLocation() + ToTarget / Distance * Step);
}

void ARougeXPOrb::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryCollect(OtherActor);
}

void ARougeXPOrb::TryCollect(AActor* OtherActor)
{
	const ARougeCharacter* Player = Cast<ARougeCharacter>(OtherActor);
	if (bCollected || !HasAuthority() || !Player || Player->IsDead())
	{
		return;
	}

	bCollected = true;

	if (ARougeGameState* RougeGameState = GetWorld()->GetGameState<ARougeGameState>())
	{
		RougeGameState->AddTeamXP(XPValue);
	}

	Destroy();
}
