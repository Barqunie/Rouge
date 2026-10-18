#include "Actors/RougeEffectZone.h"

#include "AbilitySystemComponent.h"
#include "Character/EnemyBase.h"
#include "Character/RougeCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine cylinder: 100 wide, 100 tall, pivot in the middle.
	constexpr float ZoneCylinderSize = 100.f;
	constexpr float ZoneFadeTime = 0.4f;
	const FName ZoneColorParam(TEXT("Color"));
}

ARougeEffectZone::ARougeEffectZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	DiscMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiscMesh"));
	DiscMesh->SetupAttachment(RootComponent);
	DiscMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DiscMesh->SetCastShadow(false);

	// Slightly bigger, brighter disc under the main one reads as a glowing rim.
	EdgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EdgeMesh"));
	EdgeMesh->SetupAttachment(RootComponent);
	EdgeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EdgeMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		DiscMesh->SetStaticMesh(Cylinder.Object);
		EdgeMesh->SetStaticMesh(Cylinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (ColorMaterial.Succeeded())
	{
		DiscMesh->SetMaterial(0, ColorMaterial.Object);
		EdgeMesh->SetMaterial(0, ColorMaterial.Object);
	}
}

void ARougeEffectZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARougeEffectZone, Radius);
	DOREPLIFETIME(ARougeEffectZone, Duration);
	DOREPLIFETIME(ARougeEffectZone, Color);
	DOREPLIFETIME(ARougeEffectZone, AmbientVFX);
	DOREPLIFETIME(ARougeEffectZone, VFXScale);
	DOREPLIFETIME(ARougeEffectZone, VFXInterval);
}

ARougeEffectZone* ARougeEffectZone::SpawnZone(UWorld* World, const FVector& Location, const FRougeZoneSettings& InSettings,
	const FLinearColor& InColor, UNiagaraSystem* InVFX, float InVFXScale, float InVFXInterval)
{
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	// Snap to the floor.
	FVector GroundLocation = Location;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EffectZoneGround), false);
	if (World->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 150.f), Location - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
	{
		GroundLocation = Hit.ImpactPoint;
	}
	GroundLocation.Z += 2.f;

	const FTransform SpawnTransform(FRotator::ZeroRotator, GroundLocation);
	ARougeEffectZone* Zone = World->SpawnActorDeferred<ARougeEffectZone>(ARougeEffectZone::StaticClass(), SpawnTransform);
	if (Zone)
	{
		Zone->Settings = InSettings;
		Zone->Radius = InSettings.Radius;
		Zone->Duration = FMath::Max(InSettings.Duration, 0.1f);
		Zone->Color = InColor;
		Zone->AmbientVFX = InVFX;
		Zone->VFXScale = InVFXScale;
		Zone->VFXInterval = FMath::Max(InVFXInterval, 0.05f);
		Zone->SetLifeSpan(Zone->Duration);
		Zone->FinishSpawning(SpawnTransform);
	}
	return Zone;
}

void ARougeEffectZone::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisuals();

	if (HasAuthority())
	{
		// First pulse right away so the buff / burn starts on cast.
		Pulse();
		GetWorldTimerManager().SetTimer(PulseTimer, this, &ThisClass::Pulse, FMath::Max(Settings.TickInterval, 0.1f), true);
	}
}

void ARougeEffectZone::OnRep_Visuals()
{
	ApplyVisuals();
}

void ARougeEffectZone::ApplyVisuals()
{
	if (!DiscMesh || !EdgeMesh)
	{
		return;
	}

	const float DiscXY = FMath::Max(Radius * 2.f / ZoneCylinderSize, 0.01f);
	DiscMesh->SetRelativeScale3D(FVector(DiscXY, DiscXY, 0.01f));
	const float EdgeXY = FMath::Max((Radius + 12.f) * 2.f / ZoneCylinderSize, 0.01f);
	EdgeMesh->SetRelativeScale3D(FVector(EdgeXY, EdgeXY, 0.005f));
	EdgeMesh->SetRelativeLocation(FVector(0.f, 0.f, -0.5f));

	if (!DiscMID)
	{
		DiscMID = DiscMesh->CreateDynamicMaterialInstance(0);
	}
	if (!EdgeMID)
	{
		EdgeMID = EdgeMesh->CreateDynamicMaterialInstance(0);
	}
}

