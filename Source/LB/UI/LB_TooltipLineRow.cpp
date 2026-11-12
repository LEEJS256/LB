// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_TooltipLineRow.h"

#include "Components/TextBlock.h"

void ULB_TooltipLineRow::SetLine(const FLB_TooltipLine& Line, const FLinearColor& Color)
{
	if (LabelText)
	{
		LabelText->SetText(Line.Label);
	}

	if (ValueText)
	{
		ValueText->SetText(Line.Value);
		ValueText->SetColorAndOpacity(FSlateColor(Color));
	}
}
