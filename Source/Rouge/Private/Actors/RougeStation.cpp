#include "Actors/RougeStation.h"

#include "Actors/RougeChemFX.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float StationMeshSize = 100.f;
	constexpr float StationPedestalHeight = 40.f;
	constexpr float StationBeaconHeight = 900.f;
	const FName StationColorParam(TEXT("Color"));
	const FLinearColor StationPedestalColor(0.25f, 0.2f, 0.15f, 1.f);
}

ARougeStation::ARougeStation()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto MakeMesh = [this](const TCHAR* Name, bool bCollision)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(bCollision);
		return Mesh;
	};
	PedestalMesh = MakeMesh(TEXT("PedestalMesh"), false);
	CircleMesh = MakeMesh(TEXT("CircleMesh"), false);
	ProgressMesh = MakeMesh(TEXT("ProgressMesh"), false);
	BeaconMesh = MakeMesh(TEXT("BeaconMesh"), false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CircleMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (Cylinder.Succeeded())
	{
		PedestalMesh->SetStaticMesh(Cylinder.Object);
		CircleMesh->SetStaticMesh(Cylinder.Object);
		ProgressMesh->SetStaticMesh(Cylinder.Object);
		BeaconMesh->SetStaticMesh(Cylinder.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		PedestalMesh->SetMaterial(0, ShapeMaterial.Object);
	}
	if (CircleMaterial.Succeeded())
	{
		CircleMesh->SetMaterial(0, CircleMaterial.Object);
		ProgressMesh->SetMaterial(0, CircleMaterial.Object);
		BeaconMesh->SetMaterial(0, CircleMaterial.Object);
	}

	ApplySize();
}

void ARougeStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARougeStation, bIsFinal);
	DOREPLIFETIME(ARougeStation, State);
	DOREPLIFETIME(ARougeStation, Progress);
	DOREPLIFETIME(ARougeStation, StationNumber);
}

TArray<ARougeStation*> ARougeStation::FindAll(const UWorld* World)
{
	TArray<ARougeStation*> Stations;
	if (World)
	{
		for (TActorIterator<ARougeStation> It(const_cast<UWorld*>(World)); It; ++It)
		{
			if (IsValid(*It))
			{
				Stations.Add(*It);
			}
		}
	}
	return Stations;
}

FVector ARougeStation::GetDockLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, StationPedestalHeight + 2.f);
}

void ARougeStation::SetState(ERougeStationState NewState)
{
	if (HasAuthority() && State != NewState)
	{
		State = NewState;
		ForceNetUpdate();
	}
}

void ARougeStation::SetProgress(float NewProgress)
{
	if (HasAuthority())
	{
		Progress = FMath::Clamp(NewProgress, 0.f, 1.f);
	}
}

void ARougeStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySize();
}

void ARougeStation::ApplySize()
{
	// Pedestal: a low round platform the package sits on.
	const float PedestalXY = 140.f / StationMeshSize;
	PedestalMesh->SetRelativeScale3D(FVector(PedestalXY, PedestalXY, StationPedestalHeight / StationMeshSize));
	PedestalMesh->SetRelativeLocation(FVector(0.f, 0.f, StationPedestalHeight * 0.5f));

	const float CircleXY = Radius * 2.f / StationMeshSize;
	CircleMesh->SetRelativeScale3D(FVector(CircleXY, CircleXY, 0.01f));
	CircleMesh->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	ProgressMesh->SetRelativeLocation(FVector(0.f, 0.f, 3.f));
	ProgressMesh->SetRelativeScale3D(FVector(0.01f, 0.01f, 0.01f));

	const float BeaconXY = 60.f / StationMeshSize;
	BeaconMesh->SetRelativeScale3D(FVector(BeaconXY, BeaconXY, StationBeaconHeight / StationMeshSize));
	BeaconMesh->SetRelativeLocation(FVector(0.f, 0.f, StationBeaconHeight * 0.5f));
}

void ARougeStation::BeginPlay()
{
	Super::BeginPlay();
	ApplySize();

	PedestalMID = PedestalMesh->CreateDynamicMaterialInstance(0);
	if (PedestalMID)
	{
		PedestalMID->SetVectorParameterValue(StationColorParam, StationPedestalColor);
	}
	CircleMID = CircleMesh->CreateDynamicMaterialInstance(0);
	ProgressMID = ProgressMesh->CreateDynamicMaterialInstance(0);
	BeaconMID = BeaconMesh->CreateDynamicMaterialInstance(0);
}

