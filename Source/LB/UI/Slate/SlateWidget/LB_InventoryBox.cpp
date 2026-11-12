// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_InventoryBox.h"

#include "../SLB_InventoryBox.h"
#include "Engine/Texture2D.h"
#include "Subsystem/LB_GISubsystem_Data.h"

ULB_InventoryBox::ULB_InventoryBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ItemTooltipClass = FindDefaultTooltipClass();
}


TSharedRef<SLB_SquareBox> ULB_InventoryBox::CreateSquareBox()
{
	return SNew(SLB_InventoryBox)
		.Group(Group)
		.SlotIndex(SlotIndex)
		.CountTextFont(CountTextFont)
		.HotkeyTextFont(HotkeyTextFont)
		.OnSlotDropped(FLB_OnSlotDropped::CreateUObject(this, &ULB_InventoryBox::HandleSlotDropped))
		.OnUse(FLB_OnSlotUse::CreateUObject(this, &ULB_InventoryBox::HandleUse));
}

TSharedPtr<SLB_InventoryBox> ULB_InventoryBox::GetInventoryBox() const
{
	return StaticCastSharedPtr<SLB_InventoryBox>(MySquareBox);
}

void ULB_InventoryBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyState();
}


const ULB_DA_ItemInfo* ULB_InventoryBox::GetCurrentItemInfo() const
{
	if (ItemCode.IsNone())
	{
		return nullptr;
	}

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	return Data ? Data->GetItemInfo(ItemCode) : nullptr;
}

void ULB_InventoryBox::ApplyState()
{
	const TSharedPtr<SLB_InventoryBox> Box = GetInventoryBox();
	if (!Box.IsValid())
	{
		return;
	}

	Box->SetGroup(Group);
	Box->SetSlotIndex(SlotIndex);
	Box->SetHotkeyText(HotkeyText);
	Box->SetInfoText(InfoText);

	if (bEnableTestCount)
	{
		Box->SetTestCount(TestItemCount);
	}
	else
	{
		Box->ClearTestCount();
	}

	if (Icon)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Icon);
		Brush.ImageSize = FVector2D(Icon->GetSizeX(), Icon->GetSizeY());
		Box->SetItem(Brush, Count);
	}
	else
	{
		Box->ClearItem();
	}
}


void ULB_InventoryBox::SetItem(UTexture2D* InIcon, int32 InCount, FName InItemCode)
{
	Icon = InIcon;
	ItemCode = InItemCode;
	Count = InCount;
	ApplyState();
}

void ULB_InventoryBox::ClearItem()
{
	Icon = nullptr;
	Count = 0;
	ItemCode = NAME_None;
	ApplyState();
}

void ULB_InventoryBox::SetInfoText(const FText& InText)
{
	InfoText = InText;
	ApplyState();
}

void ULB_InventoryBox::SetHotkeyText(const FText& InText)
{
	HotkeyText = InText;
	ApplyState();
}

void ULB_InventoryBox::SetSlotIndex(int32 InIndex)
{
	SlotIndex = InIndex;
	ApplyState();
}

void ULB_InventoryBox::SetGroup(FName InGroup)
{
	Group = InGroup;
	ApplyState();
}

FName ULB_InventoryBox::GetGroup()
{
	return Group;
}

void ULB_InventoryBox::HandleSlotDropped(FName SrcGroup, int32 SrcIndex, int32 DstIndex)
{
	OnSlotDropped.Broadcast(SrcGroup, SrcIndex, DstIndex);
}

void ULB_InventoryBox::HandleUse(int32 InSlotIndex)
{
	OnUse.Broadcast(InSlotIndex);
}
