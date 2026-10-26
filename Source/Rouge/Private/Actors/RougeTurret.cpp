#include "Actors/RougeTurret.h"

#include "Character/EnemyBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Projectiles/ProjectileBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TurretShapeSize = 100.f;
	constexpr float TurretHeadHeight = 70.f;
	constexpr float TurretTurnSpeed = 12.f;
	const FName TurretColorParam(TEXT("Color"));
	const FName TurretOpacityParam(TEXT("Opacity"));
	const FName TurretIntensityParam(TEXT("Intensity"));
	const FLinearColor TurretMetal(0.32f, 0.22f, 0.12f, 1.f);
	const FLinearColor TurretVial(0.25f, 1.f, 0.08f, 1.f);
}

ARougeTurret::ARougeTurret()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto MakeMesh = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Mesh;
	};
	BaseMesh = MakeMesh(TEXT("BaseMesh"));
	HeadPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivot"));
	HeadPivot->SetupAttachment(RootComponent);
	HeadPivot->SetRelativeLocation(FVector(0.f, 0.f, TurretHeadHeight));
	HeadMesh = MakeMesh(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(HeadPivot);
	BarrelMesh = MakeMesh(TEXT("BarrelMesh"));
	VialMesh = MakeMesh(TEXT("VialMesh"));
	RangeRing = MakeMesh(TEXT("RangeRing"));
	RangeRing->SetCastShadow(false);
	VialMesh->SetCastShadow(false);

	BarrelMesh->SetupAttachment(HeadPivot);
	VialMesh->SetupAttachment(HeadPivot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterial(TEXT("/Game/ThirdPerson/Materials/FX/M_ChemGlow.M_ChemGlow"));

	if (Cylinder.Succeeded())
	{
		BaseMesh->SetStaticMesh(Cylinder.Object);
		BarrelMesh->SetStaticMesh(Cylinder.Object);
		RangeRing->SetStaticMesh(Cylinder.Object);
	}
	if (Cube.Succeeded())
	{
		HeadMesh->SetStaticMesh(Cube.Object);
	}
	if (Sphere.Succeeded())
	{
		VialMesh->SetStaticMesh(Sphere.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		BaseMesh->SetMaterial(0, ShapeMaterial.Object);
		HeadMesh->SetMaterial(0, ShapeMaterial.Object);
		BarrelMesh->SetMaterial(0, ShapeMaterial.Object);
	}
	if (GlowMaterial.Succeeded())
	{
		VialMesh->SetMaterial(0, GlowMaterial.Object);
		RangeRing->SetMaterial(0, GlowMaterial.Object);
	}

	// Squat brass base, boxy head, a barrel pointing forward (+X) and a glowing vial on top.
	BaseMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.5f));
	BaseMesh->SetRelativeLocation(FVector(0.f, 0.f, 25.f));
	HeadMesh->SetRelativeScale3D(FVector(0.45f, 0.35f, 0.3f));
	// Cylinder axis is Z; pitch it to point along +X.
	BarrelMesh->SetRelativeLocation(FVector(48.f, 0.f, 0.f));
	BarrelMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	BarrelMesh->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.65f));
	VialMesh->SetRelativeLocation(FVector(-8.f, 0.f, 30.f));
	VialMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.42f));
}

void ARougeTurret::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARougeTurret, Lifetime);
	DOREPLIFETIME(ARougeTurret, Range);
	DOREPLIFETIME(ARougeTurret, Target);
}

ARougeTurret* ARougeTurret::SpawnTurret(UWorld* World, AActor* InOwner, const FVector& Location, const FRotator& Facing,
	const FRougeTurretSettings& InSettings)
{
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	FVector GroundLocation = Location;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TurretGround), false, InOwner);
	if (World->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 100.f), Location - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
	{
		GroundLocation = Hit.ImpactPoint;
	}

	const FTransform SpawnTransform(FRotator(0.f, Facing.Yaw, 0.f), GroundLocation);
	ARougeTurret* Turret = World->SpawnActorDeferred<ARougeTurret>(ARougeTurret::StaticClass(), SpawnTransform, InOwner,
		Cast<APawn>(InOwner), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Turret)
	{
		Turret->Settings = InSettings;
		Turret->Lifetime = InSettings.Duration;
		Turret->Range = InSettings.Range;
		Turret->SetLifeSpan(InSettings.Duration);
		Turret->FinishSpawning(SpawnTransform);
	}
	return Turret;
}

