#include "Actors/RougeChemFX.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine basic shapes are 100 units across.
	constexpr float ChemShapeSize = 100.f;
	// Hits can come in fast; past this the extra bursts are skipped.
	constexpr int32 MaxAliveChemFX = 48;
	// Bubbles outlive the flash a little.
	constexpr float BubbleLifeScale = 1.6f;

	const FName ChemColorParam(TEXT("Color"));
	const FName ChemIntensityParam(TEXT("Intensity"));
	const FName ChemOpacityParam(TEXT("Opacity"));
	const FName ChemRimParam(TEXT("RimAmount"));

	UStaticMesh* SphereMesh = nullptr;
	UStaticMesh* CylinderMesh = nullptr;
	UMaterialInterface* GlowMaterial = nullptr;
}

int32 ARougeChemFX::AliveCount = 0;

ARougeChemFX::ARougeChemFX()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/ThirdPerson/Materials/FX/M_ChemGlow.M_ChemGlow"));
	SphereMesh = Sphere.Succeeded() ? Sphere.Object : nullptr;
	CylinderMesh = Cylinder.Succeeded() ? Cylinder.Object : nullptr;
	GlowMaterial = Glow.Succeeded() ? Glow.Object : nullptr;
}

ARougeChemFX* ARougeChemFX::SpawnBurst(UWorld* World, const FVector& Location, const FRougeChemFXSettings& Settings, bool bStrong, float RadiusOverride)
{
	// Cosmetic: nothing to draw on a dedicated server.
	if (!World || World->GetNetMode() == NM_DedicatedServer || AliveCount >= MaxAliveChemFX)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	ARougeChemFX* FX = World->SpawnActor<ARougeChemFX>(ARougeChemFX::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
	if (FX)
	{
		FX->Setup(Settings, bStrong, RadiusOverride);
	}
	return FX;
}

UStaticMeshComponent* ARougeChemFX::MakeMesh(UStaticMesh* Mesh, UMaterialInstanceDynamic*& OutMID, float RimAmount)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetStaticMesh(Mesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCastShadow(false);
	Component->SetupAttachment(RootComponent);
	Component->RegisterComponent();

	OutMID = nullptr;
	if (GlowMaterial)
	{
		OutMID = UMaterialInstanceDynamic::Create(GlowMaterial, this);
		OutMID->SetVectorParameterValue(ChemColorParam, Color);
		OutMID->SetScalarParameterValue(ChemIntensityParam, Intensity);
		OutMID->SetScalarParameterValue(ChemRimParam, RimAmount);
		Component->SetMaterial(0, OutMID);
		MIDs.Add(OutMID);
	}
	return Component;
}

void ARougeChemFX::SetGlow(UMaterialInstanceDynamic* MID, float Opacity) const
{
	if (MID)
	{
		MID->SetScalarParameterValue(ChemOpacityParam, FMath::Clamp(Opacity, 0.f, 1.f));
	}
}

void ARougeChemFX::Setup(const FRougeChemFXSettings& Settings, bool bStrong, float RadiusOverride)
{
	++AliveCount;

	Color = bStrong ? Settings.StrongColor : Settings.Color;
	Intensity = Settings.Intensity * (bStrong ? 1.4f : 1.f);
	Radius = (RadiusOverride > 0.f ? RadiusOverride : Settings.Radius) * (bStrong ? Settings.StrongScale : 1.f);
	Duration = Settings.Duration * (bStrong ? 1.25f : 1.f);
	BubbleRise = Settings.BubbleRise;

	if (Settings.bFlash && SphereMesh)
	{
		UMaterialInstanceDynamic* MID = nullptr;
		FlashMesh = MakeMesh(SphereMesh, MID, 1.f);
		FlashMID = MID;
	}

	if (Settings.bRing && CylinderMesh)
	{
		UMaterialInstanceDynamic* MID = nullptr;
		RingMesh = MakeMesh(CylinderMesh, MID, 0.f);
		RingMID = MID;

		// The burst can be at chest height; the ring goes on the floor under it.
		FHitResult Hit;
		const FVector Location = GetActorLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ChemFXFloor), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Location, Location - FVector(0.f, 0.f, 400.f), ECC_Visibility, Params))
		{
			RingZ = Hit.ImpactPoint.Z - Location.Z + 2.f;
		}
	}

	const int32 BubbleCount = bStrong ? Settings.BubbleCount * 2 : Settings.BubbleCount;
	for (int32 Index = 0; Index < BubbleCount && SphereMesh; ++Index)
	{
		FBubble& Bubble = Bubbles.AddDefaulted_GetRef();
		UMaterialInstanceDynamic* MID = nullptr;
		Bubble.Mesh = MakeMesh(SphereMesh, MID, 1.f);
		Bubble.MID = MID;

		const FVector2D Offset = FMath::RandPointInCircle(Radius * 0.8f);
		Bubble.Start = FVector(Offset.X, Offset.Y, FMath::FRandRange(-10.f, 20.f));
		Bubble.Drift = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f) * 30.f;
		Bubble.Delay = FMath::FRandRange(0.f, Duration * 0.35f);
		Bubble.Size = Settings.BubbleSize * FMath::FRandRange(0.6f, 1.4f) / ChemShapeSize;
		Bubble.Mesh->SetRelativeScale3D(FVector(0.001f));
	}

	if (Settings.LightIntensity > 0.f)
	{
		Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(FMath::Max(200.f, Radius * 3.f));
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		LightPeak = Settings.LightIntensity * (bStrong ? 1.5f : 1.f);
		Light->SetIntensity(LightPeak);
	}

	if (Settings.ExtraVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Settings.ExtraVFX, GetActorLocation(), FRotator::ZeroRotator,
			FVector(Settings.ExtraVFXScale * (bStrong ? Settings.StrongScale : 1.f)));
	}

	SetLifeSpan(Duration * BubbleLifeScale + 0.1f);
}

