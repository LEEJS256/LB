// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_SkillSlotEntry.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Data/LB_DA_SkillInfo.h"
#include "PaperSprite.h"
#include "UI/SlateWidget/LB_InventoryBox.h"

void ULB_SkillSlotEntry::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 카드 빈 곳 클릭도 받으려면 히트 테스트가 돼야 한다
	SetVisibility(ESlateVisibility::Visible);

	if (SkillIcon)
	{
		SkillIcon->SetGroup(TEXT("Skill"));
	}
}

void ULB_SkillSlotEntry::SetSkill(int32 InIndex, FName InSkillCode, const ULB_DA_SkillInfo* Info, int32 Level)
{
	SkillCode = InSkillCode;

	if (SkillIcon)
	{
		SkillIcon->SetSlotIndex(InIndex);
		UPaperSprite* Icon = Info ? Info->Icon.LoadSynchronous() : nullptr;
		if (Icon)
		{
			// 수량 0 = 숫자 안 띄움. ItemCode 자리에 스킬 코드 (아이템 툴팁은 못 찾아서 안 뜸)
			SkillIcon->SetItem(Icon, 0, InSkillCode);
		}
		else
		{
			SkillIcon->ClearItem();
		}
	}

	if (NameText)
	{
		NameText->SetText(Info ? Info->DisplayName : FText::FromName(InSkillCode));
	}

	const int32 MaxLevel = Info ? Info->MaxLevel : 1;
	if (LevelText)
	{
		FText LevelStr;
		if (Level <= 0)
			LevelStr = FText::Format(NSLOCTEXT("LB", "SkillLv_None", "미습득 (Lv.0 / {0})"), MaxLevel);
		else if (Level >= MaxLevel)
			LevelStr = FText::Format(NSLOCTEXT("LB", "SkillLv_Max", "Lv.{0} / {0} (MAX)"), MaxLevel);
		else
			LevelStr = FText::Format(NSLOCTEXT("LB", "SkillLv", "Lv.{0} / {1}"), Level, MaxLevel);
		LevelText->SetText(LevelStr);
	}

	SetRenderOpacity(Level > 0 ? 1.f : UnlearnedOpacity);
}

void ULB_SkillSlotEntry::SetSelected(bool bInSelected)
{
	if (SkillIcon)
	{
		SkillIcon->SetSelected(bInSelected);
	}
	if (CardBorder)
	{
		CardBorder->SetBrushColor(bInSelected ? SelectedColor : NormalColor);
	}
}

FReply ULB_SkillSlotEntry::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 아이콘은 마우스 다운을 드래그 감지에 쓰니까, 선택은 마우스 업으로 받는다
	// (드래그로 끝난 경우엔 드롭 대상 쪽에서 업이 처리돼서 여기로 안 옴)
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !SkillCode.IsNone())
	{
		OnClicked.Broadcast(SkillCode);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