void ARougeTurret::BeginPlay()
{
	Super::BeginPlay();

	MetalMID = UMaterialInstanceDynamic::Create(BaseMesh->GetMaterial(0), this);
	if (MetalMID)
	{
		MetalMID->SetVectorParameterValue(TurretColorParam, TurretMetal);
		BaseMesh->SetMaterial(0, MetalMID);
		HeadMesh->SetMaterial(0, MetalMID);
		BarrelMesh->SetMaterial(0, MetalMID);
	}
	VialMID = VialMesh->CreateDynamicMaterialInstance(0);
	if (VialMID)
	{
		VialMID->SetVectorParameterValue(TurretColorParam, TurretVial);
		VialMID->SetScalarParameterValue(TurretIntensityParam, 2.5f);
	}
	RingMID = RangeRing->CreateDynamicMaterialInstance(0);
	if (RingMID)
	{
		RingMID->SetVectorParameterValue(TurretColorParam, TurretVial);
		RingMID->SetScalarParameterValue(TurretIntensityParam, 0.6f);
		RingMID->SetScalarParameterValue(TEXT("RimAmount"), 0.f);
	}
	const float RingXY = Range * 2.f / TurretShapeSize;
	RangeRing->SetRelativeScale3D(FVector(RingXY, RingXY, 0.005f));
	RangeRing->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	HeadYaw = GetActorRotation().Yaw;

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(FireTimer, this, &ThisClass::Fire, FMath::Max(0.05f, Settings.FireInterval), true, 0.25f);
	}
}

AActor* ARougeTurret::FindTarget() const
{
	const FVector Location = GetActorLocation();
	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(Settings.Range);
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsDead())
		{
			const float DistSq = FVector::DistSquared2D(It->GetActorLocation(), Location);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
	}
	return Best;
}

void ARougeTurret::Fire()
{
	Target = FindTarget();
	if (!Target || !Settings.Projectile.ProjectileClass)
	{
		return;
	}

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}
	const float BaseYaw = Direction.Rotation().Yaw;
	const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, TurretHeadHeight) + Direction.GetSafeNormal() * 90.f;

	// Extra shots fan out a little around the target.
	const int32 Shots = FMath::Max(1, Settings.ShotsPerVolley);
	constexpr float FanStep = 8.f;
	for (int32 Index = 0; Index < Shots; ++Index)
	{
		const float Yaw = BaseYaw + (Index - (Shots - 1) * 0.5f) * FanStep;
		const FTransform SpawnTransform(FRotator(0.f, Yaw, 0.f), Muzzle);
		if (AProjectileBase* Projectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
			Settings.Projectile.ProjectileClass, SpawnTransform, GetOwner(), GetInstigator()))
		{
			Projectile->SetProjectileParams(Settings.Projectile);
			Projectile->DamageEffectInfo = Settings.DamageInfo;
			Projectile->FinishSpawning(SpawnTransform);
		}
	}

	MulticastFired();
}

void ARougeTurret::MulticastFired_Implementation()
{
	Recoil = 1.f;
}

void ARougeTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;
	if (HasAuthority())
	{
		Lifetime = FMath::Max(0.f, Lifetime - DeltaTime);
	}

	// Head turns toward the target (target replicates), recoils on each shot.
	if (IsValid(Target))
	{
		const float Wanted = (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		HeadYaw = FMath::FixedTurn(HeadYaw, Wanted, TurretTurnSpeed * 60.f * DeltaTime);
	}
	Recoil = FMath::Max(0.f, Recoil - DeltaTime * 8.f);
	if (HeadPivot)
	{
		const FRotator WorldHead(0.f, HeadYaw, 0.f);
		HeadPivot->SetWorldRotation(WorldHead);
		HeadPivot->SetRelativeLocation(FVector(0.f, 0.f, TurretHeadHeight) - GetActorRotation().UnrotateVector(WorldHead.Vector()) * Recoil * 10.f);
	}

	// Vial pulses, flashes on shots and blinks in the last second.
	if (VialMID)
	{
		float Glow = 0.75f + 0.25f * FMath::Sin(Age * 5.f) + Recoil * 0.8f;
		if (Lifetime < 1.f)
		{
			Glow *= FMath::Sin(Age * 30.f) > 0.f ? 1.f : 0.2f;
		}
		VialMID->SetScalarParameterValue(TurretOpacityParam, FMath::Clamp(Glow, 0.f, 1.f));
	}
	// Range ring: only a short flash when placed, so it doesn't paint the screen.
	if (RingMID && RangeRing && RangeRing->IsVisible())
	{
		constexpr float RingFadeTime = 1.2f;
		const float RingAlpha = 1.f - Age / RingFadeTime;
		RingMID->SetScalarParameterValue(TurretOpacityParam, 0.3f * FMath::Max(0.f, RingAlpha));
		if (RingAlpha <= 0.f)
		{
			RangeRing->SetVisibility(false);
		}
	}
}
