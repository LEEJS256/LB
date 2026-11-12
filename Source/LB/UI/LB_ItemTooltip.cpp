// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_ItemTooltip.h"

#include "UI/LB_TooltipLineRow.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "Data/LB_DA_ItemInfo.h"


void ULB_ItemTooltip::SetItem(const ULB_DA_ItemInfo* Info)
{
	if (!Info)
		return;

	NameText->SetText(Info->DisplayName);
	SubNameText->SetText(Info->SubName);
	DescText->SetText(Info->Description);

	// 가격이 0이면(재화 등) 표시하지 않는다
	const int32 SellPrice = Info->GetSellPrice();
	SellPriceText->SetText(FText::Format(NSLOCTEXT("LB", "PriceGold", "{0} Gold"), FText::AsNumber(SellPrice)));
	SellPriceText->SetVisibility(SellPrice > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (const FLinearColor* Color = GradeColors.Find(Info->ItemGradeTag))
	{
		GradeBorder->SetBrushColor(*Color);
		NameText->SetColorAndOpacity(*Color);
	}

	// 아이콘은 Soft라 이미 로드돼 있으면 바로, 아니면 동기 로드(슬롯에서 이미 써서 보통 로드됨)
	if (UTexture2D* Tex = Info->Icon.LoadSynchronous())
	{
		IconImage->SetBrushFromTexture(Tex);
	}

	// 타입별 효과 줄 (DA가 GetTooltipLines에서 채워 준다)
	// 툴팁 위젯은 재사용되므로 매번 비우고 다시 만든다
	LineContainer->ClearChildren();

	TArray<FLB_TooltipLine> Lines;
	Info->GetTooltipLines(Lines);

	if (LineRowClass)
	{
		for (const FLB_TooltipLine& Line : Lines)
		{
			ULB_TooltipLineRow* Row = CreateWidget<ULB_TooltipLineRow>(GetOwningPlayer(), LineRowClass);
			if (!Row)
				continue;

			const FLinearColor* StyleColor = LineColors.Find(Line.Style);
			Row->SetLine(Line, StyleColor ? *StyleColor : FLinearColor::White);
			LineContainer->AddChild(Row);
		}
	}
	else if (Lines.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Tooltip] LineRowClass가 지정되지 않아 효과 줄을 표시할 수 없습니다."));
	}

	LineContainer->SetVisibility(LineContainer->GetChildrenCount() > 0
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}
