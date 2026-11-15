#include "LB_EquipSlot.h"

#include "UI/Slate/SLB_EquipSlot.h"
#include "Engine/Texture2D.h"
#include "Subsystem/LB_GISubsystem_Data.h"

namespace
{
	FSlateBrush MakeIconBrush(UTexture2D* InTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(InTexture);
		Brush.ImageSize = FVector2D(InTexture->GetSizeX(), InTexture->GetSizeY());
		return Brush;
	}
}

ULB_EquipSlot::ULB_EquipSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ItemTooltipClass = FindDefaultTooltipClass();
}

TSharedRef<SLB_SquareBox> ULB_EquipSlot::CreateSquareBox()
{
	return SNew(SLB_EquipSlot)
		.BoxSize(Style.BoxSize)
		.SlotKey(SlotKey)
		.Group(Group)
		.SlotIndex(SlotIndex)
		.OnEquipDropped(FLB_OnEquipSlotDropped::CreateUObject(this, &ULB_EquipSlot::HandleEquipDropped))
		.OnUnequip(FLB_OnEquipSlotUnequip::CreateUObject(this, &ULB_EquipSlot::HandleUnequip))
		.CanAcceptDrop(FLB_CanAcceptEquipDrop::CreateUObject(this, &ULB_EquipSlot::HandleCanAccept));
}

TSharedPtr<SLB_EquipSlot> ULB_EquipSlot::GetEquipSlot() const
{
	return StaticCastSharedPtr<SLB_EquipSlot>(MySquareBox);
}

void ULB_EquipSlot::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyState();
}

const ULB_DA_ItemInfo* ULB_EquipSlot::GetCurrentItemInfo() const
{
	if (ItemCode.IsNone())
	{
		return nullptr;
	}

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	return Data ? Data->GetItemInfo(ItemCode) : nullptr;
}

void ULB_EquipSlot::ApplyState()
{
	const TSharedPtr<SLB_EquipSlot> Box = GetEquipSlot();
	if (!Box.IsValid())
	{
		return;
	}

	Box->SetSlotKey(SlotKey);
	Box->SetGroup(Group);
	Box->SetSlotIndex(SlotIndex);

	if (EmptyIcon)
	{
		Box->SetEmptyBrush(MakeIconBrush(EmptyIcon));
	}
	else
	{
		Box->ClearEmptyBrush();
	}

	if (Icon)
	{
		Box->SetItem(MakeIconBrush(Icon));
	}
	else
	{
		Box->ClearItem();
	}
}

void ULB_EquipSlot::SetItem(UTexture2D* InIcon, FName InItemCode)
{
	Icon = InIcon;
	ItemCode = InItemCode;
	ApplyState();
}

void ULB_EquipSlot::ClearItem()
{
	Icon = nullptr;
	ItemCode = NAME_None;
	ApplyState();
}

void ULB_EquipSlot::SetEmptyIcon(UTexture2D* InIcon)
{
	EmptyIcon = InIcon;
	ApplyState();
}

void ULB_EquipSlot::HandleEquipDropped(const FLB_EquipSlotKey& DstKey, FName SrcGroup, int32 SrcIndex)
{
	OnEquipDropped.Broadcast(DstKey, SrcGroup, SrcIndex);
}

void ULB_EquipSlot::HandleUnequip(const FLB_EquipSlotKey& Key)
{
	OnUnequip.Broadcast(Key);
}

bool ULB_EquipSlot::HandleCanAccept(const FLB_EquipSlotKey& DstKey, FName SrcGroup, int32 SrcIndex) const
{
	return CanAcceptDrop.IsBound() ? CanAcceptDrop.Execute(DstKey, SrcGroup, SrcIndex) : true;
}
