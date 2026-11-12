// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/LB_DA_EquipInfo.h"

void ULB_DA_EquipInfo::GetTooltipLines(TArray<FLB_TooltipLine>& Out) const
{
	for (const FLB_StatBonus& Bonus : BonusStats)
	{
		FLB_TooltipLine Line;
		Line.Label = FText::FromString(Bonus.Attribute.GetName());
		Line.Value = FText::FromString(FString::Printf(TEXT("%+.0f"), Bonus.Value));
		Line.Style = ELB_TooltipLineStyle::Normal;
		Out.Add(Line);
	}
}
