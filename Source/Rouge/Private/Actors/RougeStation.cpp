#include "Actors/RougeStation.h"

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
