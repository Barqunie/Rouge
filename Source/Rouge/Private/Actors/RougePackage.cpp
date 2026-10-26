#include "Actors/RougePackage.h"

#include "Character/EnemyBase.h"
#include "Character/RougeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float PackageMeshSize = 100.f;
	constexpr float PackageBoxSize = 45.f;
	constexpr float PickupCheckInterval = 0.1f;
	const FName PackageColorParam(TEXT("Color"));
}

ARougePackage::ARougePackage()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	// Movement replication also carries the attachment to the carrier.
	SetReplicateMovement(true);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(RootComponent);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetRelativeScale3D(FVector(PackageBoxSize / PackageMeshSize));

	GroundRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundRing"));
	GroundRing->SetupAttachment(RootComponent);
	GroundRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundRing->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BoxMesh->SetStaticMesh(Cube.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMaterial.Succeeded())
	{
		BoxMesh->SetMaterial(0, ShapeMaterial.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		GroundRing->SetStaticMesh(Cylinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RingMaterial(TEXT("/Game/ThirdPerson/Materials/M_ReviveCircle.M_ReviveCircle"));
	if (RingMaterial.Succeeded())
	{
		GroundRing->SetMaterial(0, RingMaterial.Object);
	}
}

void ARougePackage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARougePackage, Carrier);
	DOREPLIFETIME(ARougePackage, DockedStation);
}

ARougePackage* ARougePackage::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ARougePackage> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

bool ARougePackage::IsCarriedByPlayer() const
{
	return Cast<ARougeCharacter>(Carrier) != nullptr;
}

bool ARougePackage::IsCarriedByEnemy() const
{
	return Cast<AEnemyBase>(Carrier) != nullptr;
}

void ARougePackage::BeginPlay()
{
	Super::BeginPlay();

	BoxMID = BoxMesh->CreateDynamicMaterialInstance(0);
	if (BoxMID)
	{
		BoxMID->SetVectorParameterValue(PackageColorParam, PackageColor);
	}
	RingMID = GroundRing->CreateDynamicMaterialInstance(0);

	const float RingXY = PickupRadius * 2.f / PackageMeshSize;
	GroundRing->SetRelativeScale3D(FVector(RingXY, RingXY, 0.01f));

	if (HasAuthority())
	{
		PlaceOnGround(GetActorLocation());
	}
	UpdateVisuals();
}

// ---------------------------------------------------------------------------
// Carrying (server)
// ---------------------------------------------------------------------------

bool ARougePackage::CanBeCarriedBy(const ACharacter* Character) const
{
	if (!IsValid(Character))
	{
		return false;
	}

	if (const ACharacterBase* CharacterBase = Cast<ACharacterBase>(Character))
	{
		if (CharacterBase->IsDead())
		{
			return false;
		}
	}

	if (Character->IsA<AEnemyBase>() && !bEnemiesCanCarry)
	{
		return false;
	}

	// Docked on a station: being purified, players leave it there (enemies can still steal it).
	if (DockedStation && Character->IsA<ARougeCharacter>())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	return !(LastDropper.Get() == Character && World && World->GetTimeSeconds() < RepickAllowedTime);
}

bool ARougePackage::TryPickUp(ACharacter* NewCarrier)
{
	if (!HasAuthority() || Carrier || !CanBeCarriedBy(NewCarrier))
	{
		return false;
	}

	DockedStation = nullptr;
	SetCarrier(NewCarrier);
	return true;
}

void ARougePackage::DockAt(AActor* Station, const FVector& DockLocation)
{
	if (!HasAuthority() || !Station)
	{
		return;
	}
	if (Carrier)
	{
		SetCarrier(nullptr);
	}
	DockedStation = Station;
	SetActorLocationAndRotation(DockLocation, FRotator::ZeroRotator);
	ForceNetUpdate();
}

void ARougePackage::Undock()
{
	if (HasAuthority() && DockedStation)
	{
		DockedStation = nullptr;
		LastDropper = nullptr;
		ForceNetUpdate();
	}
}

void ARougePackage::Drop()
{
	if (!HasAuthority() || !Carrier)
	{
		return;
	}

	LastDropper = Carrier;
	RepickAllowedTime = GetWorld()->GetTimeSeconds() + RepickDelay;
	const FVector DropLocation = IsValid(Carrier) ? Carrier->GetActorLocation() : LastCarrierLocation;

	SetCarrier(nullptr);
	PlaceOnGround(DropLocation);
}

