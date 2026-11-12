// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_InventoryPanel.h"

#include "LB_InventoryItemObject.h"
#include "TimerManager.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Component/LB_InventoryComponent.h"
#include "Slate/SlateWidget/LB_Button.h"
#include "Components/TileView.h"
#include "PlayerState/LB_PlayerState.h"

void ULB_InventoryPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (bUseDummyData)
	{
		BuildDummy();
	}
	else
	{
		TryBindInventory();
	}
}

void ULB_InventoryPanel::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}
	if (BoundPlayerState.IsValid())
	{
		BoundPlayerState->OnInventoryUpdated.RemoveDynamic(this, &ULB_InventoryPanel::HandleInventoryUpdated);
		BoundPlayerState.Reset();
	}

	
	Super::NativeDestruct();
}

void ULB_InventoryPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CloseButton)
		CloseButton->OnClicked.AddUniqueDynamic(this, &ULB_InventoryPanel::HandleCloseClicked);

}

FReply ULB_InventoryPanel::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && DragHandle
		&& DragHandle->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		DragStartScreenPos = InMouseEvent.GetScreenSpacePosition();
		DragStartPanelOffset = PanelOffset;
        
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULB_InventoryPanel::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDragging)
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	// 1. 드래그 시작 위치로부터 현재 마우스가 얼마나 움직였는지 스크린/뷰포트 기준 Delta 계산 (DPI 스케일 자동 반영)
	const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D CurrentScreenPos = InMouseEvent.GetScreenSpacePosition();
	const FVector2D Delta = (CurrentScreenPos - DragStartScreenPos) / Scale;

	// 2. 기본 이동량 적용
	FVector2D TargetOffset = DragStartPanelOffset + Delta;

	// 3. 화면 밖으로 완전히 벗어나지 않도록 클램핑 (선택 사항: 원하시면 범위를 유연하게 조절 가능)
	const FVector2D Size = InGeometry.GetLocalSize();
	const FVector2D VP = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
    
	// 예시: 창이 화면 밖으로 너무 나가지 않게 하되, 막히는 느낌이 없도록 최소한의 영역만 보장
	PanelOffset = FVector2D(
		FMath::Clamp(TargetOffset.X, -Size.X + 50.f, VP.X - 50.f),
		FMath::Clamp(TargetOffset.Y, -50.f, VP.Y - 50.f)
	);

	SetRenderTranslation(PanelOffset);
	return FReply::Handled();
}

void ULB_InventoryPanel::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	// bDragging = false;   // 알트탭 등으로 캡처를 잃어도 드래그 상태가 남지 않게
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

FReply ULB_InventoryPanel::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULB_InventoryPanel::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

void ULB_InventoryPanel::HandleInventoryUpdated()
{
	const ALB_PlayerState* PS = BoundPlayerState.Get();
	const ULB_InventoryComponent* Inv = PS ? PS->Get_InventoryComponent() : nullptr;
	if (Inv)
	{
		RefreshFromEntries(Inv->GetInventoryItems());
	}
}

int32 ULB_InventoryPanel::CalcSlotCount(int32 ItemCount) const
{
	// const int32 Cols = FMath::Max(1, Columns);
	// const int32 Count = FMath::Max(MinSlots, ItemCount + Cols);
	// return FMath::DivideAndRoundUp(Count, Cols) * Cols;

	const ALB_PlayerState* PS = BoundPlayerState.Get();
	const ULB_InventoryComponent* Inv = PS ? PS->Get_InventoryComponent() : nullptr;
	// int32 Result = Inv->Get_CurrentSlot();
	return Inv->Get_CurrentSlot();
}

void ULB_InventoryPanel::RefreshFromEntries(const TArray<FLB_ItemEntry>& Items)
{
	// UE_LOG(LogTemp, Warning, TEXT("[Panel] Refresh Slot0Stack=%d"),
	// 	Items.Num() > 0 ? Items[0].StackCount : -1);

	if (!TileView)
		return;
 
	const int32 SlotCount = CalcSlotCount(Items.Num());
 
	TArray<ULB_InventoryItemObject*> Objs;
	Objs.Reserve(SlotCount);
	for (int32 i = 0; i < SlotCount; ++i)
	{
		ULB_InventoryItemObject* Obj = NewObject<ULB_InventoryItemObject>(this);
		if (Items.IsValidIndex(i))
		{
			Obj->InitItem(i, Items[i]);
		}
		else
		{
			Obj->InitEmpty(i);
		}
		Objs.Add(Obj);
	}
 
	// 갱신하면 스크롤이 튈 수 있어서 위치를 기억해 뒀다가 되돌린다
	const float PrevOffset = TileView->GetScrollOffset();
	TileView->SetListItems(Objs);
	TileView->SetScrollOffset(PrevOffset);
}

void ULB_InventoryPanel::BuildDummy()
{
	TArray<FLB_ItemEntry> Items;

	if (DummyItemCodes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryPanel] DummyItemCodes가 비어 있어 빈 칸만 표시됩니다."));
	}
	else
	{
		for (int32 i = 0; i < DummyItemCount; ++i)
		{
			FLB_ItemEntry E;
			E.ItemCode = DummyItemCodes[i % DummyItemCodes.Num()];
			E.StackCount = (i % 5) + 1;   // 1~5, 수량 표시(2 이상일 때만 보임) 확인용
			E.bIsEquipped = (i == 0);     // 첫 칸은 장착 표시(Selected 색) 확인용
			Items.Add(E);
		}
	}

	RefreshFromEntries(Items);
}

void ULB_InventoryPanel::TryBindInventory()
{
	if (BoundPlayerState .IsValid()) return;
 
	ALB_PlayerState* PS = GetOwningPlayerState<ALB_PlayerState>();
	if (!PS)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(BindRetryHandle, this, &ULB_InventoryPanel::TryBindInventory, 0.2f, false);
		}
		return;
	}

	BoundPlayerState = PS;
	PS->OnInventoryUpdated.AddUniqueDynamic(this, &ULB_InventoryPanel::HandleInventoryUpdated);
	HandleInventoryUpdated();   // 이미 들어 있는 아이템을 처음 한 번 표시
}