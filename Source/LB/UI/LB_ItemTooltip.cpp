// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_ItemTooltip.h"

#include "UI/LB_TooltipLineRow.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameplayEffect.h"                 
#include "Data/LB_DA_EquipInfo.h"           
#include "Data/LB_DA_ConsumableInfo.h"      
#include "Components/VerticalBox.h"
#include "PaperSprite.h"
#include "Data/LB_DA_ItemInfo.h"

ULB_ItemTooltip::ULB_ItemTooltip(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SlotNames.Add(TAG_Equip_Weapon,       NSLOCTEXT("LB", "Slot_Weapon", "무기"));
	SlotNames.Add(TAG_Equip_Armor_Head,   NSLOCTEXT("LB", "Slot_Head", "머리"));
	SlotNames.Add(TAG_Equip_Armor_Top,    NSLOCTEXT("LB", "Slot_Top", "상의"));
	SlotNames.Add(TAG_Equip_Armor_Bottom, NSLOCTEXT("LB", "Slot_Bottom", "하의"));
	SlotNames.Add(TAG_Equip_Armor_Hands,  NSLOCTEXT("LB", "Slot_Hands", "장갑"));
	SlotNames.Add(TAG_Equip_Armor_Feet,   NSLOCTEXT("LB", "Slot_Feet", "신발"));
	SlotNames.Add(TAG_Equip_Accessory,    NSLOCTEXT("LB", "Slot_Accessory", "악세사리"));
}

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

	// 부위
	const FText SlotName = GetSlotName(Info);
	if (SlotText)
	{
		SlotText->SetText(SlotName);
		SlotText->SetVisibility(SlotName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	else if (!SlotName.IsEmpty())
	{
		FLB_TooltipLine SlotLine;
		SlotLine.Label = NSLOCTEXT("LB", "SlotLabel", "부위");
		SlotLine.Value = SlotName;
		Lines.Add(SlotLine);
	}

	Info->GetTooltipLines(Lines);   // DA가 직접 채우는 줄 (기존)
	AppendEffectLines(Info, Lines); // Granted_GE에서 읽은 효과 줄

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

FText ULB_ItemTooltip::GetSlotName(const ULB_DA_ItemInfo* Info) const
{
	const ULB_DA_EquipInfo* Equip = Cast<ULB_DA_EquipInfo>(Info);
	if (!Equip)
		return FText::GetEmpty();

	FGameplayTag Tag = Equip->EquipSlotTag;
	if (!Tag.IsValid() && Equip->WeaponTag.IsValid())
		Tag = TAG_Equip_Weapon;   // EquipSlotTag 없는 예전 무기 DA

	if (const FText* Name = SlotNames.Find(Tag))
		return *Name;

	return Tag.IsValid() ? FText::FromName(Tag.GetTagName()) : FText::GetEmpty();
}

void ULB_ItemTooltip::AppendEffectLines(const ULB_DA_ItemInfo* Info, TArray<FLB_TooltipLine>& Out) const
{
	const bool bConsumable = Info->IsA<ULB_DA_ConsumableInfo>();

	for (const TSubclassOf<UGameplayEffect>& GEClass : Info->Granted_GE)
	{
		const UGameplayEffect* GE = GEClass ? GEClass->GetDefaultObject<UGameplayEffect>() : nullptr;
		if (!GE)
			continue;

		// 도트힐 같은 지속형이면 "(1초마다, 5초간)" 붙이기
		const float Period = GE->Period.GetValueAtLevel(1.f);
		float Duration = 0.f;
		const bool bHasDuration = GE->DurationPolicy == EGameplayEffectDurationType::HasDuration
			&& GE->DurationMagnitude.GetStaticMagnitudeIfPossible(1.f, Duration);

		for (const FGameplayModifierInfo& Mod : GE->Modifiers)
		{
			float Mag = 0.f;
			// Scalable Float만 미리 값을 알 수 있다 (SetByCaller/MMC는 적용 시점에 정해져서 건너뜀)
			if (!Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Mag))
				continue;

			FString ValueStr;
			switch (Mod.ModifierOp)
			{
			case EGameplayModOp::AddBase:
			case EGameplayModOp::AddFinal:
				ValueStr = FString::Printf(TEXT("%+.0f"), Mag);
				break;
			case EGameplayModOp::MultiplyAdditive:
			case EGameplayModOp::MultiplyCompound:
				ValueStr = FString::Printf(TEXT("%+.0f%%"), (Mag - 1.f) * 100.f);   // 1.2 → +20%
				break;
			case EGameplayModOp::Override:
				ValueStr = FString::Printf(TEXT("= %.0f"), Mag);
				break;
			default:
				continue;
			}

			if (bHasDuration && Period > 0.f)
				ValueStr += FString::Printf(TEXT(" (%.0f초마다, %.0f초간)"), Period, Duration);
			else if (bHasDuration)
				ValueStr += FString::Printf(TEXT(" (%.0f초간)"), Duration);

			const FString AttrName = Mod.Attribute.GetName();
			const FText* Pretty = AttributeNames.Find(AttrName);

			FLB_TooltipLine Line;
			Line.Label = Pretty ? *Pretty : FText::FromString(AttrName);
			Line.Value = FText::FromString(ValueStr);
			Line.Style = bConsumable ? ELB_TooltipLineStyle::Highlight : ELB_TooltipLineStyle::Bonus;
			Out.Add(Line);
		}
	}
}
