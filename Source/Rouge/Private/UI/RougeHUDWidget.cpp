// Bottom HUD for the local player. Layout is built in code; everything is refreshed from NativeTick.

#include "UI/RougeHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/RougeSkillAbilities.h"
#include "AbilitySystem/Attributes/RougeAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Character/RougeCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/RougeGameState.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor DarkSlotColor(0.05f, 0.06f, 0.08f, 1.f);
	const FLinearColor BarBackgroundColor(0.f, 0.f, 0.f, 0.7f);
	const FLinearColor MutedTextColor(0.65f, 0.65f, 0.7f, 1.f);

	FSlateBrush MakeRoundedBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent,
		float OutlineWidth = 0.f, bool bCircle = false)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.RoundingType = bCircle ? ESlateBrushRoundingType::HalfHeightRadius : ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}

	FProgressBarStyle MakeBarStyle(const FLinearColor& Background, float Radius)
	{
		FProgressBarStyle Style;
		Style.SetBackgroundImage(MakeRoundedBrush(Background, Radius));
		// White fill, tinted by the bar's FillColorAndOpacity.
		Style.SetFillImage(MakeRoundedBrush(FLinearColor::White, Radius));
		Style.SetMarqueeImage(MakeRoundedBrush(FLinearColor::White, Radius));
		return Style;
	}

	USizeBox* WrapInSizeBox(UWidgetTree* Tree, UWidget* Content, float Width, float Height)
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

	UOverlaySlot* AddToOverlay(UOverlay* Overlay, UWidget* Child, EHorizontalAlignment HAlign = HAlign_Fill,
		EVerticalAlignment VAlign = VAlign_Fill, const FMargin& Padding = FMargin(0.f))
	{
		UOverlaySlot* OverlaySlot = Overlay->AddChildToOverlay(Child);
		OverlaySlot->SetHorizontalAlignment(HAlign);
		OverlaySlot->SetVerticalAlignment(VAlign);
		OverlaySlot->SetPadding(Padding);
		return OverlaySlot;
	}

	const URougeSkillAbility* FindSkillForInput(const UAbilitySystemComponent* ASC, const FGameplayTag& InputTag)
	{
		if (!ASC || !InputTag.IsValid())
		{
			return nullptr;
		}

		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				continue;
			}

			// The instance holds the cooldown; fall back to the CDO for name / icon until it exists.
			if (const URougeSkillAbility* Instance = Cast<URougeSkillAbility>(Spec.GetPrimaryInstance()))
			{
				return Instance;
			}
			return Cast<URougeSkillAbility>(Spec.Ability);
		}

		return nullptr;
	}
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

void URougeHUDWidget::ToggleStatsPanel()
{
	SetStatsPanelOpen(!bStatsPanelOpen);
}

void URougeHUDWidget::SetStatsPanelOpen(bool bOpen)
{
	bStatsPanelOpen = bOpen;
	ApplyStatsPanelVisibility();
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> URougeHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		bStatsPanelOpen = bStatsPanelOpenByDefault;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void URougeHUDWidget::BuildLayout()
{
	// Only the stats toggle button takes clicks; everything else lets them through to the game.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UHorizontalBox* BottomBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomBar"));
	BottomBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UCanvasPanelSlot* BarSlot = Root->AddChildToCanvas(BottomBar);
	BarSlot->SetAnchors(FAnchors(0.5f, 1.f));
	BarSlot->SetAlignment(FVector2D(0.5f, 1.f));
	BarSlot->SetAutoSize(true);
	BarSlot->SetPosition(FVector2D(0.f, -14.f));

	auto AddToBar = [BottomBar](UWidget* Child, EVerticalAlignment VAlign, const FMargin& BarPadding)
	{
		UHorizontalBoxSlot* BoxSlot = BottomBar->AddChildToHorizontalBox(Child);
		BoxSlot->SetVerticalAlignment(VAlign);
		BoxSlot->SetPadding(BarPadding);
	};

	AddToBar(BuildStatsPanel(), VAlign_Bottom, FMargin(0.f, 0.f, 4.f, 0.f));
	AddToBar(BuildStatsToggle(), VAlign_Center, FMargin(0.f, 0.f, 6.f, 0.f));
	// The portrait overlaps the main panel a little, like the reference.
	AddToBar(BuildPortrait(), VAlign_Bottom, FMargin(0.f, 0.f, -12.f, 4.f));
	AddToBar(BuildMainPanel(), VAlign_Bottom, FMargin(0.f));

	ApplyStatsPanelVisibility();
}

UTextBlock* URougeHUDWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));

	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	Font.OutlineSettings.OutlineSize = 1;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	TextBlock->SetFont(Font);
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	return TextBlock;
}

