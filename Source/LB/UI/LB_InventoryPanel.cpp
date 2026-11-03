// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_InventoryPanel.h"

#include "LB_InventoryItemObject.h"
#include "TimerManager.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Component/LB_InventoryComponent.h"
#include "Slate/SlateWidget/LB_Button.h"
#include "Components/TileView.h"
#include "GameFramework/PlayerState.h"
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
		GrabLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULB_InventoryPanel::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDragging) return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	// 잡은 지점과 현재 커서 위치의 차이만큼 창을 옮긴다 (DPI/스케일 자동 대응)
	const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D Delta = Local - GrabLocal;

	// 화면 밖으로 못 나가게 막는다
	FVector2D PixelPos, ViewPos;
	USlateBlueprintLibrary::LocalToViewport(this, InGeometry, FVector2D::ZeroVector, PixelPos, ViewPos);
	const FVector2D Size = InGeometry.GetLocalSize();
	const FVector2D VP = UWidgetLayoutLibrary::GetViewportSize(this) / UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D Target = ViewPos + Delta;
	const FVector2D Clamped(
		FMath::Clamp(Target.X, 0.0, FMath::Max(0.0, VP.X - Size.X)),
		FMath::Clamp(Target.Y, 0.0, FMath::Max(0.0, VP.Y - Size.Y)));

	PanelOffset += Clamped - ViewPos;
	SetRenderTranslation(PanelOffset);
	return FReply::Handled();
}

void ULB_InventoryPanel::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDragging = false;   // 알트탭 등으로 캡처를 잃어도 드래그 상태가 남지 않게
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
	const int32 Cols = FMath::Max(1, Columns);
	const int32 Count = FMath::Max(MinSlots, ItemCount + Cols);
	return FMath::DivideAndRoundUp(Count, Cols) * Cols;
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