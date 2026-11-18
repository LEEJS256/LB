#pragma once

#include "CoreMinimal.h"
#include "Input/DragAndDrop.h"
#include "UI/UI_Utility/LB_EquipSlotKey.h"   
#include "Styling/SlateBrush.h"

// 인벤토리/퀵슬롯/장비 슬롯이 공용으로 쓰는 드래그앤드롭 페이로드 + 슬롯 델리게이트

/** (원본 그룹, 원본 인덱스, 대상 인덱스) — 박스끼리 드래그앤드롭 결과 */
DECLARE_DELEGATE_ThreeParams(FLB_OnSlotDropped, FName /*SrcGroup*/, int32 /*SrcIndex*/, int32 /*DstIndex*/);
DECLARE_DELEGATE_OneParam(FLB_OnSlotUse, int32 /*SlotIndex*/);
DECLARE_DELEGATE_TwoParams(FLB_OnEquipDroppedToSlot, const FLB_EquipSlotKey& /*Key*/, int32 /*DstIndex*/);

/** 박스 간 드래그 정보. Group으로 인벤토리<->퀵슬롯 구분. 아이콘 브러시는 복사해서 보유 */
class LB_API FLB_SlotDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FLB_SlotDragDropOp, FDragDropOperation)

	FName SourceGroup;
	int32 SourceIndex = INDEX_NONE;
	FSlateBrush IconBrush;
	FLB_EquipSlotKey EquipKey;

	static TSharedRef<FLB_SlotDragDropOp> Create(FName InGroup, int32 InIndex, const FSlateBrush* InIcon);
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
};
