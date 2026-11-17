// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_ItemTooltip.h"

#include "UI/LB_TooltipLineRow.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "PaperSprite.h"
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
	if (UPaperSprite* Sprite = Info->Icon.LoadSynchronous())
	{
		// PaperSprite는 아틀라스 인터페이스라서 브러시 리소스로 넣으면 해당 영역만 그려진다
		// 크기는 디자이너에서 정한 IconImage 브러시 크기를 유지
		IconImage->SetBrushResourceObject(Sprite);
		IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		// 툴팁 위젯은 재사용돼서, 아이콘 없는 아이템이면 이전 아이콘이 남는다
		IconImage->SetBrushResourceObject(nullptr);
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
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
