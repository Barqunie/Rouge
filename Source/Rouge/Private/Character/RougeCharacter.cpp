// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/RougeCharacter.h"
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
}

void ARougeCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// The controller is usually not set yet in BeginPlay, so apply the input mode on possession too.
	ApplyGameInputMode();
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

