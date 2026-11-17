#pragma once

#include "CoreMinimal.h"
#include "LB_SquareBox.h"
#include "UI/UI_Utility/LB_EquipSlotKey.h"
#include "LB_EquipSlot.generated.h"

/** (대상 장비 슬롯, 원본 그룹, 원본 인덱스) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLB_OnEquipBoxDropped, FLB_EquipSlotKey, DstKey, FName, SrcGroup, int32, SrcIndex);
/** 우클릭 해제 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnEquipBoxUnequip, FLB_EquipSlotKey, Key);
/** 드롭 가능 여부 판정 (C++ 전용, 패널에서 바인딩) */
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FLB_EquipBoxCanAccept, const FLB_EquipSlotKey& /*DstKey*/, FName /*SrcGroup*/, int32 /*SrcIndex*/);

class SLB_EquipSlot;
class UPaperSprite;

/**
 * 장비 한 칸 (UMG용). 데이터는 모르고 표시값만 받는다.
 * 디자이너에서 자식 위젯을 넣지 말 것 (내부 Overlay가 덮어써짐).
 * 값은 전부 여기 캐시해 두고, Slate가 다시 만들어져도 SynchronizeProperties에서 복원한다.
 */
UCLASS()
class LB_API ULB_EquipSlot : public ULB_SquareBox
{
	GENERATED_BODY()

public:
	ULB_EquipSlot(const FObjectInitializer& ObjectInitializer);

	// ---- 런타임 세터 ----
	UFUNCTION(BlueprintCallable, Category = "LB|EquipSlot")
	void SetItem(UPaperSprite* InIcon, FName InItemCode);

	UFUNCTION(BlueprintCallable, Category = "LB|EquipSlot")
	void ClearItem();

	UFUNCTION(BlueprintCallable, Category = "LB|EquipSlot")
	void SetEmptyIcon(UPaperSprite* InIcon);

	UFUNCTION(BlueprintPure, Category = "LB|EquipSlot")
	bool HasItem() const { return Icon != nullptr; }

	UFUNCTION(BlueprintPure, Category = "LB|EquipSlot")
	FLB_EquipSlotKey GetSlotKey() const { return SlotKey; }

	UFUNCTION(BlueprintPure, Category = "LB|EquipSlot")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "LB|EquipSlot")
	FName GetItemCode() const { return ItemCode; }

	// ---- 이벤트 ----
	UPROPERTY(BlueprintAssignable, Category = "LB|EquipSlot")
	FLB_OnEquipBoxDropped OnEquipDropped;

	UPROPERTY(BlueprintAssignable, Category = "LB|EquipSlot")
	FLB_OnEquipBoxUnequip OnUnequip;

	/** 바인딩 안 하면 모든 드롭 허용 (하이라이트용, 최종 판정은 서버) */
	FLB_EquipBoxCanAccept CanAcceptDrop;

	virtual void SynchronizeProperties() override;

protected:
	virtual const ULB_DA_ItemInfo* GetCurrentItemInfo() const override;
	virtual TSharedRef<SLB_SquareBox> CreateSquareBox() override;

	/** 슬롯 종류 (Equip.Slot.Head / Body / Hands / Feet / Weapon / Accessory) + 악세 인덱스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|EquipSlot")
	FLB_EquipSlotKey SlotKey;

	/** 장비창 안에서 칸마다 겹치지 않게 지정 (드래그 SourceIndex로 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|EquipSlot")
	int32 SlotIndex = INDEX_NONE;

	/** 드래그 소스 구분용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|EquipSlot")
	FName Group = TEXT("Equip");

	/** 빈 칸일 때 흐리게 표시할 실루엣 (투구, 반지 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|EquipSlot")
	TObjectPtr<UPaperSprite> EmptyIcon;

private:
	void ApplyState();
	void HandleEquipDropped(const FLB_EquipSlotKey& DstKey, FName SrcGroup, int32 SrcIndex);
	void HandleUnequip(const FLB_EquipSlotKey& Key);
	bool HandleCanAccept(const FLB_EquipSlotKey& DstKey, FName SrcGroup, int32 SrcIndex) const;

	TSharedPtr<SLB_EquipSlot> GetEquipSlot() const;

	// Slate 브러시는 UObject를 약하게 들고 있어서 GC 방지용으로 여기서 잡는다
	UPROPERTY(Transient)
	TObjectPtr<UPaperSprite> Icon;

	FName ItemCode = NAME_None;
};
