// Copyright Epic Games, Inc. All Rights Reserved.


#include "RougePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Rouge.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include <Input/RougeInputComponent.h>
#include <Game/PlayerState/RougePlayerState.h>

#include <AbilitySystem/RougeAbilitySystemComponent.h>
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Character/RougeCharacter.h"
#include "Game/RougeGameState.h"
#include "Game/RougeMainGameMode.h"
#include "UI/RougeAugmentSelectionWidget.h"


ARougePlayerController::ARougePlayerController()
{
	bReplicates = true;
	AugmentSelectionWidgetClass = URougeAugmentSelectionWidget::StaticClass();
}

void ARougePlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ARougePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

#if !UE_BUILD_SHIPPING
	if (DebugPressInputTag.IsValid())
	{
		const FGameplayTag InputTag = DebugPressInputTag;
		DebugPressInputTag = FGameplayTag();
		AbilityInputPressed(InputTag);
		AbilityInputReleased(InputTag);
	}
#endif
}

void ARougePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}

		if(URougeInputComponent * RougeInputComp = Cast<URougeInputComponent>(InputComponent))
		{
			RougeInputComp->BindAbilityActions(RougeInputConfig, this, &ThisClass::AbilityInputPressed, &ThisClass::AbilityInputReleased);
		}
	}
}



UAbilitySystemComponent* ARougePlayerController::GetAbilitySystemComponent() const
{
	return RougeAbilitySystemComp;
}

void ARougePlayerController::SetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag ,int32 AbilityLevel)
{
	if (IsValid(RougeAbilitySystemComp))
	{
		RougeAbilitySystemComp->SetDynamicProjectile(ProjectileTag,AbilityLevel);
	}
}

URougeAbilitySystemComponent* ARougePlayerController::GetRougeAbilitySystemComponent()
{
	if(!IsValid(RougeAbilitySystemComp))
	{
		if (const ARougePlayerState* RougePlayerState = GetPlayerState<ARougePlayerState>())
		{
			RougeAbilitySystemComp = RougePlayerState->GetRougeAbilitySystemComponent();
		}
		
	}
	return RougeAbilitySystemComp;
}

void ARougePlayerController::RougeKillSelf()
{
#if !UE_BUILD_SHIPPING
	ServerRougeKillSelf();
#endif
}

void ARougePlayerController::ServerRougeKillSelf_Implementation()
{
#if !UE_BUILD_SHIPPING
	ARougeCharacter* RougeCharacter = GetPawn<ARougeCharacter>();
	if (!RougeCharacter || RougeCharacter->IsDead())
	{
		return;
	}

	if (IsValid(GetRougeAbilitySystemComponent()))
	{
		RougeAbilitySystemComp->SetNumericAttributeBase(URougeAttributeSet::GetHealthAttribute(), 0.f);
	}
	RougeCharacter->HandleHealthDepleted();
#endif
}

// ---------------------------------------------------------------------------
// Augment cards
// ---------------------------------------------------------------------------

void ARougePlayerController::ClientShowAugmentOffers_Implementation(const TArray<FRougeAugmentOffer>& Offers, int32 TeamLevel)
{
	if (!AugmentSelectionWidgetClass)
	{
		// No picker: take the first card so the round can't stall.
		ChooseAugment(0);
		return;
	}

	if (!AugmentSelectionWidget)
	{
		AugmentSelectionWidget = CreateWidget<URougeAugmentSelectionWidget>(this, AugmentSelectionWidgetClass);
	}
	if (!AugmentSelectionWidget)
	{
		ChooseAugment(0);
		return;
	}

	if (!AugmentSelectionWidget->IsInViewport())
	{
		AugmentSelectionWidget->AddToViewport(50);
	}
	AugmentSelectionWidget->ShowOffers(Offers, TeamLevel);

	// Cards are clicked with the mouse while the game is paused.
	SetShowMouseCursor(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(AugmentSelectionWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ARougePlayerController::ClientAugmentRoundFinished_Implementation()
{
	CloseAugmentSelection();
}

void ARougePlayerController::CloseAugmentSelection()
{
	if (AugmentSelectionWidget)
	{
		AugmentSelectionWidget->RemoveFromParent();
	}

	// Back to game input (same as ARougeCharacter::ApplyGameInputMode).
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);
}

void ARougePlayerController::ChooseAugment(int32 OfferIndex)
{
	ServerChooseAugment(OfferIndex);
}

void ARougePlayerController::ServerChooseAugment_Implementation(int32 OfferIndex)
{
	if (ARougeMainGameMode* GameMode = GetWorld()->GetAuthGameMode<ARougeMainGameMode>())
	{
		GameMode->HandleAugmentChosen(this, OfferIndex);
	}
}

void ARougePlayerController::RougeLevelUp()
{
#if !UE_BUILD_SHIPPING
	ServerRougeLevelUp();
#endif
}

void ARougePlayerController::ServerRougeLevelUp_Implementation()
{
#if !UE_BUILD_SHIPPING
	if (ARougeGameState* RougeGameState = GetWorld()->GetGameState<ARougeGameState>())
	{
		RougeGameState->AddTeamXP(RougeGameState->GetXPToNextLevel() - RougeGameState->GetTeamXP());
	}
#endif
}

void ARougePlayerController::ClientShowDamageNumber_Implementation(FVector_NetQuantize Location, int32 Amount, ERougeDamageNumberType Type)
{
	if (!IsLocalController())
	{
		return;
	}

	if (!DamageNumbersWidget)
	{
		DamageNumbersWidget = CreateWidget<URougeDamageNumbersWidget>(this, URougeDamageNumbersWidget::StaticClass());
	}
	if (!DamageNumbersWidget)
	{
		return;
	}

	// Under the HUD and the card picker.
	if (!DamageNumbersWidget->IsInViewport())
	{
		DamageNumbersWidget->AddToViewport(-10);
	}
	DamageNumbersWidget->AddNumber(Location, Amount, Type);
}

bool ARougePlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ARougePlayerController::AbilityInputPressed(FGameplayTag InputTag)
{
	if(IsValid(GetRougeAbilitySystemComponent()))
	{
		RougeAbilitySystemComp->AbilityInputPressed(InputTag);
	}

}


void ARougePlayerController::AbilityInputReleased(FGameplayTag InputTag)
{
	if(IsValid(GetRougeAbilitySystemComponent()))
	{
		RougeAbilitySystemComp->AbilityInputReleased(InputTag);
	}
}
