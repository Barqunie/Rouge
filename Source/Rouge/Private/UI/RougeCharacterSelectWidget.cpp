#include "UI/RougeCharacterSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/RougeCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Game/RougeLobbyPlayerController.h"
#include "UI/RougeUIStyle.h"

namespace
{
	constexpr float CardPortraitSize = 92.f;
	constexpr float SelectInfoPanelWidth = 380.f;
	constexpr float SelectAbilityIconSize = 46.f;
	constexpr float SelectRosterCheckInterval = 0.5f;

	const FLinearColor SelectCardIdleOutline(0.35f, 0.37f, 0.42f, 1.f);
	const FLinearColor SelectDescriptionColor(0.82f, 0.83f, 0.87f, 1.f);

	USizeBox* WrapInSelectSizeBox(UWidgetTree* Tree, UWidget* Content, float Width, float Height)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (Width > 0.f)
		{
			Box->SetWidthOverride(Width);
		}
		if (Height > 0.f)
		{
			Box->SetHeightOverride(Height);
		}
		Box->AddChild(Content);
		return Box;
	}
}

// ---------------------------------------------------------------------------
// URougeCharacterCardHandler
// ---------------------------------------------------------------------------

void URougeCharacterCardHandler::HandleClicked()
{
	if (Owner.IsValid())
	{
		Owner->OnCardClicked(Index);
	}
}

void URougeCharacterCardHandler::HandleHovered()
{
	if (Owner.IsValid())
	{
		Owner->OnCardHovered(Index);
	}
}

void URougeCharacterCardHandler::HandleUnhovered()
{
	if (Owner.IsValid())
	{
		Owner->OnCardUnhovered(Index);
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> URougeCharacterSelectWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	// Only the panels take clicks; the rest of the screen stays usable.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return Super::RebuildWidget();
}

UTextBlock* URougeCharacterSelectWidget::MakeText(const FText& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetFont(RougeUI::MakeFont(Size, bBold));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	return TextBlock;
}

UWidget* URougeCharacterSelectWidget::MakePicture(UTexture2D* Texture, const FLinearColor& Color, const FText& Letter, float Size, int32 LetterSize)
{
	UBorder* Picture = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Picture->SetHorizontalAlignment(HAlign_Center);
	Picture->SetVerticalAlignment(VAlign_Center);

	if (Texture)
	{
		FSlateBrush Brush = RougeUI::MakeRoundedBrush(FLinearColor::White, 6.f);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Size, Size);
		Picture->SetBrush(Brush);
	}
	else
	{
		// Placeholder until an image is set on the Blueprint.
		Picture->SetBrush(RougeUI::MakeRoundedBrush(Color * 0.55f + FLinearColor(0.f, 0.f, 0.f, 0.45f), 6.f));
		Picture->SetContent(MakeText(Letter, LetterSize, FLinearColor::White));
	}

	return WrapInSelectSizeBox(WidgetTree, Picture, Size, Size);
}

void URougeCharacterSelectWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;

	// ---- Title (top center) ----
	UTextBlock* Title = MakeText(NSLOCTEXT("RougeLobby", "ChooseCharacter", "CHOOSE YOUR CHARACTER"), 20, RougeUI::GoldColor);
	if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(Title))
	{
		TitleSlot->SetAnchors(FAnchors(0.5f, 0.f));
		TitleSlot->SetAlignment(FVector2D(0.5f, 0.f));
		TitleSlot->SetPosition(FVector2D(0.f, 28.f));
		TitleSlot->SetAutoSize(true);
	}

	// ---- Info panel (left) ----
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InfoPanel"));
	Panel->SetBrush(RougeUI::MakeRoundedBrush(RougeUI::PanelColor, 10.f, FLinearColor(1.f, 1.f, 1.f, 0.08f), 1.f));
	Panel->SetPadding(FMargin(22.f, 20.f));

	UVerticalBox* InfoContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(InfoContent);

	RoleText = MakeText(FText::GetEmpty(), 12, FLinearColor::White);
	InfoContent->AddChildToVerticalBox(RoleText);

	NameText = MakeText(FText::GetEmpty(), 34, FLinearColor::White);
	InfoContent->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.f, 2.f, 0.f, 8.f));

	DescriptionText = MakeText(FText::GetEmpty(), 13, SelectDescriptionColor, false);
	DescriptionText->SetAutoWrapText(true);
	InfoContent->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

	UTextBlock* AbilitiesLabel = MakeText(NSLOCTEXT("RougeLobby", "Abilities", "ABILITIES"), 11, RougeUI::MutedTextColor);
	InfoContent->AddChildToVerticalBox(AbilitiesLabel)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	AbilityList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AbilityList"));
	InfoContent->AddChildToVerticalBox(AbilityList);

	InfoPanel = WrapInSelectSizeBox(WidgetTree, Panel, SelectInfoPanelWidth, 0.f);
	InfoPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(InfoPanel))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.5f));
		PanelSlot->SetPosition(FVector2D(40.f, -30.f));
		PanelSlot->SetAutoSize(true);
	}

	// ---- Portrait row (bottom center) ----
	RosterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RosterRow"));
	if (UCanvasPanelSlot* RosterSlot = Canvas->AddChildToCanvas(RosterRow))
	{
		RosterSlot->SetAnchors(FAnchors(0.5f, 1.f));
		RosterSlot->SetAlignment(FVector2D(0.5f, 1.f));
		RosterSlot->SetPosition(FVector2D(0.f, -32.f));
		RosterSlot->SetAutoSize(true);
	}
}

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

