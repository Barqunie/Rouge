#include "UI/RougeAugmentSelectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "RougePlayerController.h"
#include "UI/RougeUIStyle.h"

namespace
{
	constexpr int32 MaxCards = 4;
	constexpr float CardWidth = 230.f;
	constexpr float CardHeight = 300.f;
}

TSharedRef<SWidget> URougeAugmentSelectionWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	return Super::RebuildWidget();
}

UTextBlock* URougeAugmentSelectionWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetFont(RougeUI::MakeFont(Size, bBold));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	return TextBlock;
}

void URougeAugmentSelectionWidget::BuildLayout()
{
	// Dim, click-blocking background over the whole screen.
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrush(RougeUI::MakeRoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Background->SetContent(Content);

	TitleText = MakeText(TEXT("LEVEL UP!"), 34, RougeUI::GoldColor);
	Content->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Center);

	UTextBlock* Subtitle = MakeText(TEXT("Choose an augment"), 15, FLinearColor::White, false);
	UVerticalBoxSlot* SubtitleSlot = Content->AddChildToVerticalBox(Subtitle);
	SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
	SubtitleSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 22.f));

	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardRow"));
	Content->AddChildToVerticalBox(CardRow)->SetHorizontalAlignment(HAlign_Center);

	StatusText = MakeText(TEXT(""), 14, RougeUI::MutedTextColor, false);
	UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
}

FLinearColor URougeAugmentSelectionWidget::GetCategoryColor(ERougeAugmentCategory Category) const
{
	return RougeUI::GetAugmentCategoryColor(Category);
}

FText URougeAugmentSelectionWidget::GetCategoryLabel(ERougeAugmentCategory Category) const
{
	switch (Category)
	{
	case ERougeAugmentCategory::Skill:
		return NSLOCTEXT("RougeAugments", "SkillCategory", "SKILL");
	case ERougeAugmentCategory::Passive:
		return NSLOCTEXT("RougeAugments", "PassiveCategory", "PASSIVE");
	default:
		return NSLOCTEXT("RougeAugments", "StatCategory", "STAT");
	}
}

UButton* URougeAugmentSelectionWidget::BuildCard(const FRougeAugmentOffer& Offer)
{
	const FLinearColor CategoryColor = GetCategoryColor(Offer.Category);
	const FLinearColor RarityColor = RougeUI::GetAugmentRarityColor(Offer.Rarity);

	// Border and a faint tint in the rarity color.
	const FLinearColor CardFill = RougeUI::PanelColor * 0.9f + RarityColor * 0.025f;
	const FLinearColor NormalFill(CardFill.R, CardFill.G, CardFill.B, 0.94f);
	const FLinearColor HoveredFill(CardFill.R + 0.04f, CardFill.G + 0.04f, CardFill.B + 0.05f, 0.97f);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle Style;
	Style.SetNormal(RougeUI::MakeRoundedBrush(NormalFill, 10.f, RarityColor, 2.f));
	Style.SetHovered(RougeUI::MakeRoundedBrush(HoveredFill, 10.f, RarityColor, 4.f));
	Style.SetPressed(RougeUI::MakeRoundedBrush(FLinearColor(0.1f, 0.1f, 0.14f, 1.f), 10.f, RarityColor, 5.f));
	Style.SetDisabled(RougeUI::MakeRoundedBrush(FLinearColor(0.02f, 0.02f, 0.03f, 0.7f), 10.f, FLinearColor(0.3f, 0.3f, 0.3f, 1.f), 1.f));
	Style.SetNormalPadding(FMargin(18.f, 16.f));
	Style.SetPressedPadding(FMargin(18.f, 17.f, 18.f, 15.f));
	Button->SetStyle(Style);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// Fill the card so the text starts at the top and the stack counter sits at the bottom.
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->SetContent(Content)))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Top row: category (left) and rarity (right).
	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Content->AddChildToVerticalBox(TopRow);

	UTextBlock* CategoryText = MakeText(GetCategoryLabel(Offer.Category).ToString(), 11, CategoryColor);
	TopRow->AddChildToHorizontalBox(CategoryText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* RarityText = MakeText(RougeUI::GetAugmentRarityLabel(Offer.Rarity).ToString(), 11, RarityColor);
	TopRow->AddChildToHorizontalBox(RarityText)->SetHorizontalAlignment(HAlign_Right);

	UTextBlock* NameText = MakeText(Offer.DisplayName.ToString(), 20, RarityColor);
	NameText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));

	UTextBlock* DescriptionText = MakeText(Offer.Description.ToString(), 13, FLinearColor(0.85f, 0.85f, 0.88f, 1.f), false);
	DescriptionText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(DescriptionText);

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Content->AddChildToVerticalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Stack counter: how many the player has -> after this pick.
	const FString StackLabel = Offer.MaxStacks > 1
		? FString::Printf(TEXT("%d / %d"), Offer.CurrentStacks + 1, Offer.MaxStacks)
		: FString(TEXT("Unique"));
	UTextBlock* StackText = MakeText(StackLabel, 11, RougeUI::MutedTextColor, false);
	Content->AddChildToVerticalBox(StackText)->SetHorizontalAlignment(HAlign_Right);

	return Button;
}

