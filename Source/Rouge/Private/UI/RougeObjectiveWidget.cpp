#include "UI/RougeObjectiveWidget.h"

#include "Actors/RougeStation.h"
#include "Actors/RougePackage.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Character/EnemyBase.h"
#include "Character/RougeCharacter.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/RougeGameState.h"
#include "GameFramework/PlayerController.h"
#include "UI/RougeUIStyle.h"

namespace
{
	constexpr float ObjectiveEdgeMargin = 56.f;
	// Unreal units per meter shown on the marker.
	constexpr float UnitsPerMeter = 100.f;

	const FLinearColor ObjectiveGold(1.f, 0.75f, 0.2f, 1.f);
	const FLinearColor ObjectiveGreen(0.4f, 1.f, 0.5f, 1.f);
	const FLinearColor ObjectiveOrange(1.f, 0.55f, 0.15f, 1.f);
	const FLinearColor ObjectiveRed(1.f, 0.3f, 0.25f, 1.f);
}

TSharedRef<SWidget> URougeObjectiveWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

UTextBlock* URougeObjectiveWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetFont(RougeUI::MakeFont(Size, bBold));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	return TextBlock;
}

void URougeObjectiveWidget::BuildLayout()
{
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;

	// ---- Damage warning: soft red on the screen edges only (first, so everything else draws over it) ----
	if (UMaterialInterface* EdgeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ThirdPerson/Materials/FX/M_DamageEdge.M_DamageEdge")))
	{
		DamageEdgeMID = UMaterialInstanceDynamic::Create(EdgeMaterial, this);
		DamageEdge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DamageEdge"));
		DamageEdge->SetBrushFromMaterial(DamageEdgeMID);
		DamageEdge->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* EdgeSlot = Canvas->AddChildToCanvas(DamageEdge))
		{
			EdgeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			EdgeSlot->SetOffsets(FMargin(0.f));
		}
	}

	// ---- Status line (top center) ----
	StatusBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StatusBox"));
	StatusBox->SetBrush(RougeUI::MakeRoundedBrush(FLinearColor(0.02f, 0.025f, 0.035f, 0.75f), 8.f));
	StatusBox->SetPadding(FMargin(14.f, 6.f));
	StatusText = MakeText(TEXT(""), 14, FLinearColor::White);
	StatusBox->SetContent(StatusText);
	if (UCanvasPanelSlot* StatusSlot = Canvas->AddChildToCanvas(StatusBox))
	{
		StatusSlot->SetAnchors(FAnchors(0.5f, 0.f));
		StatusSlot->SetAlignment(FVector2D(0.5f, 0.f));
		StatusSlot->SetPosition(FVector2D(0.f, 16.f));
		StatusSlot->SetAutoSize(true);
	}

	// ---- Wave counter (top right) ----
	WaveBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WaveBox"));
	WaveBox->SetBrush(RougeUI::MakeRoundedBrush(FLinearColor(0.02f, 0.025f, 0.035f, 0.8f), 8.f, RougeUI::GoldColor * FLinearColor(1.f, 1.f, 1.f, 0.5f), 1.f));
	WaveBox->SetPadding(FMargin(16.f, 8.f));
	UVerticalBox* WaveContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	WaveBox->SetContent(WaveContent);
	WaveTitle = MakeText(TEXT(""), 18, RougeUI::GoldColor);
	WaveContent->AddChildToVerticalBox(WaveTitle)->SetHorizontalAlignment(HAlign_Right);
	WaveDetail = MakeText(TEXT(""), 13, FLinearColor::White, false);
	WaveContent->AddChildToVerticalBox(WaveDetail)->SetHorizontalAlignment(HAlign_Right);
	WaveStation = MakeText(TEXT(""), 13, ObjectiveGreen, false);
	if (UVerticalBoxSlot* StationSlot = WaveContent->AddChildToVerticalBox(WaveStation))
	{
		StationSlot->SetHorizontalAlignment(HAlign_Right);
		StationSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	WaveBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* WaveSlot = Canvas->AddChildToCanvas(WaveBox))
	{
		WaveSlot->SetAnchors(FAnchors(1.f, 0.f));
		WaveSlot->SetAlignment(FVector2D(1.f, 0.f));
		WaveSlot->SetPosition(FVector2D(-20.f, 16.f));
		WaveSlot->SetAutoSize(true);
	}

	// ---- Marker: arrow glyph + distance ----
	Marker = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Marker"));
	MarkerArrow = MakeText(TEXT("▼"), 22, ObjectiveGold);
	MarkerArrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Marker->AddChildToVerticalBox(MarkerArrow)->SetHorizontalAlignment(HAlign_Center);
	MarkerDistance = MakeText(TEXT(""), 11, FLinearColor::White);
	Marker->AddChildToVerticalBox(MarkerDistance)->SetHorizontalAlignment(HAlign_Center);
	if (UCanvasPanelSlot* MarkerSlot = Canvas->AddChildToCanvas(Marker))
	{
		MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MarkerSlot->SetAutoSize(true);
	}

	// ---- Result banner (center) ----
	ResultBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultBox"));
	ResultBox->SetBrush(RougeUI::MakeRoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.7f), 12.f));
	ResultBox->SetPadding(FMargin(40.f, 20.f));
	UVerticalBox* ResultContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ResultBox->SetContent(ResultContent);
	ResultTitle = MakeText(TEXT(""), 40, ObjectiveGold);
	ResultContent->AddChildToVerticalBox(ResultTitle)->SetHorizontalAlignment(HAlign_Center);
	ResultSubtitle = MakeText(TEXT(""), 15, RougeUI::MutedTextColor, false);
	ResultContent->AddChildToVerticalBox(ResultSubtitle)->SetHorizontalAlignment(HAlign_Center);
	ResultBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* ResultSlot = Canvas->AddChildToCanvas(ResultBox))
	{
		ResultSlot->SetAnchors(FAnchors(0.5f, 0.4f));
		ResultSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ResultSlot->SetAutoSize(true);
	}
}

void URougeObjectiveWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;

	if (!StatusText)
	{
		return;
	}

	RefreshDamageEdge();
	RefreshStatus();
	RefreshMarker(MyGeometry);
	RefreshWaves();
	RefreshResult();
}

void URougeObjectiveWidget::RefreshStatus()
{
	// Dead: how to look around / watch the team.
	if (const ARougeCharacter* LocalCharacter = Cast<ARougeCharacter>(GetOwningPlayerPawn()); LocalCharacter && LocalCharacter->IsSpectating())
	{
		StatusBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		const ARougeCharacter* Target = LocalCharacter->GetSpectateTarget();
		const FString Name = Target ? Target->GetPlayerDisplayName() : FString();
		StatusText->SetText(FText::FromString(Target
			? FString::Printf(TEXT("Watching %s   [LMB] next teammate   [RMB drag / screen edges] look around"), Name.IsEmpty() ? TEXT("teammate") : *Name)
			: FString(TEXT("You are down   [LMB] watch a teammate   [RMB drag / screen edges] look around"))));
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.85f, 1.f)));
		return;
	}

	const ARougePackage* Package = ARougePackage::Find(GetWorld());
	if (!Package)
	{
		StatusBox->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	StatusBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	const APawn* LocalPawn = GetOwningPlayerPawn();
	const ACharacter* Carrier = Package->GetCarrier();

	const ARougeGameState* RougeGameState = GetWorld()->GetGameState<ARougeGameState>();
	const ARougeStation* Station = RougeGameState ? Cast<ARougeStation>(RougeGameState->GetActiveStation()) : nullptr;
	const FString StationName = !Station ? FString(TEXT("the station"))
		: Station->IsFinal() ? FString(TEXT("the FINAL")) : FString::Printf(TEXT("Station %d"), Station->GetStationNumber());

	FString Status;
	FLinearColor Color = FLinearColor::White;
	if (Station && Package->GetDockedStation() == Station)
	{
		const bool bWaveLeft = RougeGameState->GetEnemiesRemaining() > 0;
		Status = Station->GetProgress() < 1.f
			? FString::Printf(TEXT("Purifying at %s - defend it! %d%%"), *StationName, FMath::FloorToInt(Station->GetProgress() * 100.f))
			: bWaveLeft ? FString(TEXT("Purified - clear the final wave!")) : FString(TEXT("Purified!"));
		Color = ObjectiveGreen;
	}
	else if (Carrier && Carrier == LocalPawn)
	{
		Status = Station
			? FString::Printf(TEXT("You have the package - take it to %s!   [F] drop"), *StationName)
			: FString(TEXT("You have the package - keep it safe until a station opens   [F] drop"));
		Color = ObjectiveGold;
	}
	else if (const ARougeCharacter* PlayerCarrier = Cast<ARougeCharacter>(Carrier))
	{
		const FString Name = PlayerCarrier->GetPlayerDisplayName();
		Status = FString::Printf(TEXT("%s has the package - protect them!"), Name.IsEmpty() ? TEXT("A teammate") : *Name);
		Color = ObjectiveGreen;
	}
	else if (Package->IsCarriedByEnemy())
	{
		Status = TEXT("An enemy stole the package - kill it!");
		Color = ObjectiveRed;
	}
	else
	{
		Status = TEXT("The package is on the ground - pick it up!");
		Color = ObjectiveOrange;
	}

	StatusText->SetText(FText::FromString(Status));
	StatusText->SetColorAndOpacity(FSlateColor(Color));
}