ARougeLobbyPlayerController* URougeCharacterSelectWidget::GetLobbyController() const
{
	return GetOwningPlayer<ARougeLobbyPlayerController>();
}

const ARougeCharacter* URougeCharacterSelectWidget::GetCharacterDefaults(int32 Index) const
{
	return Roster.IsValidIndex(Index) && Roster[Index] ? Roster[Index]->GetDefaultObject<ARougeCharacter>() : nullptr;
}

FText URougeCharacterSelectWidget::GetCharacterName(int32 Index) const
{
	const ARougeCharacter* Character = GetCharacterDefaults(Index);
	if (!Character)
	{
		return FText::FromString(TEXT("?"));
	}
	if (!Character->GetDisplayInfo().Name.IsEmpty())
	{
		return Character->GetDisplayInfo().Name;
	}

	// BP_RougeTank_C -> Tank
	FString ClassName = Character->GetClass()->GetName();
	ClassName.RemoveFromEnd(TEXT("_C"));
	ClassName.RemoveFromStart(TEXT("BP_"));
	ClassName.RemoveFromStart(TEXT("Rouge"));
	return FText::FromString(ClassName);
}

// ---------------------------------------------------------------------------
// Roster
// ---------------------------------------------------------------------------

void URougeCharacterSelectWidget::RebuildRoster()
{
	RosterRow->ClearChildren();
	CardButtons.Reset();
	CardHandlers.Reset();
	HoveredIndex = INDEX_NONE;
	ShownIndex = INDEX_NONE;
	StyledSelection = INDEX_NONE - 1;

	for (int32 Index = 0; Index < Roster.Num(); ++Index)
	{
		const ARougeCharacter* Character = GetCharacterDefaults(Index);
		const FText Name = GetCharacterName(Index);
		const FLinearColor Color = Character ? Character->GetDisplayInfo().Color : FLinearColor::Gray;

		UButton* Card = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Card->SetContent(Content)))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}

		const FString Initial = Name.ToString().Left(1).ToUpper();
		Content->AddChildToVerticalBox(MakePicture(Character ? Character->GetDisplayInfo().Portrait.Get() : nullptr, Color,
			FText::FromString(Initial), CardPortraitSize, 38))->SetHorizontalAlignment(HAlign_Center);

		UTextBlock* CardName = MakeText(Name, 13, FLinearColor::White);
		UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(CardName);
		NameSlot->SetHorizontalAlignment(HAlign_Center);
		NameSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

		URougeCharacterCardHandler* Handler = NewObject<URougeCharacterCardHandler>(this);
		Handler->Index = Index;
		Handler->Owner = this;
		Card->OnClicked.AddDynamic(Handler, &URougeCharacterCardHandler::HandleClicked);
		Card->OnHovered.AddDynamic(Handler, &URougeCharacterCardHandler::HandleHovered);
		Card->OnUnhovered.AddDynamic(Handler, &URougeCharacterCardHandler::HandleUnhovered);
		CardHandlers.Add(Handler);
		CardButtons.Add(Card);

		RosterRow->AddChildToHorizontalBox(Card)->SetPadding(FMargin(6.f, 0.f));
	}
}

