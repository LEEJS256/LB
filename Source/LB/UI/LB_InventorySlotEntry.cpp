// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_InventorySlotEntry.h"

#include "LB_InventoryItemObject.h"
#include "UI/Slate/SlateWidget/LB_InventoryBox.h"
#include "Component/LB_InventoryComponent.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerState.h"

void ULB_InventorySlotEntry::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 엔트리는 풀링되어 계속 재사용되므로 바인딩은 생성 시 한 번만
	if (InventoryBox)
	{
		InventoryBox->OnUse.AddDynamic(this, &ULB_InventorySlotEntry::HandleUse);
		InventoryBox->OnSlotDropped.AddDynamic(this, &ULB_InventorySlotEntry::HandleSlotDropped);
	}
}

void ULB_InventorySlotEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	Item = Cast<ULB_InventoryItemObject>(ListItemObject);
	if (!InventoryBox) return;

	if (!Item)
	{
		ClearView();
		return;
	}

	// 풀링 엔트리라 슬롯 번호는 매번 갱신해야 드롭 대상 인덱스가 맞는다
	InventoryBox->SetSlotIndex(Item->GetSlotIndex());
	InventoryBox->SetSelected(Item->IsEquipped());

	if (Item->IsEmpty())
	{
		InventoryBox->ClearItem();
		InventoryBox->SetInfoText(FText::GetEmpty());
		return;
	}

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	const ULB_DA_ItemInfo* Info = Data ? Data->GetItemInfo(Item->GetItemCode()) : nullptr;
	if (!Info)
	{
		InventoryBox->ClearItem();
		InventoryBox->SetInfoText(FText::GetEmpty());
		return;
	}

	// 아이콘은 Soft라 여기서 동기 로드. 칸당 한 번이고 로드된 뒤엔 캐시라 가볍다.
	// 첫 로드 끊김이 거슬리면 나중에 비동기 로드로 바꾼다.
	UTexture2D* Icon = Info->Icon.LoadSynchronous();
	InventoryBox->SetItem(Icon, Item->GetStackCount());
	InventoryBox->SetInfoText(
		FText::Format(FText::FromString(TEXT("{0}\n{1}")), Info->DisplayName, Info->Description));
}

void ULB_InventorySlotEntry::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();

	Item = nullptr;
	ClearView();
}

void ULB_InventorySlotEntry::NativePreConstruct()
 {
 	Super::NativePreConstruct();
	ApplySlotColors();   
 }


void ULB_InventorySlotEntry::ClearView()
{
	if (!InventoryBox) return;

	InventoryBox->ClearItem();
	InventoryBox->SetInfoText(FText::GetEmpty());
	InventoryBox->SetSelected(false);
	InventoryBox->SetSlotIndex(INDEX_NONE);
}

void ULB_InventorySlotEntry::ApplySlotColors()
{
	if (!InventoryBox) return;

	FLB_SquareBoxStyle S = InventoryBox->Style;
	S.BorderColor = FLinearColor::FromSRGBColor(BorderColor);
	S.NormalColor = FLinearColor::FromSRGBColor(FillColor);
	InventoryBox->SetStyle(S);
}

void ULB_InventorySlotEntry::HandleUse(int32 InSlotIndex)
{
	if (!Item || Item->IsEmpty()) return;

	if (ULB_InventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->Server_UseItem(Item->GetItemInstanceId());
	}
}

void ULB_InventorySlotEntry::HandleSlotDropped(FName SrcGroup, int32 SrcIndex, int32 DstIndex)
{
	// TODO(8번): 인벤토리끼리 이동이면 Server_MoveItem(SrcIndex, DstIndex), 퀵슬롯이면 등록 처리
	UE_LOG(LogTemp, Log, TEXT("[Inventory] Drop %s[%d] -> %d"), *SrcGroup.ToString(), SrcIndex, DstIndex);
}

ULB_InventoryComponent* ULB_InventorySlotEntry::GetInventoryComponent() const
{
	// 인벤토리 컴포넌트는 PlayerState에 붙어 있다
	const APlayerState* PS = GetOwningPlayerState();
	return PS ? PS->FindComponentByClass<ULB_InventoryComponent>() : nullptr;
}