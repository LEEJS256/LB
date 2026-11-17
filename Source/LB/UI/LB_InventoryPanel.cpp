// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_InventoryPanel.h"

#include "LB_InventoryItemObject.h"
#include "TimerManager.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "Utility/LB_NativeGameplayTag.h"
#include "Component/LB_InventoryComponent.h"
#include "SlateWidget/LB_Button.h"
#include "Components/TileView.h"
#include "PlayerState/LB_PlayerState.h"

void ULB_InventoryPanel::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateTabVisuals();

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

	if (Tab_Equip)
		Tab_Equip->OnClicked.AddUniqueDynamic(this, &ULB_InventoryPanel::HandleTabEquipClicked);
	if (Tab_Consume)
		Tab_Consume->OnClicked.AddUniqueDynamic(this, &ULB_InventoryPanel::HandleTabConsumeClicked);
	if (Tab_Currency)
		Tab_Currency->OnClicked.AddUniqueDynamic(this, &ULB_InventoryPanel::HandleTabCurrencyClicked);
	if (SortButton)
		SortButton->OnClicked.AddUniqueDynamic(this, &ULB_InventoryPanel::HandleSortClicked);

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

void ULB_InventoryPanel::SetFilter(FGameplayTag InFilter)
{
	// '전체' 탭이 없어서 같은 탭을 다시 눌러도 그대로 둔다
	if (CurrentFilter == InFilter)
		return;
	CurrentFilter = InFilter;

	UpdateTabVisuals();
	RebuildList(/*bResetScroll*/ true);
}

void ULB_InventoryPanel::HandleTabEquipClicked()    { SetFilter(TAG_Item_Equip); }
void ULB_InventoryPanel::HandleTabConsumeClicked()  { SetFilter(TAG_Item_Consumable); }
void ULB_InventoryPanel::HandleTabCurrencyClicked() { SetFilter(TAG_Item_Currency); }

void ULB_InventoryPanel::HandleSortClicked()
{
	const ALB_PlayerState* PS = BoundPlayerState.Get();
	if (ULB_InventoryComponent* Inv = PS ? PS->Get_InventoryComponent() : nullptr)
	{
		Inv->Server_SortItems();
	}
}

void ULB_InventoryPanel::UpdateTabVisuals()
{
	auto Apply = [this](ULB_Button* Tab, const FGameplayTag& TabTag)
	{
		if (!Tab) return;
		const bool bActive = CurrentFilter.IsValid() && CurrentFilter == TabTag;
		Tab->SetBorderColor(bActive ? TabActiveBorderColor : TabInactiveBorderColor);
	};

	Apply(Tab_Equip,    TAG_Item_Equip);
	Apply(Tab_Consume,  TAG_Item_Consumable);
	Apply(Tab_Currency, TAG_Item_Currency);
}

bool ULB_InventoryPanel::PassFilter(const FLB_ItemEntry& Entry) const
{
	// 빈 칸은 어느 탭에서든 보여줘야 거기로 드래그해서 옮길 수 있다
	if (Entry.ItemCode.IsNone() || !CurrentFilter.IsValid())
		return true;

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	const ULB_DA_ItemInfo* Info = Data ? Data->GetItemInfo(Entry.ItemCode) : nullptr;

	// MatchesTag: Item.Equip 탭이면 Item.Equip.xxx 하위 태그도 통과
	return Info && Info->ItemTypeTag.MatchesTag(CurrentFilter);
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

	// 더미 모드처럼 컴포넌트가 없을 땐 아이템 수 기준으로 (예전엔 여기서 null 크래시)
	return Inv ? Inv->Get_CurrentSlot() : FMath::Max(MinSlots, ItemCount);
}

void ULB_InventoryPanel::RefreshFromEntries(const TArray<FLB_ItemEntry>& Items)
{
	CachedItems = Items;
	RebuildList(/*bResetScroll*/ false);
}

void ULB_InventoryPanel::RebuildList(bool bResetScroll)
{
	if (!TileView)
		return;

	const int32 SlotCount = CalcSlotCount(CachedItems.Num());

	TArray<ULB_InventoryItemObject*> Objs;
	Objs.Reserve(SlotCount);

	// 원본 배열 순서를 유지한 채 필터에 안 맞는 아이템 칸만 뺀다.
	// SlotIndex는 TileView 위치가 아니라 원본 배열 인덱스 그대로 넘겨야
	// Server_MoveItem(Src, Dst)이 필터 중에도 올바른 칸을 바꾼다.
	for (int32 i = 0; i < SlotCount; ++i)
	{
		const bool bHasEntry = CachedItems.IsValidIndex(i);
		if (bHasEntry && !PassFilter(CachedItems[i]))
			continue;

		ULB_InventoryItemObject* Obj = NewObject<ULB_InventoryItemObject>(this);
		if (bHasEntry && !CachedItems[i].ItemCode.IsNone())
		{
			Obj->InitItem(i, CachedItems[i]);
		}
		else
		{
			Obj->InitEmpty(i);
		}
		Objs.Add(Obj);
	}

	// 필터로 칸이 줄어 줄이 덜 찼을 때 모양 유지용 패딩.
	// 실제 슬롯이 아니라 INDEX_NONE이고, 여기로 드롭하면 서버에서 범위 체크로 무시된다.
	const int32 Cols = FMath::Max(1, Columns);
	const int32 TargetCount = FMath::Max(MinSlots, FMath::DivideAndRoundUp(Objs.Num(), Cols) * Cols);
	while (Objs.Num() < TargetCount)
	{
		ULB_InventoryItemObject* Pad = NewObject<ULB_InventoryItemObject>(this);
		Pad->InitEmpty(INDEX_NONE);
		Objs.Add(Pad);
	}

	// 아이템 갱신 땐 스크롤 유지, 탭 전환 땐 맨 위로
	const float PrevOffset = TileView->GetScrollOffset();
	TileView->SetListItems(Objs);
	TileView->SetScrollOffset(bResetScroll ? 0.f : PrevOffset);
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