// LB_TabButton.cpp
#include "UI/SlateWidget/LB_TabButton.h"

TSharedRef<SWidget> ULB_TabButton::RebuildWidget()
{
	OnClicked.AddUniqueDynamic(this, &ULB_TabButton::HandleClicked);
	return Super::RebuildWidget();
}

void ULB_TabButton::InitTab(int32 InIndex, const FText& InLabel)
{
	TabIndex = InIndex;
	BaseLabel = InLabel;
	RefreshLabel();
}

void ULB_TabButton::SetUnreadCount(int32 InCount)
{
	UnreadCount = FMath::Max(0, InCount);
	RefreshLabel();
}

void ULB_TabButton::HandleClicked()
{
	OnTabClicked.Broadcast(TabIndex);
}

void ULB_TabButton::RefreshLabel()
{
	if (UnreadCount <= 0)
	{
		SetLabelText(BaseLabel);
		return;
	}
	const FString Num = UnreadCount > 99 ? TEXT("99+") : FString::FromInt(UnreadCount);
	SetLabelText(FText::Format(INVTEXT("{0} - {1}"), BaseLabel, FText::FromString(Num)));
}