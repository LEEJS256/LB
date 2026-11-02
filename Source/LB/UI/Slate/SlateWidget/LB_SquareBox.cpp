#include "LB_SquareBox.h"

#include "../SLB_SquareBox.h"
#include "Components/Widget.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SLB_SquareBox> ULB_SquareBox::CreateSquareBox()
{
	return SNew(SLB_SquareBox);
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

#if WITH_EDITOR
const FText ULB_SquareBox::GetPaletteCategory()
{
	return FText::FromString(TEXT("LB"));
}
#endif