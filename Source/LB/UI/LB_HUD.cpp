// LB_HUD.cpp
#include "LB_HUD.h"
#include "LB_PlayerStatus.h"

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