UWidget* URougeHUDWidget::BuildStatsPanel()
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StatsPanel"));
	Panel->SetBrush(MakeRoundedBrush(PanelColor, 8.f, FrameColor, 1.5f));
	Panel->SetPadding(FMargin(12.f, 10.f));

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Content);

	UTextBlock* Title = MakeText(TEXT("STATS"), 12, FrameColor);
	Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	StatsRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatsRows"));
	Content->AddChildToVerticalBox(StatsRows);

	UTextBlock* Hint = MakeText(TEXT("TAB"), 8, MutedTextColor, false);
	UVerticalBoxSlot* HintSlot = Content->AddChildToVerticalBox(Hint);
	HintSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	HintSlot->SetHorizontalAlignment(HAlign_Right);

	StatsPanel = WrapInSizeBox(WidgetTree, Panel, 200.f, 0.f);
	return StatsPanel;
}

void URougeHUDWidget::AddStatRow(const FText& Label, const FText& Value)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UTextBlock* LabelText = MakeText(Label.ToString(), 11, MutedTextColor, false);
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* ValueText = MakeText(Value.ToString(), 11, FLinearColor::White);
	Row->AddChildToHorizontalBox(ValueText)->SetHorizontalAlignment(HAlign_Right);

	StatsRows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 1.f));
}

UWidget* URougeHUDWidget::BuildStatsToggle()
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StatsToggle"));

	FButtonStyle Style;
	Style.SetNormal(MakeRoundedBrush(PanelColor, 5.f, FrameColor, 1.f));
	Style.SetHovered(MakeRoundedBrush(FLinearColor(0.12f, 0.1f, 0.06f, 0.95f), 5.f, FrameColor, 1.5f));
	Style.SetPressed(MakeRoundedBrush(FLinearColor(0.2f, 0.16f, 0.08f, 1.f), 5.f, FrameColor, 1.5f));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
	Button->OnClicked.AddDynamic(this, &ThisClass::HandleStatsToggleClicked);

	StatsToggleText = MakeText(TEXT("<"), 12, FrameColor);
	Button->SetContent(StatsToggleText);

	return WrapInSizeBox(WidgetTree, Button, 22.f, 56.f);
}

UWidget* URougeHUDWidget::BuildPortrait()
{
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Portrait"));
	Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	PortraitCircle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitCircle"));
	PortraitCircle->SetBrush(MakeRoundedBrush(FighterColor, 0.f, FrameColor, 3.f, true));
	PortraitCircle->SetHorizontalAlignment(HAlign_Center);
	PortraitCircle->SetVerticalAlignment(VAlign_Center);
	PortraitText = MakeText(TEXT("F"), 36, FLinearColor::White);
	PortraitCircle->SetContent(PortraitText);
	AddToOverlay(Overlay, PortraitCircle);

	// Level badge in the bottom left corner.
	UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Badge->SetBrush(MakeRoundedBrush(DarkSlotColor, 0.f, FrameColor, 2.f, true));
	Badge->SetHorizontalAlignment(HAlign_Center);
	Badge->SetVerticalAlignment(VAlign_Center);
	LevelText = MakeText(TEXT("1"), 12, FLinearColor::White);
	Badge->SetContent(LevelText);
	AddToOverlay(Overlay, WrapInSizeBox(WidgetTree, Badge, 30.f, 30.f), HAlign_Left, VAlign_Bottom);

	return WrapInSizeBox(WidgetTree, Overlay, 96.f, 96.f);
}

