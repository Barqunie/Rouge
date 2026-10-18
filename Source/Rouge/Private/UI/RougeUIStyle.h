// Small style helpers shared by the code-built widgets (HUD, augment cards).

#pragma once

#include "CoreMinimal.h"
#include "Augments/RougeAugmentTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

namespace RougeUI
{
	/** Rounded (or circular) box, optionally outlined. */
	inline FSlateBrush MakeRoundedBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent,
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

	/** Default engine font with a thin dark outline so text reads over the game. */
	inline FSlateFontInfo MakeFont(int32 Size, bool bBold = true)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
		return Font;
	}

	const FLinearColor GoldColor(0.78f, 0.62f, 0.3f, 1.f);
	const FLinearColor PanelColor(0.02f, 0.025f, 0.035f, 0.92f);
	const FLinearColor MutedTextColor(0.65f, 0.65f, 0.7f, 1.f);

	/** Card colors: stat blue, skill orange, passive purple. */
	inline FLinearColor GetAugmentCategoryColor(ERougeAugmentCategory Category)
	{
		switch (Category)
		{
		case ERougeAugmentCategory::Skill:
			return FLinearColor(1.f, 0.6f, 0.15f, 1.f);
		case ERougeAugmentCategory::Passive:
			return FLinearColor(0.75f, 0.45f, 1.f, 1.f);
		default:
			return FLinearColor(0.3f, 0.65f, 1.f, 1.f);
		}
	}
}