void ARougeEffectZone::Pulse()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Center = GetActorLocation();
	const float RadiusSq = FMath::Square(Settings.Radius);

	// Buff lasts a bit longer than one tick, so it ends shortly after leaving the zone.
	if (Settings.AllySpeedMultiplier > 1.f)
	{
		for (TActorIterator<ARougeCharacter> It(World); It; ++It)
		{
			ARougeCharacter* Ally = *It;
			if (IsValid(Ally) && !Ally->IsDead() && FVector::DistSquared2D(Ally->GetActorLocation(), Center) <= RadiusSq)
			{
				Ally->ApplySpeedBuff(Settings.AllySpeedMultiplier, Settings.TickInterval + 0.35f);
			}
		}
	}

	if (Settings.DamageInfo.DamageEffect && Settings.DamageInfo.SourceASC && Settings.DamageInfo.BaseDamage > 0.f)
	{
		// Collect first: a burn tick can kill (destroy) enemies.
		TArray<AEnemyBase*> Targets;
		for (TActorIterator<AEnemyBase> It(World); It; ++It)
		{
			AEnemyBase* Enemy = *It;
			if (IsValid(Enemy) && !Enemy->IsDead() && FVector::DistSquared2D(Enemy->GetActorLocation(), Center) <= RadiusSq)
			{
				Targets.Add(Enemy);
			}
		}

		for (AEnemyBase* Enemy : Targets)
		{
			if (!IsValid(Enemy))
			{
				continue;
			}
			FDamageEffectInfo Info = Settings.DamageInfo;
			Info.TargetASC = Enemy->GetAbilitySystemComponent();
			if (Info.TargetASC)
			{
				URougeAbilitySystemLibrary::ApplyDamageEffect(Info);
			}
		}
	}
}

void ARougeEffectZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Cosmetic, every machine (a listen server draws it too).
	Age += DeltaTime;
	const float FadeIn = FMath::Clamp(Age / ZoneFadeTime, 0.f, 1.f);
	const float FadeOut = FMath::Clamp((Duration - Age) / ZoneFadeTime, 0.f, 1.f);
	const float PulseScale = 0.8f + 0.2f * FMath::Sin(Age * 6.f);
	const float Alpha = FadeIn * FadeOut;

	if (DiscMID)
	{
		FLinearColor DiscColor = Color;
		DiscColor.A = Color.A * Alpha * PulseScale;
		DiscMID->SetVectorParameterValue(ZoneColorParam, DiscColor);
	}
	if (EdgeMID)
	{
		FLinearColor EdgeColor = Color;
		EdgeColor.A = FMath::Min(1.f, Color.A * 2.2f) * Alpha;
		EdgeMID->SetVectorParameterValue(ZoneColorParam, EdgeColor);
	}

	if (AmbientVFX && GetNetMode() != NM_DedicatedServer && Age < Duration - ZoneFadeTime)
	{
		VFXCountdown -= DeltaTime;
		if (VFXCountdown <= 0.f)
		{
			VFXCountdown = VFXInterval;
			// Random point in the disc (sqrt keeps it evenly spread).
			const float Angle = FMath::FRandRange(0.f, 2.f * PI);
			const float Distance = Radius * 0.9f * FMath::Sqrt(FMath::FRand());
			const FVector Point = GetActorLocation() + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 5.f);
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AmbientVFX, Point, FRotator::ZeroRotator, FVector(VFXScale));
		}
	}
}
