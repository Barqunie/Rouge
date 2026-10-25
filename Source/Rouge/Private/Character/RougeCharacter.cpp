// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/RougeCharacter.h"
#include "Actors/RougePackage.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Rouge.h"
#include "Net/UnrealNetwork.h"	
#include "GameFramework/PlayerState.h"
#include "Data/CharacterClassInfo.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include <Game/PlayerState/RougePlayerState.h>
#include "AbilitySystem/RougeAbilitySystemComponent.h"

#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "AbilitySystem/RougeGameplayTag.h"
#include "Game/RougeGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/RougeHUDWidget.h"



ARougeCharacter::ARougeCharacter()
{


	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	//ReplicatedBasedMovement.bRelativeRotation = true;

	GetCharacterMovement()->bOrientRotationToMovement =  false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);


	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;


	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1300.0f;
	CameraBoom->SetRelativeRotation(FRotator(-45.f,0.f , 0.f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->SetUsingAbsoluteRotation(true);   
	CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;



	DynamicProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DynamicProjectileSpawnPoint"));
	DynamicProjectileSpawnPoint->SetupAttachment(RootComponent);

	// Revive circles on the ground (flattened engine cylinders), shown only while dead.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CircleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	HUDWidgetClass = URougeHUDWidget::StaticClass();

	static ConstructorHelpers::FObjectFinder<UInputAction> ToggleAimAsset(TEXT("/Game/Input/Actions/IA_ToggleAimMode.IA_ToggleAimMode"));
	if (ToggleAimAsset.Succeeded())
	{
		ToggleAimModeAction = ToggleAimAsset.Object;
	}

	ReviveAreaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReviveAreaMesh"));
	ReviveAreaMesh->SetupAttachment(RootComponent);
	ReviveAreaMesh->SetRelativeLocation(FVector(0.f, 0.f, -95.f));
	ReviveAreaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReviveAreaMesh->SetCastShadow(false);
	ReviveAreaMesh->SetVisibility(false);

	ReviveProgressMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReviveProgressMesh"));
	ReviveProgressMesh->SetupAttachment(RootComponent);
	ReviveProgressMesh->SetRelativeLocation(FVector(0.f, 0.f, -94.f));
	ReviveProgressMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReviveProgressMesh->SetCastShadow(false);
	ReviveProgressMesh->SetVisibility(false);

	if (CircleMesh.Succeeded())
	{
		ReviveAreaMesh->SetStaticMesh(CircleMesh.Object);
		ReviveProgressMesh->SetStaticMesh(CircleMesh.Object);
	}

	// Bubble shown while the Shield attribute is above 0.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BubbleMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	ShieldBubbleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldBubbleMesh"));
	ShieldBubbleMesh->SetupAttachment(RootComponent);
	ShieldBubbleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldBubbleMesh->SetCastShadow(false);
	ShieldBubbleMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 2.2f));
	ShieldBubbleMesh->SetVisibility(false);
	if (BubbleMesh.Succeeded())
	{
		ShieldBubbleMesh->SetStaticMesh(BubbleMesh.Object);
	}





}

void ARougeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARougeCharacter::Move);

		if (ToggleAimModeAction)
		{
			EnhancedInputComponent->BindAction(ToggleAimModeAction, ETriggerEvent::Started, this, &ARougeCharacter::ToggleAimMode);
		}

		// Tab: stats panel. Action and mapping are made at runtime so no input assets are needed.
		if (!ToggleStatsAction)
		{
			ToggleStatsAction = NewObject<UInputAction>(this, TEXT("IA_ToggleStatsPanel"));
			ToggleStatsAction->ValueType = EInputActionValueType::Boolean;

			StatsMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_StatsPanel"));
			StatsMappingContext->MapKey(ToggleStatsAction, EKeys::Tab);

			// F: drop the package (same runtime mapping context).
			DropPackageAction = NewObject<UInputAction>(this, TEXT("IA_DropPackage"));
			DropPackageAction->ValueType = EInputActionValueType::Boolean;
			StatsMappingContext->MapKey(DropPackageAction, EKeys::F);
		}

		if (const APlayerController* RougePlayerController = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(RougePlayerController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(StatsMappingContext, 1);
			}
		}

		EnhancedInputComponent->BindAction(ToggleStatsAction, ETriggerEvent::Started, this, &ARougeCharacter::ToggleStatsPanel);
		EnhancedInputComponent->BindAction(DropPackageAction, ETriggerEvent::Started, this, &ARougeCharacter::DropPackagePressed);

		// Looking

	}
	else
	{
		UE_LOG(LogRouge, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ARougeCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitReviveCircleMaterials();
	ApplyGameInputMode();
	CreateHUDWidget();
}

void ARougeCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	if (StatsMappingContext)
	{
		const APlayerController* RougePlayerController = Cast<APlayerController>(GetController());
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = RougePlayerController
			? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(RougePlayerController->GetLocalPlayer()) : nullptr)
		{
			Subsystem->RemoveMappingContext(StatsMappingContext);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ARougeCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// The controller is usually not set yet in BeginPlay, so apply the input mode on possession too.
	ApplyGameInputMode();
	CreateHUDWidget();
}

void ARougeCharacter::CreateHUDWidget()
{
	APlayerController* RougePlayerController = Cast<APlayerController>(GetController());
	if (HUDWidget || !HUDWidgetClass || !RougePlayerController || !RougePlayerController->IsLocalController())
	{
		return;
	}

	HUDWidget = CreateWidget<URougeHUDWidget>(RougePlayerController, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void ARougeCharacter::ToggleStatsPanel()
{
	if (HUDWidget)
	{
		HUDWidget->ToggleStatsPanel();
	}
}

void ARougeCharacter::ApplyGameInputMode()
{
	APlayerController* RougePlayerController = Cast<APlayerController>(GetController());
	if (!RougePlayerController || !RougePlayerController->IsLocalController())
	{
		return;
	}

	RougePlayerController->SetShowMouseCursor(true);

	// With a visible cursor the viewport releases mouse capture on every button up, so each click
	// re-captures it. Without this the capturing click (left click aim toggle) never reaches input.
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	RougePlayerController->SetInputMode(InputMode);
}

void ARougeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSpeedBuff();
	UpdateCameraFeel(DeltaTime);

	// Sürekli mouse'a bak - input'a baðlý deðil
	// Aim is driven only by the owning player (server-side remote pawns and simulated proxies skip this).
	if (bIsDead)
	{
		if (HasAuthority())
		{
			TickDeathServer(DeltaTime);
		}
		if (bIsDead)
		{
			UpdateDeathPresentation();
		}
		return;
	}

	if (!IsLocallyControlled()) return;

	APlayerController* RougePlayerController = Cast<APlayerController>(GetController());
	if (!RougePlayerController) return;

	// Auto aim faces the nearest enemy in range; otherwise (or with no enemy around) face the mouse cursor.
	FVector AimPoint = FVector::ZeroVector;
	bool bHasAimPoint = bAutoAim && GetAutoAimPoint(AimPoint);

	FHitResult Hit;
	if (!bHasAimPoint && RougePlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		AimPoint = Hit.ImpactPoint;
		bHasAimPoint = true;
	}

	if (bHasAimPoint)
	{
		const FVector ActorLocation = GetActorLocation();
		FVector TargetLocation = AimPoint;
		TargetLocation.Z = ActorLocation.Z;

		FVector ToTarget = TargetLocation - ActorLocation;
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator NewRotation = ToTarget.Rotation();
			SetActorRotation(NewRotation);

			// Listen-server host already has authority; only remote owning clients need the RPC.
			if (!HasAuthority())
			{
				SendLookRotationToServer(NewRotation);
			}
		}
	}
}
void ARougeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}



void ARougeCharacter::SendLookRotationToServer(const FRotator& NewRotation)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(NewRotation.Yaw - LastSentLookYaw));
	const bool bResendDue = LastLookSendTime < 0.f || (Now - LastLookSendTime) >= LookResendInterval;

	if (YawDelta >= LookYawSendThreshold || bResendDue)
	{
		ServerSetLookRotation(NewRotation);
		LastSentLookYaw = NewRotation.Yaw;
		LastLookSendTime = Now;
	}
}

void ARougeCharacter::ServerSetLookRotation_Implementation(const FRotator& NewRotation)
{
	// Top-down aim is yaw only.
	SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
}

void ARougeCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr && !bIsDead)
	{
		const FVector ForwardDirection = FVector::ForwardVector;
		const FVector RightDirection = FVector::RightVector;

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ARougeCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{

		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}




void ARougeCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if(HasAuthority())
	{
		InitAbilityActorInfo();

		// Replicate which character this player is playing (UI, character-specific augments).
		if (ARougePlayerState* RougePlayerState = GetPlayerState<ARougePlayerState>())
		{
			RougePlayerState->SetSelectedCharacterTag(CharacterTag);
		}
	}

	BroadcastPlayerName();
}

void ARougeCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
	BroadcastPlayerName();
}

FString ARougeCharacter::GetPlayerDisplayName() const
{
	const APlayerState* CurrentPlayerState = GetPlayerState();
	return CurrentPlayerState ? CurrentPlayerState->GetPlayerName() : FString();
}

void ARougeCharacter::BroadcastPlayerName()
{
	const FString PlayerName = GetPlayerDisplayName();
	if (!PlayerName.IsEmpty())
	{
		OnPlayerNameChanged(PlayerName);
	}
}

UAbilitySystemComponent* ARougeCharacter::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

USceneComponent* ARougeCharacter::GetDynamicSpawnPoint_Implementation()
{
	return DynamicProjectileSpawnPoint;
}

void ARougeCharacter::InitAbilityActorInfo()
{
	if(ARougePlayerState* RougePlayerState = GetPlayerState<ARougePlayerState>())
	{
		RougeAbilitySystemComp = RougePlayerState->GetRougeAbilitySystemComponent();
		RougeAttributes = RougePlayerState->GetRougeAttributeSet();


		if(IsValid(RougeAbilitySystemComp))
		{
			RougeAbilitySystemComp->InitAbilityActorInfo(RougePlayerState, this);
			BindCallbacksToDependencies();  BroadcastInitialValues();

			// ASC can arrive after OnRep_IsDead on clients: keep State.Dead in sync.
			RougeAbilitySystemComp->SetLooseGameplayTagCount(RougeGameplayTags::State::Dead, bIsDead ? 1 : 0);

			if(HasAuthority())
			{
				InitClassDefaults();
			}
		}
	}
}

void ARougeCharacter::ApplySpeedBuff(float Multiplier, float Duration)
{
	if (!HasAuthority() || Multiplier <= 1.f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	SpeedBuffEndTime = FMath::Max(SpeedBuffEndTime, Now + Duration);
	if (Multiplier > SpeedBuffMultiplier)
	{
		SpeedBuffMultiplier = Multiplier;
		ApplyWalkSpeed();
	}
}

void ARougeCharacter::AddCameraTrauma(float Amount)
{
	if (IsLocallyControlled())
	{
		CameraTrauma = FMath::Clamp(CameraTrauma + Amount, 0.f, 1.f);
	}
}

void ARougeCharacter::PlayDamageFlash(float Strength)
{
	if (IsLocallyControlled())
	{
		DamageFlash = FMath::Clamp(FMath::Max(DamageFlash, Strength), 0.f, 1.f);
	}
}

void ARougeCharacter::UpdateCameraFeel(float DeltaTime)
{
	if (!FollowCamera || !IsLocallyControlled())
	{
		return;
	}
	if (!bCameraBaseCaptured)
	{
		CameraBaseLocation = FollowCamera->GetRelativeLocation();
		CameraBaseRotation = FollowCamera->GetRelativeRotation();
		bCameraBaseCaptured = true;
	}

	// Trauma shake: offset grows with trauma squared, smooth noise, fades out.
	if (CameraTrauma > 0.f)
	{
		CameraNoiseTime += DeltaTime * 28.f;
		const float Shake = CameraTrauma * CameraTrauma;
		const FVector Offset(0.f,
			FMath::PerlinNoise1D(CameraNoiseTime) * 22.f * Shake,
			FMath::PerlinNoise1D(CameraNoiseTime + 37.f) * 22.f * Shake);
		const float Roll = FMath::PerlinNoise1D(CameraNoiseTime + 91.f) * 2.f * Shake;
		FollowCamera->SetRelativeLocationAndRotation(CameraBaseLocation + Offset, CameraBaseRotation + FRotator(0.f, 0.f, Roll));

		CameraTrauma = FMath::Max(0.f, CameraTrauma - DeltaTime * 1.8f);
		if (CameraTrauma <= 0.f)
		{
			FollowCamera->SetRelativeLocationAndRotation(CameraBaseLocation, CameraBaseRotation);
		}
	}

	// Damage flash fades; URougeObjectiveWidget draws it as a soft red glow on the screen edges.
	DamageFlash = FMath::Max(0.f, DamageFlash - DeltaTime * 2.5f);
}

void ARougeCharacter::SetCarrySpeedMultiplier(float Multiplier)
{
	if (HasAuthority() && !FMath::IsNearlyEqual(CarrySpeedMultiplier, Multiplier))
	{
		CarrySpeedMultiplier = Multiplier;
		ApplyWalkSpeed();
	}
}

void ARougeCharacter::DropPackagePressed()
{
	ServerDropPackage();
}

void ARougeCharacter::ServerDropPackage_Implementation()
{
	ARougePackage* Package = ARougePackage::Find(GetWorld());
	if (Package && Package->GetCarrier() == this)
	{
		Package->Drop();
	}
}

void ARougeCharacter::UpdateSpeedBuff()
{
	if (HasAuthority() && SpeedBuffMultiplier != 1.f && GetWorld()->GetTimeSeconds() >= SpeedBuffEndTime)
	{
		SpeedBuffMultiplier = 1.f;
		ApplyWalkSpeed();
	}
}

void ARougeCharacter::OnRep_SpeedBuff()
{
	ApplyWalkSpeed();
}

void ARougeCharacter::ApplyWalkSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}
	if (BaseWalkSpeed <= 0.f)
	{
		BaseWalkSpeed = Movement->MaxWalkSpeed;
	}
	Movement->MaxWalkSpeed = BaseWalkSpeed * SpeedBuffMultiplier * CarrySpeedMultiplier;
	RefreshStats();
}

