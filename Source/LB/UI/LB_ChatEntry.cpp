// LB_ChatEntry.cpp
#include "UI/LB_ChatEntry.h"

#include "Components/TextBlock.h"

void ULB_ChatEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const ULB_ChatItemObject* Item = Cast<ULB_ChatItemObject>(ListItemObject);
	if (!Item || !MessageText)
		return;

	const FLB_ChatMessage& M = Item->Message;
	FString Line;
	FSlateColor Color = AllColor;

	switch (M.Channel)
	{
	case ELB_ChatChannel::Party:
		Line = FString::Printf(TEXT("[파티] %s: %s"), *M.Sender, *M.Text);
		Color = PartyColor;
		break;
	case ELB_ChatChannel::System:
		Line = FString::Printf(TEXT("[시스템] %s"), *M.Text);
		Color = SystemColor;
		break;
	default:
		Line = FString::Printf(TEXT("%s: %s"), *M.Sender, *M.Text);
		break;
	}
	if (bShowTimestamp)
		Line = FString::Printf(TEXT("[%02d:%02d] %s"), Item->LocalTime.GetHour(), Item->LocalTime.GetMinute(), *Line);

	MessageText->SetText(FText::FromString(Line));
	MessageText->SetColorAndOpacity(Color);
}