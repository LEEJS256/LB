// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_StatRow.h"

#include "Components/TextBlock.h"

void ULB_StatRow::SetStat(const FText& Label, const FText& Value, float Bonus, int32 Decimals)
{
	if (LabelText) LabelText->SetText(Label);
	if (ValueText) ValueText->SetText(Value);

	if (!BonusText)
		return;

	if (FMath::IsNearlyZero(Bonus, 0.001f))
	{
		BonusText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FNumberFormattingOptions Options;
	Options.SetMinimumFractionalDigits(Decimals);
	Options.SetMaximumFractionalDigits(Decimals);
	Options.SetAlwaysSign(true);   // +12 / -5

	BonusText->SetText(FText::Format(NSLOCTEXT("LB", "StatBonus", "({0})"), FText::AsNumber(Bonus, &Options)));
	BonusText->SetColorAndOpacity(Bonus > 0.f ? PositiveColor : NegativeColor);
	BonusText->SetVisibility(ESlateVisibility::HitTestInvisible);
}