void ARougeCharacter::InitClassDefaults()
{
	if(!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("'%s' CharacterTag is not valid! Please set a valid GameplayTag for the character in the class defaults."), *GetNameSafe(this));
	}

	else if (UCharacterClassInfo* ClassInfo = URougeAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if (const FCharacterClassDefaultInfo* SelectedClassInfo = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if(IsValid(RougeAbilitySystemComp))
			{
				RougeAbilitySystemComp->AddCharacterAbilities(SelectedClassInfo->StartingAbilities);
				RougeAbilitySystemComp->AddCharacterPassiveAbilities(SelectedClassInfo->PassiveAbilities);
				RougeAbilitySystemComp->InitializeDefaultAttributes(SelectedClassInfo->DefaultAttributes);
			}
		}
	}
}

void ARougeCharacter::BindCallbacksToDependencies()
{
	if (IsValid(RougeAbilitySystemComp) && IsValid(RougeAttributes))
	{
		RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(RougeAttributes->GetShieldAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				UpdateShieldBubble(Data.NewValue);
				RefreshStats();
			});

		// Stats panel: refresh when any displayed stat changes.
		for (const FGameplayAttribute& StatAttribute : {
			URougeAttributeSet::GetMaxHealthAttribute(), URougeAttributeSet::GetHealthAttribute(),
			URougeAttributeSet::GetArmorAttribute(), URougeAttributeSet::GetCritChanceAttribute(),
			URougeAttributeSet::GetCritDamageAttribute() })
		{
			RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(StatAttribute).AddLambda(
				[this](const FOnAttributeChangeData&)
				{
					RefreshStats();
				});
		}

		RougeAbilitySystemComp->GetGameplayAttributeValueChangeDelegate(RougeAttributes->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, RougeAttributes->GetMaxHealth());
			});

		// XP is shared by the team now: ARougeGameState drives OnXPChanged.
	}
}

void ARougeCharacter::BroadcastInitialValues()
{
	if (IsValid(RougeAttributes))
	{
		OnHealthChanged(RougeAttributes->GetHealth(), RougeAttributes->GetMaxHealth());
		UpdateShieldBubble(RougeAttributes->GetShield());
		RefreshStats();
		if (const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr)
		{
			OnXPChanged(RougeGameState->GetTeamXP(), RougeGameState->GetXPToNextLevel());
		}
	}
}

