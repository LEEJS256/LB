// LB_TabButton.h
#pragma once

#include "CoreMinimal.h"
#include "UI/SlateWidget/LB_Button.h"
#include "LB_TabButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnTabClicked, int32, TabIndex);

UCLASS()
class LB_API ULB_TabButton : public ULB_Button
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "LB|Tab")
	FLB_OnTabClicked OnTabClicked;

	UFUNCTION(BlueprintCallable, Category = "LB|Tab")
	void InitTab(int32 InIndex, const FText& InLabel);

	/** 0이면 뱃지 숨김, 99 초과는 99+ */
	UFUNCTION(BlueprintCallable, Category = "LB|Tab")
	void SetUnreadCount(int32 InCount);

	int32 GetTabIndex() const { return TabIndex; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void HandleClicked();

	void RefreshLabel();

	int32 TabIndex = INDEX_NONE;
	int32 UnreadCount = 0;
	FText BaseLabel;
};