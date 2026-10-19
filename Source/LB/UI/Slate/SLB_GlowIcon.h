// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class LB_API SLB_GlowIcon : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLB_GlowIcon)
			: _IconText(FText::FromString(TEXT("♥")))
			  , _IconColor(FLinearColor(1.0f, 0.17f, 0.33f, 1.0f))
			  , _BackgroundColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.7f))
			  , _GlowIntensity(2.0f)
			  , _IconFontSize(14)
			  , _GlowBoxSize(FVector2D(34.0f, 34.0f))
		{
		}

		SLATE_ATTRIBUTE(FText, IconText)
		SLATE_ATTRIBUTE(FSlateColor, IconColor)
		SLATE_ATTRIBUTE(FSlateColor, BackgroundColor)
		SLATE_ATTRIBUTE(float, GlowIntensity)
		SLATE_ARGUMENT(int32, IconFontSize)
		SLATE_ARGUMENT(FVector2D, GlowBoxSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetIconText(TAttribute<FText> InText);
	void SetIconColor(TAttribute<FSlateColor> InColor);
	void SetGlowIntensity(TAttribute<float> InIntensity);
	void SetBoxSize(FVector2D InSize);
	void SetIconFontSize(int32 InSize);
	
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	TAttribute<FText> IconText;
	TAttribute<FSlateColor> IconColor;
	TAttribute<FSlateColor> BackgroundColor;
	TAttribute<float> GlowIntensity;
	FVector2D GlowBoxSize;

	TSharedPtr<class STextBlock> TextBlock;
	const FSlateBrush* WhiteBrush = nullptr;
};
