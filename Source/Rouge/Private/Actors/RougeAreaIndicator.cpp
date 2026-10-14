// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/RougeAreaIndicator.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARougeAreaIndicator::ARougeAreaIndicator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	CircleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CircleMesh"));
	SetRootComponent(CircleMesh);
	CircleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CircleMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		CircleMesh->SetStaticMesh(Cylinder.Object);
	}

	// Translucent unlit material with a "Color" parameter.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (ColorMaterial.Succeeded())
	{
		CircleMesh->SetMaterial(0, ColorMaterial.Object);
	}
}

void ARougeAreaIndicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARougeAreaIndicator, Radius);
	DOREPLIFETIME(ARougeAreaIndicator, Color);
	DOREPLIFETIME(ARougeAreaIndicator, Duration);
}

ARougeAreaIndicator* ARougeAreaIndicator::SpawnIndicator(UWorld* World, const FVector& Location, float InRadius, const FLinearColor& InColor, float InDuration)
{
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	// Snap to the floor under the location.
	FVector GroundLocation = Location;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AreaIndicatorGround), false);
	if (World->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 150.f), Location - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
	{
		GroundLocation = Hit.ImpactPoint;
	}
	GroundLocation.Z += 3.f;

	const FTransform SpawnTransform(FRotator::ZeroRotator, GroundLocation);
	ARougeAreaIndicator* Indicator = World->SpawnActorDeferred<ARougeAreaIndicator>(ARougeAreaIndicator::StaticClass(), SpawnTransform);
	if (Indicator)
	{
		Indicator->Radius = InRadius;
		Indicator->Color = InColor;
		Indicator->Duration = FMath::Max(InDuration, 0.05f);
		Indicator->SetLifeSpan(Indicator->Duration);
		Indicator->FinishSpawning(SpawnTransform);
	}
	return Indicator;
}

void ARougeAreaIndicator::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisuals();
}

void ARougeAreaIndicator::OnRep_Visuals()
{
	ApplyVisuals();
}

void ARougeAreaIndicator::ApplyVisuals()
{
	if (!CircleMesh)
	{
		return;
	}

	// Engine cylinder is 100 units wide.
	const float XY = FMath::Max(Radius * 2.f / 100.f, 0.01f);
	CircleMesh->SetRelativeScale3D(FVector(XY, XY, 0.01f));

	if (!CircleMID)
	{
		CircleMID = CircleMesh->CreateDynamicMaterialInstance(0);
	}
	if (CircleMID)
	{
		CircleMID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ARougeAreaIndicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fade out over the lifetime (cosmetic, every machine).
	Age += DeltaTime;
	if (CircleMID && Duration > 0.f)
	{
		FLinearColor Faded = Color;
		Faded.A = Color.A * FMath::Clamp(1.f - Age / Duration, 0.f, 1.f);
		CircleMID->SetVectorParameterValue(TEXT("Color"), Faded);
	}
}