UWidget* URougeHUDWidget::BuildMainPanel()
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainPanel"));
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->SetBrush(MakeRoundedBrush(PanelColor, 8.f, FrameColor, 1.5f));
	Panel->SetPadding(FMargin(20.f, 10.f, 12.f, 10.f));

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Content);

	// Skill row: Q, E, aim mode, then the class name on the right.
	UHorizontalBox* SkillRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const FGameplayTag QTag = FGameplayTag::RequestGameplayTag(TEXT("Input.QAbility"), false);
	const FGameplayTag ETag = FGameplayTag::RequestGameplayTag(TEXT("Input.EAbility"), false);
	SkillRow->AddChildToHorizontalBox(BuildSkillSlot(QSkillSlot, QTag, TEXT("Q")))->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	SkillRow->AddChildToHorizontalBox(BuildSkillSlot(ESkillSlot, ETag, TEXT("E")))->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	SkillRow->AddChildToHorizontalBox(BuildAimSlot());

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	SkillRow->AddChildToHorizontalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	ClassNameText = MakeText(TEXT("Fighter"), 14, FrameColor);
	UHorizontalBoxSlot* ClassSlot = SkillRow->AddChildToHorizontalBox(ClassNameText);
	ClassSlot->SetVerticalAlignment(VAlign_Top);
	ClassSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

	Content->AddChildToVerticalBox(SkillRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	Content->AddChildToVerticalBox(BuildBar(HealthBar, HealthText, HealthColor, 20.f, 12));

	ShieldRow = BuildBar(ShieldBar, ShieldText, ShieldColor, 12.f, 9);
	Content->AddChildToVerticalBox(ShieldRow)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	ShieldRow->SetVisibility(ESlateVisibility::Collapsed);

	Content->AddChildToVerticalBox(BuildBar(XPBar, XPText, XPColor, 12.f, 9))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	return WrapInSizeBox(WidgetTree, Panel, 340.f, 0.f);
}

UWidget* URougeHUDWidget::BuildSkillSlot(FRougeHUDSkillSlot& SkillSlot, const FGameplayTag& InputTag, const FString& KeyLabel)
{
	SkillSlot.InputTag = InputTag;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	SkillSlot.Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	SkillSlot.Frame->SetBrush(MakeRoundedBrush(DarkSlotColor, 6.f, FighterColor, 2.f));
	AddToOverlay(Overlay, SkillSlot.Frame);

	SkillSlot.Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	SkillSlot.Icon->SetVisibility(ESlateVisibility::Collapsed);
	AddToOverlay(Overlay, SkillSlot.Icon, HAlign_Fill, VAlign_Fill, FMargin(4.f));

	SkillSlot.NameText = MakeText(TEXT("-"), 10, FLinearColor::White);
	SkillSlot.NameText->SetJustification(ETextJustify::Center);
	AddToOverlay(Overlay, SkillSlot.NameText, HAlign_Center, VAlign_Center, FMargin(2.f, 0.f, 2.f, 8.f));

	// Dark fill from the bottom that shrinks while the cooldown runs.
	SkillSlot.CooldownShade = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	SkillSlot.CooldownShade->SetWidgetStyle(MakeBarStyle(FLinearColor::Transparent, 5.f));
	SkillSlot.CooldownShade->SetBarFillType(EProgressBarFillType::BottomToTop);
	SkillSlot.CooldownShade->SetFillColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	SkillSlot.CooldownShade->SetPercent(0.f);
	AddToOverlay(Overlay, SkillSlot.CooldownShade, HAlign_Fill, VAlign_Fill, FMargin(2.f));

	SkillSlot.CooldownText = MakeText(TEXT(""), 18, FLinearColor::White);
	SkillSlot.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	AddToOverlay(Overlay, SkillSlot.CooldownText, HAlign_Center, VAlign_Center);

	// Key label in the bottom right corner.
	UBorder* KeyBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	KeyBadge->SetBrush(MakeRoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.8f), 3.f));
	KeyBadge->SetPadding(FMargin(4.f, 0.f));
	KeyBadge->SetContent(MakeText(KeyLabel, 10, FrameColor));
	AddToOverlay(Overlay, KeyBadge, HAlign_Right, VAlign_Bottom, FMargin(0.f, 0.f, 3.f, 3.f));

	return WrapInSizeBox(WidgetTree, Overlay, 58.f, 58.f);
}

UWidget* URougeHUDWidget::BuildAimSlot()
{
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	AimFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	AimFrame->SetBrush(MakeRoundedBrush(DarkSlotColor, 6.f, MutedTextColor, 1.5f));
	AddToOverlay(Overlay, AimFrame);

	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Lines->AddChildToVerticalBox(MakeText(TEXT("AIM"), 8, MutedTextColor, false))->SetHorizontalAlignment(HAlign_Center);
	AimText = MakeText(TEXT("MOUSE"), 10, FLinearColor::White);
	Lines->AddChildToVerticalBox(AimText)->SetHorizontalAlignment(HAlign_Center);
	AddToOverlay(Overlay, Lines, HAlign_Center, VAlign_Center, FMargin(0.f, 0.f, 0.f, 8.f));

	UBorder* KeyBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	KeyBadge->SetBrush(MakeRoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.8f), 3.f));
	KeyBadge->SetPadding(FMargin(4.f, 0.f));
	KeyBadge->SetContent(MakeText(TEXT("LMB"), 8, FrameColor));
	AddToOverlay(Overlay, KeyBadge, HAlign_Right, VAlign_Bottom, FMargin(0.f, 0.f, 3.f, 3.f));

	return WrapInSizeBox(WidgetTree, Overlay, 58.f, 58.f);
}

