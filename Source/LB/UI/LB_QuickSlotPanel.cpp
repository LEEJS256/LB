#include "LB_QuickSlotPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Component/LB_InventoryComponent.h"
#include "PlayerState/LB_PlayerState.h"
#include "UI/Slate/SlateWidget/LB_QuickSlot.h"
#include "Utility/LB_ItemEntry.h"

ULB_QuickSlotPanel::ULB_QuickSlotPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SkillKeyLabels = {INVTEXT("A"), INVTEXT("S"), INVTEXT("D"), INVTEXT("F")};
	ItemKeyLabels = {INVTEXT("1"), INVTEXT("2"), INVTEXT("3"), INVTEXT("4")};
}

void ULB_QuickSlotPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RebuildSlots();
}

void ULB_QuickSlotPanel::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 디자이너에서는 속성(폰트/정렬/라벨)이 바뀔 때마다 다시 만들어서 바로 확인
	if (IsDesignTime())
	{
		RebuildSlots();
	}
}

void ULB_QuickSlotPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void ULB_QuickSlotPanel::NativeConstruct()
{
	Super::NativeConstruct();
	TryAutoBind();
}

void ULB_QuickSlotPanel::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}
	Super::NativeDestruct();
}

void ULB_QuickSlotPanel::BuildSlots(ELB_QuickSlotType Type, UHorizontalBox* Box, int32 Count,
                                    const TArray<FText>& Labels)
{
	if (!Box || !WidgetTree) return;

	Box->ClearChildren();

	for (int32 i = 0; i < Count; ++i)
	{
		ULB_QuickSlot* QuickSlot = WidgetTree->ConstructWidget<ULB_QuickSlot>(ULB_QuickSlot::StaticClass());
		QuickSlot->Setup(Type, i, Labels.IsValidIndex(i) ? Labels[i] : FText::GetEmpty());
		QuickSlot->SetStyle(SlotStyle); // ULB_SquareBox::SetStyle (크기/색/테두리)
		QuickSlot->SetTextStyles(HotkeyStyle, CountStyle);
		QuickSlot->OnSlotDropped.AddDynamic(this, &ULB_QuickSlotPanel::HandleSlotDropped);

		if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(QuickSlot))
		{
			BoxSlot->SetPadding(SlotPadding);
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); // 칸 크기는 SquareBox의 BoxSize가 결정
		}

		Slots.Add(QuickSlot);
		// if (Type == ELB_QuickSlotType::Skill)
		// {
		// 	SkillSlotWidgets.Add(QuickSlot);
		// }
	}
}

void ULB_QuickSlotPanel::RebuildSlots()
{
	Slots.Reset();
	SkillSlotWidgets.Reset();
	BuildSlots(ELB_QuickSlotType::Skill, SkillBox, ULB_QuickSlotComponent::SkillSlotCount, SkillKeyLabels);
	BuildSlots(ELB_QuickSlotType::Item, ItemBox, ULB_QuickSlotComponent::ItemSlotCount, ItemKeyLabels);
}

void ULB_QuickSlotPanel::TryAutoBind()
{
	// 멀티에선 위젯이 PlayerState보다 먼저 만들어질 수 있어서, 준비될 때까지 재시도
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (ALB_PlayerState* PS = PC->GetPlayerState<ALB_PlayerState>())
		{
			if (ULB_QuickSlotComponent* QS_Comp = PS->Get_QuickSlotComponent())
			{
				BindToComponent(QS_Comp);
				return; 
			}
		}
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryHandle, this, &ULB_QuickSlotPanel::TryAutoBind, 0.2f, false);
	}
}

void ULB_QuickSlotPanel::BindToComponent(ULB_QuickSlotComponent* InComponent)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}

	Component = InComponent;

	for (ULB_QuickSlot* QuickSlot : Slots)
	{
		if (QuickSlot)
		{
			QuickSlot->BindToComponent(InComponent);
		}
	}
}

void ULB_QuickSlotPanel::HandleSlotDropped(ELB_QuickSlotType DstType, FName SrcGroup, int32 SrcIndex, int32 DstIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[QuickSlot] Drop %s[%d] -> %s[%d]"),
	*SrcGroup.ToString(), SrcIndex, *LBSlotGroup::FromQuickSlotType(DstType).ToString(), DstIndex);

	if (!Component.IsValid())
		return;

	// 1) 인벤토리 → 퀵슬롯: 아이템 칸에만 등록
	if (SrcGroup == LBSlotGroup::Inventory())
	{
		if (DstType != ELB_QuickSlotType::Item)
			return;

		const FName ItemCode = ResolveInventoryItemCode(SrcIndex);
		if (!ItemCode.IsNone())
		{
			Component->RegisterItem(DstIndex, ItemCode); // 소모품/보유 여부 검증은 Server_SetSlot에서
		}
		return;
	}
	// 2) 같은 종류 퀵슬롯끼리: 서로 교환
	if (SrcGroup == LBSlotGroup::FromQuickSlotType(DstType))
	{
		Component->SwapSlots(DstType, SrcIndex, DstIndex);
		return;
	}

	// TODO: 스킬창 → 스킬 칸 (스킬 목록 UI가 생기면 SrcGroup/SrcIndex → AbilityTag 변환 후 RegisterSkill)
}

FName ULB_QuickSlotPanel::ResolveInventoryItemCode(int32 InventoryIndex) const
{
	// 인벤토리는 고정 슬롯이라 배열 인덱스 = 칸 번호. 배열이 칸 수보다 짧을 수 있어서 범위 확인
	if (!Component.IsValid()) return NAME_None;

	const AActor* Owner = Component->GetOwner();
	const ULB_InventoryComponent* Inv = Owner ? Owner->FindComponentByClass<ULB_InventoryComponent>() : nullptr;
	if (!Inv) return NAME_None;

	const TArray<FLB_ItemEntry>& Items = Inv->GetInventoryItems();
	return Items.IsValidIndex(InventoryIndex) ? Items[InventoryIndex].ItemCode : NAME_None;
}