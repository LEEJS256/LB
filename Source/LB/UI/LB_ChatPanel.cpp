// LB_ChatPanel.cpp
#include "UI/LB_ChatPanel.h"
#include "UI/SlateWidget/LB_Button.h"
#include "Component/LB_ChatComponent.h"
#include "UI/LB_ChatEntry.h"
#include "UI/SlateWidget/LB_TabButton.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/PanelWidget.h"
#include "TimerManager.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"

void ULB_ChatPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 탭 버튼 수집 (배치 순서 = Tabs 인덱스)
	TabButtons.Reset();
	for (UWidget* Child : TabBar->GetAllChildren())
	{
		if (ULB_TabButton* Btn = Cast<ULB_TabButton>(Child))
			TabButtons.Add(Btn);
	}
	ensureMsgf(TabButtons.Num() == Tabs.Num(),
		TEXT("[Chat] 탭 버튼 수(%d)와 Tabs(%d)가 다릅니다"), TabButtons.Num(), Tabs.Num());

	const int32 Num = FMath::Min(TabButtons.Num(), Tabs.Num());
	for (int32 i = 0; i < Num; ++i)
	{
		TabButtons[i]->InitTab(i, Tabs[i].Label);
		TabButtons[i]->OnTabClicked.AddUniqueDynamic(this, &ULB_ChatPanel::SetActiveTab);
	}
	UnreadCounts.Init(0, Tabs.Num());

	InputBox->OnTextCommitted.AddUniqueDynamic(this, &ULB_ChatPanel::HandleTextCommitted);
	// InputBox->SetVisibility(ESlateVisibility::Collapsed);
	// TabBar->SetVisibility(ESlateVisibility::Collapsed); // 입력 모드에서만 보임

	InputBox->OnTextCommitted.AddUniqueDynamic(this, &ULB_ChatPanel::HandleTextCommitted);

	if (SendTargetButton)
		SendTargetButton->OnClicked.AddUniqueDynamic(this, &ULB_ChatPanel::HandleSendTargetClicked);

	if (ExpandToggleButton)
		ExpandToggleButton->OnClicked.AddUniqueDynamic(this, &ULB_ChatPanel::HandleExpandToggleClicked);

	
	SetExpanded(!bHideChromeWhenIdle);
	RefreshToggleLabel();
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULB_ChatComponent* Comp = PC->FindComponentByClass<ULB_ChatComponent>())
		{
			ChatComp = Comp;
			Comp->OnChatReceived.AddUniqueDynamic(this, &ULB_ChatPanel::HandleChatReceived);
		}
	}

	SetActiveTab(0);
	SetRenderOpacity(IdleOpacity);
	SetBackgroundAlpha(IdleBgAlpha);
}

void ULB_ChatPanel::NativeDestruct()
{
	if (ChatComp.IsValid())
		ChatComp->OnChatReceived.RemoveDynamic(this, &ULB_ChatPanel::HandleChatReceived);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimer);
		World->GetTimerManager().ClearTimer(CollapseTimer);
	}
	
	Super::NativeDestruct();
}

FReply ULB_ChatPanel::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bInputOpen && InKeyEvent.GetKey() == EKeys::Tab)
	{
		HandleSendTargetClicked();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);

	
}

// ---------------- 메시지 ----------------

void ULB_ChatPanel::SetBackgroundAlpha(float Alpha)
{
	if (!Background)
		return;

	FLinearColor Color = Background->GetBrushColor();
	Color.A = Alpha;
	Background->SetBrushColor(Color);
}

void ULB_ChatPanel::HandleSendTargetClicked()
{
	SetSendTarget(SendTarget == ELB_ChatChannel::Party ? ELB_ChatChannel::All : ELB_ChatChannel::Party);

	if (bInputOpen)
		InputBox->SetKeyboardFocus();
}