UWidget* URougeHUDWidget::BuildBar(TObjectPtr<UProgressBar>& OutBar, TObjectPtr<UTextBlock>& OutText,
	const FLinearColor& FillColor, float Height, int32 FontSize)
{
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	OutBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	OutBar->SetWidgetStyle(MakeBarStyle(BarBackgroundColor, 3.f));
	OutBar->SetFillColorAndOpacity(FillColor);
	OutBar->SetPercent(1.f);
	AddToOverlay(Overlay, OutBar);

	OutText = MakeText(TEXT(""), FontSize, FLinearColor::White);
	AddToOverlay(Overlay, OutText, HAlign_Center, VAlign_Center);

	return WrapInSizeBox(WidgetTree, Overlay, 0.f, Height);
}

// ---------------------------------------------------------------------------
// Updates
// ---------------------------------------------------------------------------

void URougeHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ARougeCharacter* RougeCharacter = Cast<ARougeCharacter>(GetOwningPlayerPawn());
	if (!RougeCharacter || !HealthBar)
	{
		return;
	}

	UAbilitySystemComponent* ASC = RougeCharacter->GetAbilitySystemComponent();

	RefreshClassVisuals(RougeCharacter);
	RefreshVitals(ASC);
	RefreshTeamXP();
	RefreshSkills(ASC);
	RefreshAimSlot(RougeCharacter);

	StatsRefreshCountdown -= InDeltaTime;
	if (StatsRefreshCountdown <= 0.f)
	{
		StatsRefreshCountdown = StatsRefreshInterval;
		RefreshStatsPanel(RougeCharacter);
	}
}