void URougeAugmentSelectionWidget::ShowOffers(const TArray<FRougeAugmentOffer>& InOffers, int32 TeamLevel)
{
	// Make sure the tree exists even if the widget wasn't added to the viewport yet.
	TakeWidget();

	Offers = InOffers;
	if (Offers.Num() > MaxCards)
	{
		Offers.SetNum(MaxCards);
	}
	bChosen = false;

	TitleText->SetText(FText::FromString(FString::Printf(TEXT("LEVEL %d!"), TeamLevel)));
	StatusText->SetText(FText::GetEmpty());

	CardRow->ClearChildren();
	CardButtons.Reset();

	static const FName HandlerNames[MaxCards] =
	{
		GET_FUNCTION_NAME_CHECKED(URougeAugmentSelectionWidget, HandleCard0Clicked),
		GET_FUNCTION_NAME_CHECKED(URougeAugmentSelectionWidget, HandleCard1Clicked),
		GET_FUNCTION_NAME_CHECKED(URougeAugmentSelectionWidget, HandleCard2Clicked),
		GET_FUNCTION_NAME_CHECKED(URougeAugmentSelectionWidget, HandleCard3Clicked)
	};

	for (int32 Index = 0; Index < Offers.Num(); ++Index)
	{
		UButton* Card = BuildCard(Offers[Index]);
		FScriptDelegate ClickDelegate;
		ClickDelegate.BindUFunction(this, HandlerNames[Index]);
		Card->OnClicked.Add(ClickDelegate);
		CardButtons.Add(Card);

		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(CardWidth);
		Box->SetHeightOverride(CardHeight);
		Box->AddChild(Card);

		CardRow->AddChildToHorizontalBox(Box)->SetPadding(FMargin(10.f, 0.f));
	}
}

void URougeAugmentSelectionWidget::Choose(int32 Index)
{
	if (bChosen || !Offers.IsValidIndex(Index))
	{
		return;
	}
	bChosen = true;

	// Lock the cards; the picked one stays lit.
	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		if (CardIndex != Index && CardButtons[CardIndex])
		{
			CardButtons[CardIndex]->SetIsEnabled(false);
		}
	}
	StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s picked. Waiting for teammates..."), *Offers[Index].DisplayName.ToString())));

	if (ARougePlayerController* PlayerController = GetOwningPlayer<ARougePlayerController>())
	{
		PlayerController->ChooseAugment(Index);
	}
}

void URougeAugmentSelectionWidget::HandleCard0Clicked()
{
	Choose(0);
}

void URougeAugmentSelectionWidget::HandleCard1Clicked()
{
	Choose(1);
}

void URougeAugmentSelectionWidget::HandleCard2Clicked()
{
	Choose(2);
}

void URougeAugmentSelectionWidget::HandleCard3Clicked()
{
	Choose(3);
}
