#include "LB_QuickSlot.h"

#include "../SLB_QuickSlot.h"
#include "Component/LB_InventoryComponent.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Engine/Texture2D.h"
#include "Subsystem/LB_GISubsystem_Data.h"


ULB_QuickSlot::ULB_QuickSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ItemTooltipClass = FindDefaultTooltipClass();
}

TSharedRef<SLB_SquareBox> ULB_QuickSlot::CreateSquareBox()
{
	return SNew(SLB_QuickSlot)
		.Group(LBSlotGroup::FromQuickSlotType(SlotType))
		.SlotIndex(SlotIndex)
		.HotkeyText(HotkeyText)
		.HotkeyStyle(HotkeyStyle)
		.CountStyle(CountStyle)
		.OnSlotDropped(FLB_OnSlotDropped::CreateUObject(this, &ULB_QuickSlot::HandleSlotDropped))
		.OnClearRequested(FLB_OnQuickSlotClear::CreateUObject(this, &ULB_QuickSlot::HandleClearRequested));
}

TSharedPtr<SLB_QuickSlot> ULB_QuickSlot::GetQuickSlotBox() const
{
	return StaticCastSharedPtr<SLB_QuickSlot>(MySquareBox);
}

void ULB_QuickSlot::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyState();
}

const ULB_DA_ItemInfo* ULB_QuickSlot::GetCurrentItemInfo() const
{
	if (SlotType != ELB_QuickSlotType::Item || !Component.IsValid())
	{
		return nullptr;
	}

	const FLB_QuickSlotEntry Entry = Component->GetEntry(SlotType, SlotIndex);
	if (Entry.ItemCode.IsNone())
	{
		return nullptr;
	}

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	return Data ? Data->GetItemInfo(Entry.ItemCode) : nullptr;
}


void ULB_QuickSlot::Setup(ELB_QuickSlotType InType, int32 InIndex, const FText& InHotkeyText)
{
	SlotType = InType;
	SlotIndex = InIndex;
	HotkeyText = InHotkeyText;
	ApplyState();
}

void ULB_QuickSlot::BindToComponent(ULB_QuickSlotComponent* InComponent)
{
	if (Component.IsValid())
	{
		Component->OnQuickSlotUpdated.RemoveDynamic(this, &ULB_QuickSlot::HandleQuickSlotUpdated);
		Component->OnSlotChanged.RemoveDynamic(this, &ULB_QuickSlot::HandleSlotChanged);
	}

	Component = InComponent;

	if (Component.IsValid())
	{
		Component->OnQuickSlotUpdated.AddUniqueDynamic(this, &ULB_QuickSlot::HandleQuickSlotUpdated);
		Component->OnSlotChanged.AddUniqueDynamic(this, &ULB_QuickSlot::HandleSlotChanged);

		// 바인딩 직후 초기 상태 갱신
		RefreshFromComponent();
	}
}

void ULB_QuickSlot::SetTextStyles(const FLB_QuickSlotTextStyle& InHotkeyStyle,
                                  const FLB_QuickSlotTextStyle& InCountStyle)
{
	HotkeyStyle = InHotkeyStyle;
	CountStyle = InCountStyle;
	ApplyState();
}

void ULB_QuickSlot::SetCooldownPercent(float InPercent)
{
	CooldownPercent = InPercent;
	if (const TSharedPtr<SLB_QuickSlot> Box = GetQuickSlotBox())
	{
		Box->SetCooldownPercent(InPercent);
	}
}

void ULB_QuickSlot::HandleSlotChanged(ELB_QuickSlotType Type, int32 Index)
{
	if (Type == SlotType && Index == SlotIndex)
	{
		RefreshFromComponent();
	}
}

void ULB_QuickSlot::HandleQuickSlotUpdated(ELB_QuickSlotType InSlotType, int32 InSlotIndex, FName NewItemCode)
{
	if (SlotType == InSlotType && SlotIndex == InSlotIndex)
	{
		// TODO: NewItemCode에 맞춰 아이콘 텍스처 및 수량 텍스트 갱신
		RefreshFromComponent();
	}
}