void URougeHUDWidget::RefreshVitals(const UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(URougeAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(URougeAttributeSet::GetMaxHealthAttribute());
	HealthBar->SetPercent(MaxHealth > 0.f ? Health / MaxHealth : 0.f);
	HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth))));

	const float Shield = ASC->GetNumericAttribute(URougeAttributeSet::GetShieldAttribute());
	if (Shield > 0.f)
	{
		ShieldPeak = FMath::Max(ShieldPeak, Shield);
		ShieldBar->SetPercent(Shield / ShieldPeak);
		ShieldText->SetText(FText::FromString(FString::Printf(TEXT("Shield %d"), FMath::RoundToInt(Shield))));
		ShieldRow->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		ShieldPeak = 0.f;
		ShieldRow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URougeHUDWidget::RefreshTeamXP()
{
	const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr;
	if (!RougeGameState)
	{
		return;
	}

	const float XP = RougeGameState->GetTeamXP();
	const float XPToNext = RougeGameState->GetXPToNextLevel();
	const int32 Level = RougeGameState->GetTeamLevel();

	XPBar->SetPercent(XPToNext > 0.f ? XP / XPToNext : 0.f);
	XPText->SetText(FText::FromString(FString::Printf(TEXT("Lv %d   %d / %d XP"), Level, FMath::FloorToInt(XP), FMath::RoundToInt(XPToNext))));
	LevelText->SetText(FText::AsNumber(Level));
}

void URougeHUDWidget::RefreshSkills(UAbilitySystemComponent* ASC)
{
	RefreshSkillSlot(QSkillSlot, FindSkillForInput(ASC, QSkillSlot.InputTag));
	RefreshSkillSlot(ESkillSlot, FindSkillForInput(ASC, ESkillSlot.InputTag));
}

void URougeHUDWidget::RefreshSkillSlot(FRougeHUDSkillSlot& SkillSlot, const URougeSkillAbility* Skill)
{
	if (SkillSlot.BoundSkill.Get() != Skill)
	{
		SkillSlot.BoundSkill = Skill;

		UTexture2D* SkillIcon = Skill ? Skill->GetSkillIcon() : nullptr;
		if (SkillIcon)
		{
			SkillSlot.Icon->SetBrushFromTexture(SkillIcon);
			SkillSlot.Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
			SkillSlot.NameText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			SkillSlot.Icon->SetVisibility(ESlateVisibility::Collapsed);
			SkillSlot.NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
			SkillSlot.NameText->SetText(Skill ? Skill->GetSkillName() : FText::FromString(TEXT("-")));
		}
	}

	const float Remaining = Skill ? Skill->GetCooldownRemaining() : 0.f;
	const float Duration = Skill ? Skill->GetCooldownDuration() : 0.f;

	SkillSlot.CooldownShade->SetPercent(Duration > 0.f ? FMath::Clamp(Remaining / Duration, 0.f, 1.f) : 0.f);

	if (Remaining > 0.f)
	{
		const FString Seconds = Remaining >= 1.f
			? FString::FromInt(FMath::CeilToInt(Remaining))
			: FString::Printf(TEXT("%.1f"), Remaining);
		SkillSlot.CooldownText->SetText(FText::FromString(Seconds));
		SkillSlot.CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		SkillSlot.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URougeHUDWidget::RefreshClassVisuals(const ARougeCharacter* RougeCharacter)
{
	const FGameplayTag ClassTag = RougeCharacter->GetCharacterTag();
	if (bClassVisualsApplied && ClassTag == LastClassTag)
	{
		return;
	}
	bClassVisualsApplied = true;
	LastClassTag = ClassTag;

	// Characters.Player.Melee.* / Ranged.* / Tank, with the pawn class name as a fallback.
	const FString Source = ClassTag.ToString() + TEXT(" ") + RougeCharacter->GetClass()->GetName();

	FLinearColor ClassColor = FighterColor;
	FString ClassName = TEXT("Fighter");
	if (Source.Contains(TEXT("Tank")))
	{
		ClassColor = TankColor;
		ClassName = TEXT("Tank");
	}
	else if (Source.Contains(TEXT("Ranged")))
	{
		ClassColor = RangedColor;
		ClassName = TEXT("Ranged");
	}

	PortraitCircle->SetBrush(MakeRoundedBrush(ClassColor, 0.f, FrameColor, 3.f, true));
	PortraitText->SetText(FText::FromString(ClassName.Left(1)));
	ClassNameText->SetText(FText::FromString(ClassName));

	const FSlateBrush SlotBrush = MakeRoundedBrush(DarkSlotColor, 6.f, ClassColor, 2.f);
	QSkillSlot.Frame->SetBrush(SlotBrush);
	ESkillSlot.Frame->SetBrush(SlotBrush);
}

void URougeHUDWidget::RefreshAimSlot(const ARougeCharacter* RougeCharacter)
{
	const bool bAuto = RougeCharacter->IsAutoAimEnabled();
	if (ShownAutoAim == static_cast<int8>(bAuto))
	{
		return;
	}
	ShownAutoAim = static_cast<int8>(bAuto);

	AimText->SetText(FText::FromString(bAuto ? TEXT("AUTO") : TEXT("MOUSE")));
	AimText->SetColorAndOpacity(FSlateColor(bAuto ? FrameColor : FLinearColor::White));
	AimFrame->SetBrush(MakeRoundedBrush(DarkSlotColor, 6.f, bAuto ? FrameColor : MutedTextColor, bAuto ? 2.f : 1.5f));
}

void URougeHUDWidget::RefreshStatsPanel(const ARougeCharacter* RougeCharacter)
{
	if (!bStatsPanelOpen || !StatsRows)
	{
		return;
	}

	const TArray<FRougeStatLine> Lines = RougeCharacter->GetStatLines();

	// Rows change only when a stat appears / disappears (shield); otherwise just update the texts.
	if (StatsRows->GetChildrenCount() != Lines.Num())
	{
		StatsRows->ClearChildren();
		for (const FRougeStatLine& Line : Lines)
		{
			AddStatRow(Line.Label, Line.Value);
		}
		return;
	}

	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const UPanelWidget* Row = Cast<UPanelWidget>(StatsRows->GetChildAt(Index));
		UTextBlock* LabelText = Row ? Cast<UTextBlock>(Row->GetChildAt(0)) : nullptr;
		UTextBlock* ValueText = Row ? Cast<UTextBlock>(Row->GetChildAt(1)) : nullptr;
		if (LabelText && ValueText)
		{
			LabelText->SetText(Lines[Index].Label);
			ValueText->SetText(Lines[Index].Value);
		}
	}
}

void URougeHUDWidget::ApplyStatsPanelVisibility()
{
	if (StatsPanel)
	{
		// Hidden (not collapsed) so the rest of the bar doesn't shift when the panel closes.
		StatsPanel->SetVisibility(bStatsPanelOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (StatsToggleText)
	{
		StatsToggleText->SetText(FText::FromString(bStatsPanelOpen ? TEXT("<") : TEXT(">")));
	}
	StatsRefreshCountdown = 0.f;
}

void URougeHUDWidget::HandleStatsToggleClicked()
{
	ToggleStatsPanel();
}