void ARougePackage::SetCarrier(ACharacter* NewCarrier)
{
	ACharacter* OldCarrier = Carrier;

	if (ARougeCharacter* OldPlayer = Cast<ARougeCharacter>(OldCarrier))
	{
		OldPlayer->SetCarrySpeedMultiplier(1.f);
	}

	Carrier = NewCarrier;

	if (Carrier)
	{
		LastCarrierLocation = Carrier->GetActorLocation();

		// On the back: a socket on the mesh if there is one (follows the animation), else behind the capsule.
		// Always the animated character mesh: its sockets are made for its skeleton, and a retargeted look
		// (ACharacterBase::VisualMesh) follows that same pose.
		const ACharacterBase* CarrierCharacter = Cast<ACharacterBase>(Carrier);
		USkeletalMeshComponent* CarrierMesh = Carrier->GetMesh();
		if (!CarrySocketName.IsNone() && CarrierMesh && CarrierMesh->DoesSocketExist(CarrySocketName))
		{
			AttachToComponent(CarrierMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CarrySocketName);
			SetActorRelativeLocation(CarrySocketOffset);
			SetActorRelativeRotation(CarrySocketRotation);
			if (CarrierCharacter && !CarrierCharacter->GetCarriedPackageOffset().IsNearlyZero())
			{
				AddActorWorldOffset(Carrier->GetActorRotation().RotateVector(CarrierCharacter->GetCarriedPackageOffset()));
			}
		}
		else
		{
			AttachToActor(Carrier, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SetActorRelativeLocation(CarryOffset);
			SetActorRelativeRotation(FRotator::ZeroRotator);
		}

		if (ARougeCharacter* NewPlayer = Cast<ARougeCharacter>(Carrier))
		{
			NewPlayer->SetCarrySpeedMultiplier(CarrierSpeedMultiplier);
		}
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	ForceNetUpdate();
	UpdateVisuals();
	OnCarrierChanged.Broadcast(Carrier);
}

void ARougePackage::PlaceOnGround(const FVector& Near)
{
	FVector Location = Near;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PackageGround), false, this);
	if (IsValid(Carrier))
	{
		Params.AddIgnoredActor(Carrier);
	}
	if (GetWorld()->LineTraceSingleByChannel(Hit, Near + FVector(0.f, 0.f, 100.f), Near - FVector(0.f, 0.f, 2000.f), ECC_Visibility, Params))
	{
		Location = Hit.ImpactPoint;
	}

	// Root on the floor; the box floats above it (see UpdateVisuals / Tick).
	SetActorLocationAndRotation(Location + FVector(0.f, 0.f, 2.f), FRotator::ZeroRotator);
}

void ARougePackage::CheckForPickup()
{
	const FVector Location = GetActorLocation();
	const float RadiusSq = FMath::Square(PickupRadius);
	constexpr float MaxHeightDifference = 250.f;

	// Players first, the closest one wins.
	ACharacter* Best = nullptr;
	float BestDistSq = RadiusSq;
	for (TActorIterator<ARougeCharacter> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared2D(It->GetActorLocation(), Location);
		if (DistSq <= BestDistSq && FMath::Abs(It->GetActorLocation().Z - Location.Z) < MaxHeightDifference && CanBeCarriedBy(*It))
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}

	if (!Best && bEnemiesCanCarry)
	{
		BestDistSq = RadiusSq;
		for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
		{
			const float DistSq = FVector::DistSquared2D(It->GetActorLocation(), Location);
			if (DistSq <= BestDistSq && FMath::Abs(It->GetActorLocation().Z - Location.Z) < MaxHeightDifference && CanBeCarriedBy(*It))
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
	}

	if (Best)
	{
		TryPickUp(Best);
	}
}

void ARougePackage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	if (HasAuthority())
	{
		if (Carrier)
		{
			// Carrier died or was destroyed (killed enemies are destroyed right away): drop where it was.
			const ACharacterBase* CarrierBase = Cast<ACharacterBase>(Carrier);
			if (!IsValid(Carrier) || (CarrierBase && CarrierBase->IsDead()))
			{
				Drop();
			}
			else
			{
				LastCarrierLocation = Carrier->GetActorLocation();
			}
		}
		else if (GetWorld()->GetTimeSeconds() >= NextPickupCheckTime)
		{
			NextPickupCheckTime = GetWorld()->GetTimeSeconds() + PickupCheckInterval;
			CheckForPickup();
		}
	}

	// Cosmetic: bob and spin on the ground, still on the carrier's back.
	if (BoxMesh)
	{
		const bool bGround = Carrier == nullptr;
		BoxMesh->SetRelativeLocation(FVector(0.f, 0.f, bGround ? 45.f + 8.f * FMath::Sin(Age * 3.f) : 0.f));
		BoxMesh->SetRelativeRotation(bGround ? FRotator(0.f, Age * 90.f, 0.f) : FRotator::ZeroRotator);
	}
	if (RingMID && GroundRing && GroundRing->IsVisible())
	{
		FLinearColor RingColor = PackageColor;
		RingColor.A = 0.25f + 0.12f * FMath::Sin(Age * 4.f);
		RingMID->SetVectorParameterValue(PackageColorParam, RingColor);
	}
}

void ARougePackage::OnRep_Carrier()
{
	UpdateVisuals();
	OnCarrierChanged.Broadcast(Carrier);
}

void ARougePackage::UpdateVisuals()
{
	if (GroundRing)
	{
		GroundRing->SetVisibility(Carrier == nullptr);
	}
}