void ULB_QuickSlot::HandleSlotDropped(FName SrcGroup, int32 SrcIndex, int32 DstIndex)
{
	OnSlotDropped.Broadcast(SlotType, SrcGroup, SrcIndex, DstIndex);
}

void ULB_QuickSlot::HandleClearRequested()
{
	if (Component.IsValid())
	{
		Component->ClearSlot(SlotType, SlotIndex);
	}
}

void ULB_QuickSlot::RefreshFromComponent()
{
	UTexture2D* NewIcon = nullptr;
	int32 NewCount = 0;
	FText NewInfo;

	if (Component.IsValid())
	{
		const FLB_QuickSlotEntry Entry = Component->GetEntry(SlotType, SlotIndex);
		UE_LOG(LogTemp, Log, TEXT("[QuickSlot] Refresh Slot[%d] ItemCode: %s"), SlotIndex, *Entry.ItemCode.ToString());

		if (SlotType == ELB_QuickSlotType::Item && !Entry.ItemCode.IsNone())
		{
			ResolveItemVisual(Entry.ItemCode, NewIcon, NewCount, NewInfo);
		}
	}

	Icon = NewIcon;
	Count = NewCount;
	InfoText = NewInfo;
	ApplyState();
}

void ULB_QuickSlot::ApplyState()
{
	const TSharedPtr<SLB_QuickSlot> Box = GetQuickSlotBox();
	if (!Box.IsValid())
	{
		return;
	}

	Box->SetGroup(LBSlotGroup::FromQuickSlotType(SlotType));
	Box->SetSlotIndex(SlotIndex);
	Box->SetHotkeyText(HotkeyText);
	Box->SetHotkeyStyle(HotkeyStyle);
	Box->SetCountStyle(CountStyle);
	Box->SetInfoText(InfoText);
	Box->SetCooldownPercent(CooldownPercent);

	if (Icon)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Icon);
		Brush.ImageSize = FVector2D(Icon->GetSizeX(), Icon->GetSizeY());
		Box->SetItem(Brush, Count);
		Box->SetDimmed(SlotType == ELB_QuickSlotType::Item && Count <= 0);
	}
	else
	{
		Box->ClearItem();
		Box->SetDimmed(false);
	}
}

void ULB_QuickSlot::ResolveItemVisual(FName ItemCode, UTexture2D*& OutIcon, int32& OutCount, FText& OutInfo) const
{
	OutIcon = nullptr;
	OutCount = 0;
	OutInfo = FText::GetEmpty();

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	const ULB_DA_ItemInfo* Info = Data ? Data->GetItemInfo(ItemCode) : nullptr;
	if (!Info)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] ItemInfo not found for ItemCode: %s"), *ItemCode.ToString());
		return;
	}

	// 인벤토리 슬롯 엔트리와 같은 방식 (아이콘은 Soft라 동기 로드, 로드 후엔 캐시)
	OutIcon = Info->Icon.LoadSynchronous();
	UE_LOG(LogTemp, Log, TEXT("[QuickSlot] Loaded Icon for %s: %s"), *ItemCode.ToString(),
	       OutIcon ? *OutIcon->GetName() : TEXT("nullptr"));
	OutInfo = FText::Format(FText::FromString(TEXT("{0}\n{1}")), Info->DisplayName, Info->Description);

	// 수량은 같은 PlayerState의 인벤토리에서 (스택 합계)
	if (Component.IsValid())
	{
		if (const AActor* Owner = Component->GetOwner())
		{
			if (const ULB_InventoryComponent* Inv = Owner->FindComponentByClass<ULB_InventoryComponent>())
			{
				OutCount = Inv->GetItemCount(ItemCode);
			}
		}
	}
}
