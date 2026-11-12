// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Utility/LB_ItemEntry.h"
#include "LB_InventoryPanel.generated.h"

class UTileView;
class ULB_Button;
class ALB_PlayerState;
class ULB_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLB_OnInventoryCloseRequested);


/**
 * TileView는 보이는 줄(+여유 한두 줄)만큼의 엔트리만 만들어 돌려 쓴다.
 *
 * 지금은 bUseDummyData로 UI만 단독 검증하고, 7번에서 인벤토리 컴포넌트가
 * 바뀔 때 RefreshFromEntries를 호출하도록 연결한다.
 */
UCLASS(Abstract)
class LB_API ULB_InventoryPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	
	/** 슬롯 목록을 다시 만든다. 배열 인덱스가 곧 슬롯 번호 */
	void RefreshFromEntries(const TArray<FLB_ItemEntry>& Items);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FLB_OnInventoryCloseRequested OnCloseRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class ULB_Button> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> TileView;

	/** TileView 열 수(= 한 줄 칸 수). 실제 열 수는 TileView 너비 / EntryWidth로 정해지니 WBP 크기와 맞출 것 */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Columns = 4;

	/** 아이템이 적어도 항상 보여줄 최소 칸 수 (4x4 = 16) */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MinSlots = 16;

	// ---- UI 단독 검증용 ----
	UPROPERTY(EditAnywhere, Category = "Inventory|Debug")
	bool bUseDummyData = false;

	UPROPERTY(EditAnywhere, Category = "Inventory|Debug", meta = (ClampMin = "0"))
	int32 DummyItemCount = 30;


	UPROPERTY(EditAnywhere, Category = "Inventory|Debug")
	TArray<FName> DummyItemCodes;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DragHandle;
private:
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleInventoryUpdated();

	/** PlayerState 복제가 늦을 수 있어서 못 찾으면 타이머로 재시도 */
	void TryBindInventory();

	bool bDragging = false;
	FVector2D GrabLocal = FVector2D::ZeroVector;   // 잡은 지점(창 로컬 좌표)
	FVector2D PanelOffset = FVector2D::ZeroVector; // 현재 이동량

	FVector2D DragStartScreenPos; // 드래그 시작 시점의 마우스 스크린(절대) 좌표
	FVector2D DragStartPanelOffset; // 드래그 시작 시점의 패널 오프셋
	
	
	int32 CalcSlotCount(int32 ItemCount) const;

	void BuildDummy();

	TWeakObjectPtr<ALB_PlayerState> BoundPlayerState;
	FTimerHandle BindRetryHandle;
};


