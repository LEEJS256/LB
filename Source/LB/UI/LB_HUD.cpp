// LB_HUD.cpp
#include "LB_HUD.h"

#include "LB_InventoryPanel.h"
#include "LB_PlayerStatus.h"
#include "Controller/LB_PlayerController.h"

ULB_PlayerStatus* ULB_HUD::GetPlayerStatus(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return PlayerStatus_P1;
	case 1: return PlayerStatus_P2;
	default: return nullptr;
	}
}

ULB_PlayerStatus* ULB_HUD::GetLocalPlayerStatus() const
{
	return PlayerStatus_P1;
}

void ULB_HUD::ToggleInventory()
{
	if (!BP_Inventory)
		return;
	BP_Inventory->SetVisibility(IsInventoryVisible()
		                            ? ESlateVisibility::Collapsed
		                            : ESlateVisibility::Visible);
}

bool ULB_HUD::IsInventoryVisible() const
{
	return BP_Inventory && BP_Inventory->IsVisible();
}

void ULB_HUD::SetInventoryVisible(bool bVisible)
{
	if (BP_Inventory)
		BP_Inventory->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULB_HUD::NativeConstruct()
{
	Super::NativeConstruct();
	if (BP_Inventory)
	{
		BP_Inventory->SetVisibility(ESlateVisibility::Collapsed);
		BP_Inventory->OnCloseRequested.AddUniqueDynamic(this, &ULB_HUD::HandleInventoryCloseRequested);
	}
}

void ULB_HUD::HandleInventoryCloseRequested()
{
	if (ALB_PlayerController* PC = GetOwningPlayer<ALB_PlayerController>())
		PC->CloseInventory();
}
