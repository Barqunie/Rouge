// Package objective HUD: status line, a marker / edge arrow to where to go, and the end of run banner.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RougeObjectiveWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/**
 * Built in code, click-through. Reads the package, the delivery zone and the run result every frame (local only).
 * The marker points at the package, or at the delivery zone while a player carries it. Off screen it sticks to
 * the screen edge as an arrow with the distance.
 */
UCLASS()
class ROUGE_API URougeObjectiveWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayout();
	void RefreshStatus();
	void RefreshMarker(const FGeometry& MyGeometry);
	void RefreshResult();

	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = true);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> StatusBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> Marker;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MarkerArrow;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MarkerDistance;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ResultBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultSubtitle;

	float Age = 0.f;
};
