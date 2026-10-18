// Floating damage numbers for the local player (built in code like the HUD).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RougeDamageNumbersWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

UENUM()
enum class ERougeDamageNumberType : uint8
{
	/** Damage you dealt. */
	Dealt,
	/** Damage you dealt with a critical hit. */
	Critical,
	/** Damage you took. */
	Taken,
	/** Health restored (support heal). */
	Heal,
	/** Armor given (support buff). */
	Armor
};

/**
 * Full screen, click-through layer that draws small numbers over world locations; they rise and fade out.
 * Filled by ARougePlayerController::ClientShowDamageNumber, so each player only sees their own numbers.
 */
UCLASS()
class ROUGE_API URougeDamageNumbersWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddNumber(const FVector& WorldLocation, int32 Amount, ERougeDamageNumberType Type);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FActiveNumber
	{
		/** Owned by Canvas (texts stay attached and are reused). */
		TObjectPtr<UTextBlock> Text;
		FVector WorldLocation = FVector::ZeroVector;
		FVector2D Drift = FVector2D::ZeroVector;
		float Age = 0.f;
		float Lifetime = 1.f;
		float StartScale = 1.f;
	};

	UTextBlock* AcquireText();
	void Release(int32 ActiveIndex);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> FreeTexts;

	TArray<FActiveNumber> Active;
};