void ULB_ChatPanel::SetSendTarget(ELB_ChatChannel NewTarget)
{
	// 시스템 채널로는 보낼 수 없으니 All로 보정
	SendTarget = (NewTarget == ELB_ChatChannel::Party) ? ELB_ChatChannel::Party : ELB_ChatChannel::All;

	if (!SendTargetButton)
		return;

	SendTargetButton->SetLabelText(SendTarget == ELB_ChatChannel::Party ? PartyTargetLabel : AllTargetLabel);
}

void ULB_ChatPanel::SetExpanded(bool bInExpanded)
{
	bExpanded = bInExpanded;
	RefreshToggleLabel();
	
	const ESlateVisibility Vis = bInExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (Background)
	{
		Background->SetVisibility(Vis);
	}
	else
	{
		MessageList->SetVisibility(Vis);
		if (InputRow)
			InputRow->SetVisibility(Vis);
	}

	if (!ChatSizeBox)
		return;

	if (bExpanded)
		ChatSizeBox->SetHeightOverride(ExpandedHeight);
	else
		ChatSizeBox->ClearHeightOverride();
}


void ULB_ChatPanel::HandleChatReceived(const FLB_ChatMessage& Msg)
{
	ULB_ChatItemObject* Item = NewObject<ULB_ChatItemObject>(this);
	Item->Message = Msg;
	Item->LocalTime = FDateTime::Now();
	AllItems.Add(Item);

	if (AllItems.Num() > MaxMessages)
	{
		ULB_ChatItemObject* Oldest = AllItems[0];
		AllItems.RemoveAt(0);
		MessageList->RemoveItem(Oldest); // 현재 탭에 없으면 무시됨
	}

	if (PassesTab(Msg, ActiveTab))
	{
		MessageList->AddItem(Item);
		MessageList->ScrollToBottom();
	}

	const bool bCollapsed =  !bExpanded;
	for (int32 i = 0; i < TabButtons.Num() && i < Tabs.Num(); ++i)
	{
		if ((i != ActiveTab || bCollapsed) && PassesTab(Msg, i))
			TabButtons[i]->SetUnreadCount(++UnreadCounts[i]);
	}
	
	if (bExpanded && !bInputOpen)
		ScheduleCollapse();   // 대화가 이어지는 동안엔 계속 펼침

	
	WakeUp();
}

bool ULB_ChatPanel::PassesTab(const FLB_ChatMessage& Msg, int32 TabIndex) const
{
	return Tabs.IsValidIndex(TabIndex)
		&& (Tabs[TabIndex].ChannelMask & static_cast<int32>(Msg.Channel)) != 0;
}

// ---------------- 탭 ----------------

void ULB_ChatPanel::SetActiveTab(int32 Index)
{
	if (!Tabs.IsValidIndex(Index))
		return;

	ActiveTab = Index;
	UnreadCounts[Index] = 0;

	for (int32 i = 0; i < TabButtons.Num(); ++i)
	{
		TabButtons[i]->SetSelected(i == Index);
		if (i == Index)
			TabButtons[i]->SetUnreadCount(0);
	}

	TArray<ULB_ChatItemObject*> Filtered;
	for (ULB_ChatItemObject* Item : AllItems)
	{
		if (PassesTab(Item->Message, Index))
			Filtered.Add(Item);
	}
	MessageList->SetListItems(Filtered);
	MessageList->ScrollToBottom();

	// 탭 클릭으로 포커스가 빠졌으면 다시 입력창으로
	if (bInputOpen)
		InputBox->SetKeyboardFocus();

	SetSendTarget(Tabs[Index].SendChannel);
}

// ---------------- 입력 ----------------

