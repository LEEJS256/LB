// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Core/Public/Misc/Attribute.h"
#include "Runtime/SlateCore/Public/Widgets/DeclarativeSyntaxSupport.h"
#include "Runtime/SlateCore/Public/Widgets/SCompoundWidget.h"


/**
 * 
 */
class LB_API SLB_NeonProgressbar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLB_NeonProgressbar)
			: _Percent(1.0f)
			  , _BarColor(FLinearColor(1.0f, 0.17f, 0.33f, 1.0f)) // Neon Red (#ff2d55)
			  , _TrackColor(FLinearColor(0.05f, 0.06f, 0.09f, 0.8f))
			  , _GlowIntensity(1.5f)
		      ,_SlantOffset(6.0f)
		,_IsBarVisible(true)
		{
		}

		SLATE_ATTRIBUTE(float, Percent)
		SLATE_ATTRIBUTE(FSlateColor, BarColor)
		SLATE_ATTRIBUTE(FSlateColor, TrackColor)
		SLATE_ATTRIBUTE(float, GlowIntensity)
		SLATE_ATTRIBUTE(float, SlantOffset)
		SLATE_ATTRIBUTE(bool, IsBarVisible)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void SetPercent(TAttribute<float> InPercent);
	void SetBarColor(TAttribute<FSlateColor> InBarColor);
	void SetTrackColor(TAttribute<FSlateColor> InTrackColor);
	void SetGlowIntensity(TAttribute<float> InGlowIntensity);
	void SetSlantOffset(TAttribute<float> InSlantOffset);
	void SetIsBarVisible(TAttribute<bool> InIsBarVisible); 

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	TAttribute<float> Percent;
	TAttribute<FSlateColor> BarColor;
	TAttribute<FSlateColor> TrackColor; //빈테두리
	TAttribute<float> GlowIntensity; // 발광정도
	TAttribute<float> SlantOffset; //기울기 
	TAttribute<bool> IsBarVisible; // 프로그래스바 visible
	const FSlateBrush* WhiteBrush = nullptr;
};
