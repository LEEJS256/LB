// LB_ChatPanel.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UI_Utility/LB_ChatTypes.h"
#include "Types/SlateEnums.h"
#include "LB_ChatPanel.generated.h"

class UListView;
class UBorder;
class USizeBox;
class ULB_Button;
class UEditableTextBox;
class UPanelWidget;
class ULB_TabButton;
class ULB_ChatComponent;
class ULB_ChatItemObject;

USTRUCT(BlueprintType)
struct FLB_ChatTabDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	FText Label;

	UPROPERTY(EditAnywhere, Category = "LB|Chat", meta = (Bitmask, BitmaskEnum = "/Script/LB.ELB_ChatChannel"))
	int32 ChannelMask = 0;

	/** 이 탭에서 Enter 쳤을 때 보낼 채널 (시스템 탭이면 All로) */
	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	ELB_ChatChannel SendChannel = ELB_ChatChannel::All;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnChatInputToggled, bool, bOpen);

UCLASS(Abstract)
class LB_API ULB_ChatPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LB|Chat")
	void OpenInput();

	UFUNCTION(BlueprintCallable, Category = "LB|Chat")
	void CloseInput();

	UFUNCTION(BlueprintPure, Category = "LB|Chat")
	bool IsInputOpen() const { return bInputOpen; }

	UFUNCTION(BlueprintCallable, Category = "LB|Chat")
	void SetActiveTab(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "LB|Chat")
	void ToggleExpand() { HandleExpandToggleClicked(); }
	
	/** PC가 구독해서 InputMode 전환 */
	UPROPERTY(BlueprintAssignable, Category = "LB|Chat")
	FLB_OnChatInputToggled OnInputToggled;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> ExpandToggleButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> InputRow; // 대상 버튼 + 입력창 묶음

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> ChatSizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> SendTargetButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TabBar; // 안에 ULB_TabButton을 Tabs 순서대로 배치

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> MessageList; // EntryWidgetClass = WBP_ChatEntry

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> InputBox;

	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	TArray<FLB_ChatTabDef> Tabs;

	UPROPERTY(EditAnywhere, Category = "LB|Chat", meta = (ClampMin = "0.0"))
	float ChatOpenTime = 3.f;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Toggle")
	FText PinLabel = INVTEXT("▲");      // 눌러서 펼침+고정

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Toggle")
	FText UnpinLabel = INVTEXT("▼");    // 눌러서 고정 해제+접기
	
	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	int32 MaxMessages = 100;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Fade")
	float FadeDelay = 4.f;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Fade")
	float IdleOpacity = 0.4f;

	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	bool bHideChromeWhenIdle = true;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Target")
	FText AllTargetLabel = INVTEXT("모두에게");

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Target")
	FText PartyTargetLabel = INVTEXT("파티에게");

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Fade")
	float ActiveBgAlpha = 0.6f;

	UPROPERTY(EditAnywhere, Category = "LB|Chat|Fade")
	float IdleBgAlpha = 0.2f;

	UPROPERTY(EditAnywhere, Category = "LB|Chat")
	float ExpandedHeight = 260.f;

private:
	void SetBackgroundAlpha(float Alpha);

	UFUNCTION()
	void HandleSendTargetClicked();

	void SetSendTarget(ELB_ChatChannel NewTarget);
	void SetExpanded(bool bInExpanded);

	ELB_ChatChannel SendTarget = ELB_ChatChannel::All;

	UFUNCTION()
	void HandleChatReceived(const FLB_ChatMessage& Msg);

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	bool PassesTab(const FLB_ChatMessage& Msg, int32 TabIndex) const;
	void ParseCommand(FString& InOutText, ELB_ChatChannel& InOutChannel) const;

	void WakeUp();
	void ScheduleFade();
	void FadeOut();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULB_TabButton>> TabButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULB_ChatItemObject>> AllItems;

	TArray<int32> UnreadCounts;
	TWeakObjectPtr<ULB_ChatComponent> ChatComp;

	UFUNCTION()
	void HandleExpandToggleClicked();

	bool ShouldStayExpanded() const;
	void ScheduleCollapse();
	void TryCollapse();
	void RefreshToggleLabel();

	FTimerHandle CollapseTimer;
	bool bExpanded = false;
	// bool bPinnedOpen = false;
	
	int32 ActiveTab = 0;
	bool bInputOpen = false;
	FTimerHandle FadeTimer;
};