void ARougeChemFX::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	const float Alpha = FMath::Clamp(Age / Duration, 0.f, 1.f);
	const float EaseOut = 1.f - FMath::Square(1.f - Alpha);

	// Flash: pops to full size quickly, then fades.
	if (FlashMesh)
	{
		const float Scale = Radius * 2.f / ChemShapeSize * FMath::Lerp(0.35f, 1.f, FMath::Min(1.f, EaseOut * 1.6f));
		FlashMesh->SetRelativeScale3D(FVector(Scale, Scale, Scale * 0.75f));
		SetGlow(FlashMID, (1.f - Alpha) * (1.f - Alpha) * 0.9f);
	}

	// Ring: thin shockwave sliding out along the ground.
	if (RingMesh)
	{
		const float XY = Radius * 2.f / ChemShapeSize * FMath::Lerp(0.2f, 1.15f, EaseOut);
		RingMesh->SetRelativeScale3D(FVector(XY, XY, 0.01f));
		RingMesh->SetRelativeLocation(FVector(0.f, 0.f, RingZ));
		SetGlow(RingMID, (1.f - Alpha) * 0.7f);
	}

	// Bubbles: swell, rise with a wobble, pop.
	const float BubbleLife = Duration * BubbleLifeScale;
	for (FBubble& Bubble : Bubbles)
	{
		const float Local = FMath::Clamp((Age - Bubble.Delay) / FMath::Max(BubbleLife - Bubble.Delay, 0.05f), 0.f, 1.f);
		if (!Bubble.Mesh)
		{
			continue;
		}
		if (Local <= 0.f)
		{
			continue;
		}
		const float Wobble = FMath::Sin((Age + Bubble.Delay * 13.f) * 9.f) * 6.f;
		Bubble.Mesh->SetRelativeLocation(Bubble.Start + Bubble.Drift * Local + FVector(Wobble, -Wobble, BubbleRise * Local));
		const float Swell = Local < 0.8f ? FMath::Lerp(0.3f, 1.f, Local / 0.8f) : FMath::Lerp(1.f, 1.6f, (Local - 0.8f) / 0.2f);
		Bubble.Mesh->SetRelativeScale3D(FVector(Bubble.Size * Swell));
		SetGlow(Bubble.MID, Local < 0.8f ? 1.f : 1.f - (Local - 0.8f) / 0.2f);
	}

	if (Light)
	{
		Light->SetIntensity(LightPeak * FMath::Square(1.f - Alpha));
	}
}

void ARougeChemFX::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AliveCount = FMath::Max(0, AliveCount - 1);
	Super::EndPlay(EndPlayReason);
}
