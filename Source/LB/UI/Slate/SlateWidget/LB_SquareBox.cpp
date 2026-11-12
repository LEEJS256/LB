#include "LB_SquareBox.h"

#include "../SLB_SquareBox.h"
#include "Components/Widget.h"
#include "Data/LB_DA_ItemInfo.h"
#include "UI/LB_ItemTooltip.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SToolTip.h"


ULB_SquareBox::ULB_SquareBox(const FObjectInitializer& ObjectInitializer)
{
	// if (!ToolTipWidgetDelegate.IsBound())
	// {
	// 	ToolTipWidgetDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULB_SquareBox, GetItemTooltip));
	// }
}

TSharedRef<SLB_SquareBox> ULB_SquareBox::CreateSquareBox()
{
	return SNew(SLB_SquareBox);
}

UWidget* ULB_SquareBox::GetItemTooltip()
{
	const ULB_DA_ItemInfo* DA_Info = GetCurrentItemInfo();
	if (!DA_Info)
	{
		UE_LOG(LogTemp, Log, TEXT("[Tooltip] GetItemTooltip: 빈 칸이거나 아이템 정보가 없습니다."));
		return nullptr;
	}

	if (!ItemTooltipClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Tooltip] GetItemTooltip: ItemTooltipClass가 nullptr입니다!"));
		return nullptr;
	}

	if (!ItemTooltipWidget)
	{
		ItemTooltipWidget = CreateWidget<ULB_ItemTooltip>(GetOwningPlayer(), ItemTooltipClass);
	}

	ItemTooltipWidget->SetItem(DA_Info);
	UE_LOG(LogTemp, Log, TEXT("[Tooltip] GetItemTooltip 성공: %s"), *DA_Info->DisplayName.ToString());

	return ItemTooltipWidget;
}

TSubclassOf<ULB_ItemTooltip> ULB_SquareBox::FindDefaultTooltipClass()
{
	static ConstructorHelpers::FClassFinder<ULB_ItemTooltip> Finder(
		TEXT("/Game/LB/UI/WBP_ItemTooltip.WBP_ItemTooltip_C"));
	return Finder.Succeeded() ? Finder.Class : nullptr;
}

TSharedRef<SWidget> ULB_SquareBox::RebuildWidget()
{
	MySquareBox = CreateSquareBox();

	if (GetContentSlot() && GetContentSlot()->Content)
	{
		MySquareBox->SetContent(GetContentSlot()->Content->TakeWidget());
	}


	return MySquareBox.ToSharedRef();
}

void ULB_SquareBox::SynchronizeProperties()
{
	if (!ToolTipWidgetDelegate.IsBound())
	{
		ToolTipWidgetDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULB_SquareBox, GetItemTooltip));
	}
	
	Super::SynchronizeProperties();

	if (MySquareBox.IsValid())
	{
		MySquareBox->ApplyStyle(Style);
		MySquareBox->SetSelected(bSelected);
	}
}

void ULB_SquareBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySquareBox.Reset();
}

void ULB_SquareBox::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MySquareBox.IsValid() && InSlot && InSlot->Content)
	{
		MySquareBox->SetContent(InSlot->Content->TakeWidget());
	}
}

void ULB_SquareBox::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MySquareBox.IsValid())
	{
		MySquareBox->SetContent(SNullWidget::NullWidget);
	}
}


void ULB_SquareBox::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	if (MySquareBox.IsValid())
	{
		MySquareBox->SetSelected(bSelected);
	}
}

void ULB_SquareBox::SetStyle(const FLB_SquareBoxStyle& InStyle)
{
	Style = InStyle;
	if (MySquareBox.IsValid())
	{
		MySquareBox->ApplyStyle(Style);
	}
}

const ULB_DA_ItemInfo* ULB_SquareBox::GetCurrentItemInfo() const
{
	return nullptr;
}

#if WITH_EDITOR
const FText ULB_SquareBox::GetPaletteCategory()
{
	return FText::FromString(TEXT("LB"));
}



#endif

