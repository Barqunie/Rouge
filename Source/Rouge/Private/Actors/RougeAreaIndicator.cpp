// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/RougeAreaIndicator.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine cylinder is 100 units wide and 100 tall, pivot in the middle.
	constexpr float CylinderSize = 100.f;
	constexpr float BurstExpandTime = 0.15f;
	constexpr float PillarFlashTime = 0.25f;
	constexpr float PillarHeight = 250.f;
	const FName ColorParam(TEXT("Color"));
}

ARougeAreaIndicator::ARougeAreaIndicator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CircleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CircleMesh"));
	CircleMesh->SetupAttachment(RootComponent);
	CircleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CircleMesh->SetCastShadow(false);

	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetupAttachment(RootComponent);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PillarMesh->SetCastShadow(false);
	PillarMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		CircleMesh->SetStaticMesh(Cylinder.Object);
		PillarMesh->SetStaticMesh(Cylinder.Object);
	}

	// Translucent unlit material with a "Color" parameter.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (ColorMaterial.Succeeded())
	{
		CircleMesh->SetMaterial(0, ColorMaterial.Object);
		PillarMesh->SetMaterial(0, ColorMaterial.Object);
	}
}

void ARougeAreaIndicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARougeAreaIndicator, Radius);
	DOREPLIFETIME(ARougeAreaIndicator, Color);
	DOREPLIFETIME(ARougeAreaIndicator, Duration);
	DOREPLIFETIME(ARougeAreaIndicator, bBurstEffect);
	DOREPLIFETIME(ARougeAreaIndicator, BurstVFX);
	DOREPLIFETIME(ARougeAreaIndicator, VFXScale);
}

ARougeAreaIndicator* ARougeAreaIndicator::SpawnIndicator(UWorld* World, const FVector& Location, float InRadius, const FLinearColor& InColor,
	float InDuration, bool bBurst, UNiagaraSystem* InVFX, float InVFXScale)
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
		Indicator->bBurstEffect = bBurst;
		Indicator->BurstVFX = InVFX;
		Indicator->VFXScale = InVFXScale;
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

void ARougeAreaIndicator::SetCircleRadius(float CurrentRadius)
{
	const float XY = FMath::Max(CurrentRadius * 2.f / CylinderSize, 0.01f);
	CircleMesh->SetRelativeScale3D(FVector(XY, XY, 0.01f));
}

void ARougeAreaIndicator::ApplyVisuals()
{
	if (!CircleMesh || !PillarMesh)
	{
		return;
	}

	SetCircleRadius(bBurstEffect ? Radius * 0.3f : Radius);

	if (!CircleMID)
	{
		CircleMID = CircleMesh->CreateDynamicMaterialInstance(0);
	}
	if (CircleMID)
	{
		CircleMID->SetVectorParameterValue(ColorParam, Color);
	}

	// Pillar: a thin column of light in the middle (burst only).
	PillarMesh->SetVisibility(bBurstEffect);
	if (bBurstEffect)
	{
		const float PillarXY = FMath::Clamp(Radius * 0.6f / CylinderSize, 0.3f, 3.f);
		PillarMesh->SetRelativeScale3D(FVector(PillarXY, PillarXY, PillarHeight / CylinderSize));
		PillarMesh->SetRelativeLocation(FVector(0.f, 0.f, PillarHeight * 0.5f));

		if (!PillarMID)
		{
			PillarMID = PillarMesh->CreateDynamicMaterialInstance(0);
		}
		if (PillarMID)
		{
			PillarMID->SetVectorParameterValue(ColorParam, Color);
		}
	}

	TrySpawnVFX();
}

void ARougeAreaIndicator::TrySpawnVFX()
{
	if (bVFXSpawned || !BurstVFX || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	bVFXSpawned = true;
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BurstVFX, GetActorLocation(), FRotator::ZeroRotator, FVector(VFXScale));
}

void ARougeAreaIndicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Cosmetic, every machine.
	Age += DeltaTime;
	const float Life = FMath::Clamp(1.f - Age / FMath::Max(Duration, 0.01f), 0.f, 1.f);

	if (bBurstEffect && CircleMesh)
	{
		// Shockwave: expand quickly to full radius.
		const float Expand = FMath::Clamp(Age / BurstExpandTime, 0.f, 1.f);
		SetCircleRadius(FMath::Lerp(Radius * 0.3f, Radius, FMath::Sqrt(Expand)));
	}

	if (CircleMID)
	{
		FLinearColor Faded = Color;
		Faded.A = Color.A * Life;
		CircleMID->SetVectorParameterValue(ColorParam, Faded);
	}

	if (PillarMID && PillarMesh && PillarMesh->IsVisible())
	{
		const float PillarLife = FMath::Clamp(1.f - Age / PillarFlashTime, 0.f, 1.f);
		FLinearColor Faded = Color;
		Faded.A = FMath::Min(1.f, Color.A * 1.5f) * PillarLife;
		PillarMID->SetVectorParameterValue(ColorParam, Faded);
		if (PillarLife <= 0.f)
		{
			PillarMesh->SetVisibility(false);
		}
	}
}
