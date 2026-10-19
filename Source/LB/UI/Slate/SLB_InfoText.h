// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class LB_API SLB_InfoText : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLB_InfoText)
			: _IconText(FText::FromString(TEXT("◆")))
			  , _LabelText(FText::FromString(TEXT("GOLD")))
			  , _ValueText(FText::FromString(TEXT("0")))
			  , _IconColor(FLinearColor(1.0f, 0.82f, 0.29f, 1.0f))
			  , _LabelColor(FLinearColor(0.7f, 0.7f, 0.7f, 0.9f))
			  , _LabelFontSize(8)
			  , _ValueFontSize(12)
			  , _IconBoxSize(28.0f)
			  , _BoxColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.7f))
			  , _BoxPadding(FMargin(8.0f, 4.0f))
			  , _IconFontSize(14)
		{
		}

		SLATE_ATTRIBUTE(FText, IconText)
		SLATE_ATTRIBUTE(FText, LabelText)
		SLATE_ATTRIBUTE(FText, ValueText)
		SLATE_ATTRIBUTE(FSlateColor, IconColor)
		SLATE_ARGUMENT(int32, LabelFontSize)
		SLATE_ARGUMENT(int32, ValueFontSize)
		SLATE_ARGUMENT(float, IconBoxSize)
		SLATE_ATTRIBUTE(FSlateColor, LabelColor)
		SLATE_ATTRIBUTE(FSlateColor, ValueColor)
		SLATE_ATTRIBUTE(FSlateColor, BoxColor)
		SLATE_ARGUMENT(FMargin, BoxPadding)
		SLATE_ARGUMENT(int32, IconFontSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetValue(FText InValue);
	void SetLabelText(FText InLabel);
	void SetLabelFontSize(int32 InSize);
	void SetValueFontSize(int32 InSize);
	void SetIconBoxSize(float InSize);
	void SetLabelColor(FSlateColor InColor);
	void SetValueColor(FSlateColor InColor);
	void SetBoxColor(FSlateColor InColor);
	void SetBoxPadding(FMargin InPadding);
	void SetIconFontSize(int32 InSize);

private:
	TSharedPtr<class SLB_GlowIcon> Icon;
	TSharedPtr<class STextBlock> ValueTextBlock;
	TSharedPtr<class STextBlock> LabelTextBlock;
	TSharedPtr<class SBorder> TextBoxBorder;
};
