// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/LB_DA_ConsumableInfo.h"

void ULB_DA_ConsumableInfo::GetTooltipLines(TArray<FLB_TooltipLine>& Out) const
{
	for (const FLB_StatBonus& Effect : Effects)
	{
		FLB_TooltipLine Line;
		Line.Label = FText::FromString(Effect.Attribute.GetName());
		Line.Value = FText::FromString(FString::Printf(TEXT("%+.0f"), Effect.Value));
		Line.Style = ELB_TooltipLineStyle::Highlight;
		Out.Add(Line);
	}

	// 기존 에셋 호환: Effects를 안 채웠으면 HealAmount를 HP 회복 줄로 보여준다
	if (Effects.IsEmpty() && HealAmount > 0.f)
	{
		FLB_TooltipLine Line;
		Line.Label = NSLOCTEXT("LB", "HealLabel", "HP");
		Line.Value = FText::Format(NSLOCTEXT("LB", "HealValue", "{0} 회복"), FText::AsNumber(HealAmount));
		Line.Style = ELB_TooltipLineStyle::Highlight;
		Out.Add(Line);
	}
}
