#include "UI/RougeDamageNumbersWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "UI/RougeUIStyle.h"

namespace
{
	// Kept small on purpose: readable, but the fight stays visible.
	constexpr int32 MaxNumbers = 60;
	constexpr float RisePixels = 38.f;
	constexpr float FadeStart = 0.55f;
	constexpr float PopTime = 0.12f;

	const FLinearColor DealtColor(1.f, 0.96f, 0.88f, 1.f);
	const FLinearColor CriticalColor(1.f, 0.62f, 0.12f, 1.f);
	const FLinearColor TakenColor(1.f, 0.3f, 0.25f, 1.f);
}

TSharedRef<SWidget> URougeDamageNumbersWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		WidgetTree->RootWidget = Canvas;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

UTextBlock* URougeDamageNumbersWidget::AcquireText()
{
	if (FreeTexts.Num() > 0)
	{
		return FreeTexts.Pop(EAllowShrinking::No);
	}

	if (Active.Num() >= MaxNumbers)
	{
		// Busy fight: reuse the oldest number.
		int32 Oldest = 0;
		for (int32 Index = 1; Index < Active.Num(); ++Index)
		{
			if (Active[Index].Age > Active[Oldest].Age)
			{
				Oldest = Index;
			}
		}
		Release(Oldest);
		return FreeTexts.Pop(EAllowShrinking::No);
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Text))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.f));
	}
	return Text;
}

void URougeDamageNumbersWidget::Release(int32 ActiveIndex)
{
	UTextBlock* Text = Active[ActiveIndex].Text;
	Text->SetVisibility(ESlateVisibility::Collapsed);
	FreeTexts.Add(Text);
	Active.RemoveAtSwap(ActiveIndex, EAllowShrinking::No);
}

void URougeDamageNumbersWidget::AddNumber(const FVector& WorldLocation, int32 Amount, ERougeDamageNumberType Type)
{
	// Make sure the canvas exists even before the first paint.
	TakeWidget();
	if (!Canvas || Amount <= 0)
	{
		return;
	}

	UTextBlock* Text = AcquireText();

	FActiveNumber& Number = Active.AddDefaulted_GetRef();
	Number.Text = Text;
	Number.WorldLocation = WorldLocation;
	Number.Drift = FVector2D(FMath::FRandRange(-18.f, 18.f), 0.f);

	switch (Type)
	{
	case ERougeDamageNumberType::Critical:
		Text->SetText(FText::FromString(FString::Printf(TEXT("CRIT %d"), Amount)));
		Text->SetFont(RougeUI::MakeFont(14));
		Text->SetColorAndOpacity(FSlateColor(CriticalColor));
		Number.Lifetime = 0.9f;
		Number.StartScale = 1.45f;
		break;
	case ERougeDamageNumberType::Taken:
		Text->SetText(FText::FromString(FString::Printf(TEXT("-%d"), Amount)));
		Text->SetFont(RougeUI::MakeFont(12));
		Text->SetColorAndOpacity(FSlateColor(TakenColor));
		Number.Lifetime = 0.8f;
		Number.StartScale = 1.15f;
		break;
	default:
		Text->SetText(FText::AsNumber(Amount));
		Text->SetFont(RougeUI::MakeFont(12));
		Text->SetColorAndOpacity(FSlateColor(DealtColor));
		Number.Lifetime = 0.7f;
		Number.StartScale = 1.15f;
		break;
	}

	// Positioned on the next tick.
	Text->SetVisibility(ESlateVisibility::Hidden);
	Text->SetRenderOpacity(1.f);
}

void URougeDamageNumbersWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PlayerController = GetOwningPlayer();

	for (int32 Index = Active.Num() - 1; Index >= 0; --Index)
	{
		FActiveNumber& Number = Active[Index];
		Number.Age += InDeltaTime;
		if (Number.Age >= Number.Lifetime || !Number.Text)
		{
			Release(Index);
			continue;
		}

		FVector2D ScreenPosition;
		if (!PlayerController || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, Number.WorldLocation, ScreenPosition, false))
		{
			Number.Text->SetVisibility(ESlateVisibility::Hidden);
			continue;
		}

		// Rises quickly then settles, drifts a little sideways, fades at the end.
		const float Alpha = Number.Age / Number.Lifetime;
		const float Rise = RisePixels * (1.f - FMath::Square(1.f - Alpha));
		ScreenPosition += Number.Drift * Alpha - FVector2D(0.f, Rise);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Number.Text->Slot))
		{
			CanvasSlot->SetPosition(ScreenPosition);
		}

		const float Opacity = Alpha < FadeStart ? 1.f : 1.f - (Alpha - FadeStart) / (1.f - FadeStart);
		const float Scale = FMath::Lerp(Number.StartScale, 1.f, FMath::Clamp(Number.Age / PopTime, 0.f, 1.f));
		Number.Text->SetRenderOpacity(Opacity);
		Number.Text->SetRenderScale(FVector2D(Scale));
		Number.Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
