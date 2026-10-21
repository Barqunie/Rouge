#include "Actors/RougeDeliveryZone.h"

#include "Actors/RougePackage.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/RougeMainGameMode.h"
#include "Game/RougeGameState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float DeliveryMeshSize = 100.f;
	constexpr float DeliveryPillarHeight = 600.f;
	constexpr float DeliveryCheckInterval = 0.2f;
	const FName DeliveryColorParam(TEXT("Color"));
}

ARougeDeliveryZone::ARougeDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = true;
	// Replicated so a zone spawned at runtime by the game mode shows up on clients too.
	bReplicates = true;
	bAlwaysRelevant = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CircleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CircleMesh"));
	CircleMesh->SetupAttachment(RootComponent);
	CircleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CircleMesh->SetCastShadow(false);

	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetupAttachment(RootComponent);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PillarMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		CircleMesh->SetStaticMesh(Cylinder.Object);
		PillarMesh->SetStaticMesh(Cylinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (ColorMaterial.Succeeded())
	{
		CircleMesh->SetMaterial(0, ColorMaterial.Object);
		PillarMesh->SetMaterial(0, ColorMaterial.Object);
	}

	ApplySize();
}

ARougeDeliveryZone* ARougeDeliveryZone::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ARougeDeliveryZone> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

bool ARougeDeliveryZone::IsOpen() const
{
	const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr;
	return !RougeGameState || RougeGameState->AreWavesCleared();
}

void ARougeDeliveryZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySize();
}

void ARougeDeliveryZone::ApplySize()
{
	const float CircleXY = Radius * 2.f / DeliveryMeshSize;
	CircleMesh->SetRelativeScale3D(FVector(CircleXY, CircleXY, 0.01f));
	CircleMesh->SetRelativeLocation(FVector(0.f, 0.f, 2.f));

	// Thin beam of light so the destination can be seen from far away.
	const float PillarXY = FMath::Clamp(Radius * 0.35f / DeliveryMeshSize, 0.3f, 3.f);
	PillarMesh->SetRelativeScale3D(FVector(PillarXY, PillarXY, DeliveryPillarHeight / DeliveryMeshSize));
	PillarMesh->SetRelativeLocation(FVector(0.f, 0.f, DeliveryPillarHeight * 0.5f));
}

void ARougeDeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	ApplySize();
	CircleMID = CircleMesh->CreateDynamicMaterialInstance(0);
	PillarMID = PillarMesh->CreateDynamicMaterialInstance(0);
}

void ARougeDeliveryZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// Cosmetic pulse, every machine. Red and dim while locked.
	const bool bOpen = IsOpen();
	const FLinearColor BaseColor = bOpen ? ZoneColor : LockedColor;
	if (CircleMID)
	{
		FLinearColor Color = BaseColor;
		Color.A = BaseColor.A * (0.8f + 0.2f * FMath::Sin(Age * 3.f));
		CircleMID->SetVectorParameterValue(DeliveryColorParam, Color);
	}
	if (PillarMID)
	{
		FLinearColor Color = BaseColor;
		Color.A = bOpen ? BaseColor.A * 0.5f : BaseColor.A * 0.2f;
		PillarMID->SetVectorParameterValue(DeliveryColorParam, Color);
	}

	if (!HasAuthority() || GetWorld()->GetTimeSeconds() < NextCheckTime)
	{
		return;
	}
	NextCheckTime = GetWorld()->GetTimeSeconds() + DeliveryCheckInterval;

	// Locked until the waves are done. A stolen package doesn't count; carried by a player or lying here does.
	const ARougePackage* Package = ARougePackage::Find(GetWorld());
	if (!bOpen || !Package || Package->IsCarriedByEnemy())
	{
		return;
	}

	if (FVector::DistSquared2D(Package->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius))
	{
		if (ARougeMainGameMode* GameMode = GetWorld()->GetAuthGameMode<ARougeMainGameMode>())
		{
			GameMode->HandlePackageDelivered();
		}
	}
}