void URougeCharacterSelectWidget::RefreshCardStyles()
{
	const ARougeLobbyPlayerController* Controller = GetLobbyController();
	const int32 Selected = Controller ? Controller->GetSelectedIndex() : INDEX_NONE;
	if (Selected == StyledSelection)
	{
		return;
	}
	StyledSelection = Selected;

	for (int32 Index = 0; Index < CardButtons.Num(); ++Index)
	{
		const ARougeCharacter* Character = GetCharacterDefaults(Index);
		const FLinearColor Color = Character ? Character->GetDisplayInfo().Color : FLinearColor::Gray;
		const bool bSelected = Index == Selected;

		const FLinearColor Fill = bSelected ? RougeUI::PanelColor * 0.8f + Color * 0.12f : RougeUI::PanelColor;
		const FLinearColor NormalFill(Fill.R, Fill.G, Fill.B, 0.95f);
		const FLinearColor HoverFill(Fill.R + 0.04f, Fill.G + 0.04f, Fill.B + 0.05f, 0.97f);

		FButtonStyle Style;
		Style.SetNormal(RougeUI::MakeRoundedBrush(NormalFill, 8.f, bSelected ? Color : SelectCardIdleOutline, bSelected ? 3.f : 1.5f));
		Style.SetHovered(RougeUI::MakeRoundedBrush(HoverFill, 8.f, Color, bSelected ? 3.f : 2.f));
		Style.SetPressed(RougeUI::MakeRoundedBrush(HoverFill, 8.f, Color, 4.f));
		Style.SetNormalPadding(FMargin(8.f));
		Style.SetPressedPadding(FMargin(8.f, 9.f, 8.f, 7.f));
		CardButtons[Index]->SetStyle(Style);
	}
}

// ---------------------------------------------------------------------------
// Info panel
// ---------------------------------------------------------------------------

void URougeCharacterSelectWidget::ShowInfo(int32 Index)
{
	ShownIndex = Index;
	const ARougeCharacter* Character = GetCharacterDefaults(Index);
	if (!Character)
	{
		InfoPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FRougeCharacterDisplayInfo& Info = Character->GetDisplayInfo();
	InfoPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	RoleText->SetText(FText::FromString(Info.Role.ToString().ToUpper()));
	RoleText->SetColorAndOpacity(FSlateColor(Info.Color));
	NameText->SetText(GetCharacterName(Index));
	DescriptionText->SetText(Info.Description);
	DescriptionText->SetVisibility(Info.Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	AbilityList->ClearChildren();
	for (const FRougeAbilityDisplayInfo& Ability : Info.Abilities)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		Row->AddChildToHorizontalBox(MakePicture(Ability.Icon.Get(), Info.Color, Ability.Key, SelectAbilityIconSize, 18))
			->SetVerticalAlignment(VAlign_Top);

		UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		TitleRow->AddChildToHorizontalBox(MakeText(Ability.Name, 14, FLinearColor::White))->SetVerticalAlignment(VAlign_Center);
		if (!Ability.Key.IsEmpty())
		{
			UHorizontalBoxSlot* KeySlot = TitleRow->AddChildToHorizontalBox(MakeText(Ability.Key, 11, Info.Color));
			KeySlot->SetVerticalAlignment(VAlign_Center);
			KeySlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
		Texts->AddChildToVerticalBox(TitleRow);

		if (!Ability.Description.IsEmpty())
		{
			UTextBlock* AbilityDescription = MakeText(Ability.Description, 12, SelectDescriptionColor, false);
			AbilityDescription->SetAutoWrapText(true);
			Texts->AddChildToVerticalBox(AbilityDescription)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}

		UHorizontalBoxSlot* TextsSlot = Row->AddChildToHorizontalBox(Texts);
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

		AbilityList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void URougeCharacterSelectWidget::OnCardClicked(int32 Index)
{
	if (ARougeLobbyPlayerController* Controller = GetLobbyController())
	{
		Controller->SelectCharacter(Index);
	}
}

void URougeCharacterSelectWidget::OnCardHovered(int32 Index)
{
	HoveredIndex = Index;
}

void URougeCharacterSelectWidget::OnCardUnhovered(int32 Index)
{
	if (HoveredIndex == Index)
	{
		HoveredIndex = INDEX_NONE;
	}
}

void URougeCharacterSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ARougeLobbyPlayerController* Controller = GetLobbyController();
	if (!Controller || !RosterRow)
	{
		return;
	}

	// The list comes from the game state on clients, so it can show up a little after the widget.
	RosterCheckCountdown -= InDeltaTime;
	if (RosterCheckCountdown <= 0.f)
	{
		RosterCheckCountdown = SelectRosterCheckInterval;
		const TArray<TSubclassOf<ARougeCharacter>> Characters = Controller->GetSelectableCharacters();
		if (Characters != Roster)
		{
			Roster = Characters;
			RebuildRoster();
		}
	}

	RefreshCardStyles();

	// Hover previews another character; otherwise show the pick.
	const int32 WantedInfo = HoveredIndex != INDEX_NONE ? HoveredIndex : Controller->GetSelectedIndex();
	if (WantedInfo != ShownIndex)
	{
		ShowInfo(WantedInfo);
	}
}
