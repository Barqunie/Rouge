#include "Game/RougeLobbyPlayerController.h"

#include "Character/RougeCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/RougeCharacterSelectionSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "UI/RougeCharacterSelectWidget.h"

ARougeLobbyPlayerController::ARougeLobbyPlayerController()
{
	CharacterSelectWidgetClass = URougeCharacterSelectWidget::StaticClass();
}

void ARougeLobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ARougeLobbyPlayerController, SelectedIndex, COND_OwnerOnly);
}

void ARougeLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && CharacterSelectWidgetClass)
	{
		CharacterSelectWidget = CreateWidget<URougeCharacterSelectWidget>(this, CharacterSelectWidgetClass);
		if (CharacterSelectWidget)
		{
			CharacterSelectWidget->AddToViewport(CharacterSelectZOrder);
		}
	}
}

TArray<TSubclassOf<ARougeCharacter>> ARougeLobbyPlayerController::GetSelectableCharacters() const
{
	if (SelectableCharacters.Num() > 0)
	{
		return SelectableCharacters;
	}

	// The game mode only exists on the server, but its class is replicated through the game state,
	// so every client can read the list from its defaults.
	TArray<TSubclassOf<ARougeCharacter>> Characters;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const UClass* GameModeClass = GameState ? GameState->GameModeClass.Get() : nullptr;
	if (!GameModeClass)
	{
		return Characters;
	}

	const FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(GameModeClass, GameModeCharacterListProperty);
	const FObjectPropertyBase* ClassProperty = ArrayProperty ? CastField<FObjectPropertyBase>(ArrayProperty->Inner) : nullptr;
	if (!ClassProperty)
	{
		return Characters;
	}

	FScriptArrayHelper Array(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(GameModeClass->GetDefaultObject()));
	for (int32 Index = 0; Index < Array.Num(); ++Index)
	{
		// Keep the index order even for bad entries (the game mode picks by index).
		UClass* Class = Cast<UClass>(ClassProperty->GetObjectPropertyValue(Array.GetRawPtr(Index)));
		Characters.Add(Class && Class->IsChildOf(ARougeCharacter::StaticClass()) ? Class : nullptr);
	}
	return Characters;
}

void ARougeLobbyPlayerController::SelectCharacter(int32 Index)
{
	if (!GetSelectableCharacters().IsValidIndex(Index))
	{
		return;
	}

	// Shown right away; the server confirms through replication.
	SelectedIndex = Index;
	ServerSelectCharacter(Index);
}

void ARougeLobbyPlayerController::ServerSelectCharacter_Implementation(int32 Index)
{
	const TArray<TSubclassOf<ARougeCharacter>> Characters = GetSelectableCharacters();
	if (!Characters.IsValidIndex(Index) || !Characters[Index])
	{
		return;
	}

	SelectedIndex = Index;
	ApplyCharacterSelection(Index);
}

void ARougeLobbyPlayerController::ApplyCharacterSelection_Implementation(int32 Index)
{
	if (CallGameModeSelectFunction(Index))
	{
		return;
	}

	// No lobby game mode function: just remember the pick for the game map.
	const TArray<TSubclassOf<ARougeCharacter>> Characters = GetSelectableCharacters();
	const UGameInstance* GameInstance = GetGameInstance();
	if (URougeCharacterSelectionSubsystem* Selection = GameInstance ? GameInstance->GetSubsystem<URougeCharacterSelectionSubsystem>() : nullptr)
	{
		Selection->SetSelectedCharacter(this, Characters[Index]);
	}
}

bool ARougeLobbyPlayerController::CallGameModeSelectFunction(int32 Index)
{
	AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
	UFunction* Function = GameMode ? GameMode->FindFunction(GameModeSelectFunction) : nullptr;
	if (!Function)
	{
		return false;
	}

	// Fill the first controller-compatible object param with this and the first int param with the index.
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Function->ParmsSize, 1)));
	FMemory::Memzero(Params, Function->ParmsSize);

	bool bHasController = false;
	bool bHasIndex = false;
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		FProperty* Param = *It;
		Param->InitializeValue_InContainer(Params);
		if (Param->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
		{
			continue;
		}

		if (const FObjectPropertyBase* ObjectParam = CastField<FObjectPropertyBase>(Param))
		{
			if (!bHasController && IsA(ObjectParam->PropertyClass))
			{
				ObjectParam->SetObjectPropertyValue_InContainer(Params, this);
				bHasController = true;
			}
		}
		else if (const FIntProperty* IntParam = CastField<FIntProperty>(Param))
		{
			if (!bHasIndex)
			{
				IntParam->SetPropertyValue_InContainer(Params, Index);
				bHasIndex = true;
			}
		}
	}

	const bool bCall = bHasController && bHasIndex;
	if (bCall)
	{
		GameMode->ProcessEvent(Function, Params);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RougeLobby: %s::%s needs (Controller, int) params."), *GetNameSafe(GameMode), *GameModeSelectFunction.ToString());
	}

	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params);
	}
	return bCall;
}

void ARougeLobbyPlayerController::SetCharacterSelectVisible(bool bVisible)
{
	if (CharacterSelectWidget)
	{
		CharacterSelectWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