void ULB_ChatPanel::OpenInput()
{
	if (bInputOpen)
		return;
	bInputOpen = true;

	GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
	GetWorld()->GetTimerManager().ClearTimer(CollapseTimer);
	
	SetRenderOpacity(1.f);
	SetBackgroundAlpha(ActiveBgAlpha);

	if (UnreadCounts.IsValidIndex(ActiveTab) && TabButtons.IsValidIndex(ActiveTab))
	{
		UnreadCounts[ActiveTab] = 0;
		TabButtons[ActiveTab]->SetUnreadCount(0);
	}
	
	SetExpanded(true);

	OnInputToggled.Broadcast(true);
	InputBox->SetKeyboardFocus();
}

void ULB_ChatPanel::CloseInput()
{
	if (!bInputOpen)
		return;
	bInputOpen = false;

	InputBox->SetText(FText::GetEmpty());


	SetBackgroundAlpha(IdleBgAlpha);
	OnInputToggled.Broadcast(false);
	ScheduleFade();
	ScheduleCollapse(); 
}

void ULB_ChatPanel::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// 포커스 이동(탭 클릭 등)으로는 닫지 않음
	if (CommitMethod == ETextCommit::OnCleared || CommitMethod == ETextCommit::OnUserMovedFocus)
	{
		CloseInput();
		return;
	}
	if (CommitMethod != ETextCommit::OnEnter)
		return;

	FString Str = Text.ToString();
	ELB_ChatChannel Channel = SendTarget;   // 탭 기본값 대신 현재 선택된 대상
	ParseCommand(Str, Channel);

	if (ChatComp.IsValid())
		ChatComp->SendChat(Channel, Str);

	CloseInput();
}

void ULB_ChatPanel::ParseCommand(FString& InOutText, ELB_ChatChannel& InOutChannel) const
{
	auto TryPrefix = [&](const TCHAR* Prefix, ELB_ChatChannel NewChannel)
	{
		if (!InOutText.StartsWith(Prefix, ESearchCase::IgnoreCase))
			return false;
		InOutText.RightChopInline(FCString::Strlen(Prefix));
		InOutChannel = NewChannel;
		return true;
	};

	TryPrefix(TEXT("/p "), ELB_ChatChannel::Party) || TryPrefix(TEXT("/a "), ELB_ChatChannel::All);
}

// ---------------- 페이드 ----------------

void ULB_ChatPanel::WakeUp()
{
	if (bInputOpen)
		return;
	SetRenderOpacity(1.f);
	ScheduleFade();
}

void ULB_ChatPanel::ScheduleFade()
{
	GetWorld()->GetTimerManager().SetTimer(FadeTimer, this, &ULB_ChatPanel::FadeOut, FadeDelay, false);
}

void ULB_ChatPanel::FadeOut()
{
	if (!bInputOpen)
		SetRenderOpacity(IdleOpacity);
}

bool ULB_ChatPanel::ShouldStayExpanded() const
{
	return bInputOpen || !bHideChromeWhenIdle;
}

void ULB_ChatPanel::ScheduleCollapse()
{
	if (ShouldStayExpanded())
		return;

	GetWorld()->GetTimerManager().SetTimer(CollapseTimer, this, &ULB_ChatPanel::TryCollapse, FMath::Max(ChatOpenTime, 0.01f), false);
}

void ULB_ChatPanel::TryCollapse()
{
	if (ShouldStayExpanded())
		return;

	SetExpanded(false);
}

void ULB_ChatPanel::HandleExpandToggleClicked()
{
	GetWorld()->GetTimerManager().ClearTimer(CollapseTimer);

	if (bExpanded)
	{
		if (bInputOpen)
			CloseInput();

		GetWorld()->GetTimerManager().ClearTimer(CollapseTimer); // CloseInput이 건 타이머 취소
		SetExpanded(false);
		return;
	}

	SetExpanded(true);
	MessageList->ScrollToBottom();
	ScheduleCollapse(); // 수동으로 펼쳐도 ChatOpenTime 뒤엔 접힘
}

void ULB_ChatPanel::RefreshToggleLabel()
{
	if (!ExpandToggleButton)
		return;

	ExpandToggleButton->SetLabelText(bExpanded ? UnpinLabel : PinLabel);
}