void ARougeStation::TickPurifyFX(float DeltaTime)
{
	if (State != ERougeStationState::Purifying || GetNetMode() == NM_DedicatedServer)
	{
		LastFxProgress = Progress;
		return;
	}

	const FVector Top = GetActorLocation() + FVector(0.f, 0.f, StationPedestalHeight + 10.f);

	// A shock ring every quarter of the bar.
	if (FMath::FloorToInt(Progress * 4.f) > FMath::FloorToInt(LastFxProgress * 4.f))
	{
		FRougeChemFXSettings Ring;
		Ring.Color = PurifyColor;
		Ring.Intensity = 3.f;
		Ring.bFlash = false;
		Ring.bRing = true;
		Ring.BubbleCount = 10;
		Ring.BubbleSize = 9.f;
		Ring.BubbleRise = 220.f;
		Ring.Duration = 0.7f;
		ARougeChemFX::SpawnBurst(GetWorld(), Top, Ring, false, Radius);
	}
	LastFxProgress = Progress;

	// Steam puffs and sparks, thicker as the bar fills.
	SteamTimer -= DeltaTime;
	if (SteamTimer > 0.f)
	{
		return;
	}
	SteamTimer = FMath::Lerp(0.55f, 0.16f, Progress) * FMath::FRandRange(0.8f, 1.2f);

	FRougeChemFXSettings Steam;
	Steam.Color = FLinearColor(0.45f, 1.f, 0.55f, 1.f);
	Steam.Intensity = 1.2f + Progress;
	Steam.bFlash = false;
	Steam.BubbleCount = 4 + FMath::RoundToInt(Progress * 4.f);
	Steam.BubbleSize = 12.f + Progress * 10.f;
	Steam.BubbleRise = 240.f + Progress * 120.f;
	Steam.Duration = 1.3f;
	const FVector Jitter(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 0.f);
	ARougeChemFX::SpawnBurst(GetWorld(), Top + Jitter, Steam, false, 45.f);

	if (FMath::FRand() < 0.25f + Progress * 0.5f)
	{
		FRougeChemFXSettings Sparks;
		Sparks.Color = FLinearColor(1.f, 0.85f, 0.25f, 1.f);
		Sparks.Intensity = 6.f;
		Sparks.bFlash = false;
		Sparks.BubbleCount = 7;
		Sparks.BubbleSize = 2.5f;
		Sparks.BubbleRise = 190.f;
		Sparks.Duration = 0.35f;
		ARougeChemFX::SpawnBurst(GetWorld(), Top + FVector(0.f, 0.f, 20.f), Sparks, false, 70.f);
	}
}

FLinearColor ARougeStation::GetStateColor() const
{
	switch (State)
	{
	case ERougeStationState::Active:
		return ActiveColor;
	case ERougeStationState::Purifying:
		return PurifyColor;
	case ERougeStationState::Purified:
		return PurifiedColor;
	default:
		return IdleColor;
	}
}

void ARougeStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// Cosmetic, every machine.
	TickPurifyFX(DeltaTime);
	const FLinearColor Base = GetStateColor();
	const bool bLive = State == ERougeStationState::Active || State == ERougeStationState::Purifying;
	const float Pulse = bLive ? 0.75f + 0.25f * FMath::Sin(Age * 4.f) : 1.f;

	if (CircleMID)
	{
		FLinearColor Color = Base;
		Color.A = Base.A * Pulse;
		CircleMID->SetVectorParameterValue(StationColorParam, Color);
	}

	if (ProgressMesh)
	{
		const bool bShowProgress = State == ERougeStationState::Purifying && Progress > 0.f;
		ProgressMesh->SetVisibility(bShowProgress);
		if (bShowProgress)
		{
			const float XY = FMath::Max(Radius * 2.f * Progress / StationMeshSize, 0.01f);
			ProgressMesh->SetRelativeScale3D(FVector(XY, XY, 0.01f));
			if (ProgressMID)
			{
				FLinearColor Color = PurifyColor;
				Color.A = FMath::Min(1.f, PurifyColor.A * 1.6f);
				ProgressMID->SetVectorParameterValue(StationColorParam, Color);
			}
		}
	}

	// Beacon only on the station the team has to go to.
	if (BeaconMesh)
	{
		BeaconMesh->SetVisibility(bLive);
		if (bLive && BeaconMID)
		{
			FLinearColor Color = Base;
			Color.A = Base.A * 0.6f * Pulse;
			BeaconMID->SetVectorParameterValue(StationColorParam, Color);
		}
	}
}