void URougeObjectiveWidget::RefreshMarker(const FGeometry& MyGeometry)
{
	APlayerController* PlayerController = GetOwningPlayer();
	const ARougePackage* Package = ARougePackage::Find(GetWorld());
	const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr;
	const bool bRunOver = RougeGameState && RougeGameState->IsRunOver();
	if (!PlayerController || !Package || bRunOver)
	{
		Marker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// While a player carries it (or it is docked), point at the active station; otherwise at the package.
	FVector TargetLocation = Package->GetActorLocation() + FVector(0.f, 0.f, 90.f);
	FLinearColor Color = Package->IsCarriedByEnemy() ? ObjectiveRed : ObjectiveGold;
	const AActor* Station = RougeGameState ? RougeGameState->GetActiveStation() : nullptr;
	if (Station && (Package->IsCarriedByPlayer() || Package->GetDockedStation() == Station))
	{
		TargetLocation = Station->GetActorLocation() + FVector(0.f, 0.f, 150.f);
		Color = ObjectiveGreen;
	}

	const APawn* LocalPawn = GetOwningPlayerPawn();
	const float Meters = LocalPawn ? FVector::Dist2D(LocalPawn->GetActorLocation(), TargetLocation) / UnitsPerMeter : 0.f;

	const FVector2D ScreenSize = MyGeometry.GetLocalSize();
	FVector2D ScreenPosition;
	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, TargetLocation, ScreenPosition, false);
	const bool bOnScreen = bProjected && ScreenPosition.X >= ObjectiveEdgeMargin && ScreenPosition.Y >= ObjectiveEdgeMargin &&
		ScreenPosition.X <= ScreenSize.X - ObjectiveEdgeMargin && ScreenPosition.Y <= ScreenSize.Y - ObjectiveEdgeMargin;

	FVector2D MarkerPosition = ScreenPosition;
	float Angle = 0.f;
	if (!bOnScreen)
	{
		// Clamp to the screen edge along the direction from the center, arrow pointing outwards.
		const FVector2D Center = ScreenSize * 0.5f;
		FVector2D Direction = bProjected ? (ScreenPosition - Center) : FVector2D(0.f, 1.f);
		Direction = Direction.IsNearlyZero() ? FVector2D(0.f, 1.f) : Direction.GetSafeNormal();
		const FVector2D HalfExtent = Center - FVector2D(ObjectiveEdgeMargin, ObjectiveEdgeMargin);
		const float Scale = FMath::Min(
			FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER ? HalfExtent.X / FMath::Abs(Direction.X) : TNumericLimits<float>::Max(),
			FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER ? HalfExtent.Y / FMath::Abs(Direction.Y) : TNumericLimits<float>::Max());
		MarkerPosition = Center + Direction * Scale;
		// The glyph points down (+Y); rotate it toward Direction.
		Angle = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)) - 90.f;
	}
	else
	{
		// Bounce a little above the target.
		MarkerPosition.Y -= 18.f + 5.f * FMath::Sin(Age * 5.f);
	}

	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
	{
		MarkerSlot->SetPosition(MarkerPosition);
	}
	MarkerArrow->SetRenderTransformAngle(Angle);
	MarkerArrow->SetColorAndOpacity(FSlateColor(Color));
	MarkerDistance->SetText(FText::FromString(FString::Printf(TEXT("%dm"), FMath::RoundToInt(Meters))));
	MarkerDistance->SetVisibility(bOnScreen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URougeObjectiveWidget::RefreshWaves()
{
	const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr;
	if (!RougeGameState || (RougeGameState->GetCurrentWave() <= 0 && RougeGameState->GetWavePhase() != ERougeWavePhase::Break) || RougeGameState->IsRunOver())
	{
		WaveBox->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	WaveBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	const ARougeStation* Station = Cast<ARougeStation>(RougeGameState->GetActiveStation());
	const bool bFinalWave = Station && Station->IsFinal() && Station->GetState() == ERougeStationState::Purifying;

	// Wave
	if (bFinalWave)
	{
		WaveTitle->SetText(FText::FromString(TEXT("FINAL WAVE")));
		WaveDetail->SetText(FText::FromString(FString::Printf(TEXT("%d enemies left"), RougeGameState->GetEnemiesRemaining())));
		WaveDetail->SetColorAndOpacity(FSlateColor(ObjectiveRed));
	}
	else if (RougeGameState->GetWavePhase() == ERougeWavePhase::Fighting)
	{
		WaveTitle->SetText(FText::FromString(FString::Printf(TEXT("WAVE %d"), RougeGameState->GetCurrentWave())));
		WaveDetail->SetText(FText::FromString(FString::Printf(TEXT("%d enemies left"), RougeGameState->GetEnemiesRemaining())));
		WaveDetail->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	else
	{
		WaveTitle->SetText(FText::FromString(FString::Printf(TEXT("WAVE %d"), RougeGameState->GetCurrentWave() + 1)));
		WaveDetail->SetText(FText::FromString(FString::Printf(TEXT("starts in %ds"), FMath::CeilToInt(RougeGameState->GetBreakTimeRemaining()))));
		WaveDetail->SetColorAndOpacity(FSlateColor(ObjectiveOrange));
	}

	// Station side objective
	const int32 Count = RougeGameState->GetStationCount();
	const int32 Purified = RougeGameState->GetStationsPurified();
	FString StationLine;
	FLinearColor StationColor = ObjectiveGreen;
	if (Station)
	{
		const FString Name = Station->IsFinal() ? FString(TEXT("FINAL STATION"))
			: FString::Printf(TEXT("Station %d  (%d/%d)"), Station->GetStationNumber(), Purified + 1, Count);
		if (Station->GetState() == ERougeStationState::Purifying)
		{
			StationLine = FString::Printf(TEXT("%s: purify %d%%"), *Name, FMath::FloorToInt(Station->GetProgress() * 100.f));
		}
		else
		{
			StationLine = FString::Printf(TEXT("%s open - bring the package!"), *Name);
			StationColor = ObjectiveGold;
		}
	}
	else if (Count > 0 || RougeGameState->GetWavesUntilStation() > 0)
	{
		const int32 Waves = FMath::Max(1, RougeGameState->GetWavesUntilStation());
		StationLine = FString::Printf(TEXT("%s in %d wave%s   (%d/%d purified)"),
			Purified >= Count ? TEXT("Final station") : TEXT("Next station"), Waves, Waves == 1 ? TEXT("") : TEXT("s"), Purified, Count);
		StationColor = FLinearColor(0.75f, 0.75f, 0.75f, 1.f);
	}
	WaveStation->SetText(FText::FromString(StationLine));
	WaveStation->SetColorAndOpacity(FSlateColor(StationColor));
	WaveStation->SetVisibility(StationLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void URougeObjectiveWidget::RefreshDamageEdge()
{
	if (!DamageEdge || !DamageEdgeMID)
	{
		return;
	}

	const ARougeCharacter* RougeCharacter = Cast<ARougeCharacter>(GetOwningPlayerPawn());
	const float Flash = RougeCharacter ? RougeCharacter->GetDamageFlash() : 0.f;
	if (Flash <= 0.f)
	{
		DamageEdge->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Light warning: at most ~half opacity, only at the edges.
	DamageEdgeMID->SetScalarParameterValue(TEXT("Strength"), FMath::Clamp(0.15f + Flash * 0.4f, 0.f, 0.55f));
	DamageEdge->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URougeObjectiveWidget::RefreshResult()
{
	const ARougeGameState* RougeGameState = GetWorld() ? GetWorld()->GetGameState<ARougeGameState>() : nullptr;
	if (!RougeGameState || !RougeGameState->IsRunOver())
	{
		ResultBox->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const bool bWon = RougeGameState->GetRunResult() == ERougeRunResult::Won;
	ResultTitle->SetText(FText::FromString(bWon ? TEXT("PACKAGE DELIVERED!") : TEXT("TEAM WIPED")));
	ResultTitle->SetColorAndOpacity(FSlateColor(bWon ? ObjectiveGreen : ObjectiveRed));
	ResultSubtitle->SetText(FText::FromString(FString::Printf(TEXT("Team level %d   -   returning to the lobby..."), RougeGameState->GetTeamLevel())));
	ResultBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	StatusBox->SetVisibility(ESlateVisibility::Collapsed);
}
