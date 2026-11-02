// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LB_SquareBox.h"

#include "LB_InventoryBox.generated.h"

class SLB_InventoryBox;
class UTexture2D;

/** (원본 그룹, 원본 인덱스, 대상 인덱스) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLB_OnInventoryBoxDropped, FName, SrcGroup, int32, SrcIndex, int32, DstIndex);
/** 우클릭 사용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnInventoryBoxUse, int32, SlotIndex);

/**
 * 인벤토리 한 칸 (UMG용). 데이터는 모르고 표시값만 받는다.
 * 디자이너에서 자식 위젯을 넣지 말 것 (내부 Overlay가 덮어써짐).
 * TileView 엔트리처럼 재사용되는 위젯 안에서 쓰므로, 값은 전부 여기 캐시해 두고
 * Slate가 다시 만들어져도 SynchronizeProperties에서 복원한다.
 */
UCLASS()
class LB_API ULB_InventoryBox : public ULB_SquareBox
{
	GENERATED_BODY()

public:
	// ---- 런타임 세터 ----
	UFUNCTION(BlueprintCallable, Category = "LB|InventoryBox")
	void SetItem(UTexture2D* InIcon, int32 InCount);

	UFUNCTION(BlueprintCallable, Category = "LB|InventoryBox")
	void ClearItem();

	UFUNCTION(BlueprintCallable, Category = "LB|InventoryBox")
	void SetInfoText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "LB|InventoryBox")
	void SetHotkeyText(const FText& InText);

	/** 풀링되는 엔트리는 갱신할 때마다 반드시 호출 */
	UFUNCTION(BlueprintCallable, Category = "LB|InventoryBox")
	void SetSlotIndex(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "LB|InventoryBox")
	void SetGroup(FName InGroup);

	UFUNCTION(BlueprintPure, Category = "LB|InventoryBox")
	bool HasItem() const { return Icon != nullptr; }

	UFUNCTION(BlueprintPure, Category = "LB|InventoryBox")
	int32 GetSlotIndex() const { return SlotIndex; }

	// ---- 이벤트 ----
	UPROPERTY(BlueprintAssignable, Category = "LB|InventoryBox")
	FLB_OnInventoryBoxDropped OnSlotDropped;

	UPROPERTY(BlueprintAssignable, Category = "LB|InventoryBox")
	FLB_OnInventoryBoxUse OnUse;

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SLB_SquareBox> CreateSquareBox() override;

	/** 인벤토리 / 퀵슬롯 구분용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|InventoryBox")
	FName Group = TEXT("Inventory");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|InventoryBox")
	FText HotkeyText;

private:
	void ApplyState();
	void HandleSlotDropped(FName SrcGroup, int32 SrcIndex, int32 DstIndex);
	void HandleUse(int32 InSlotIndex);

	TSharedPtr<SLB_InventoryBox> GetInventoryBox() const;

	// Slate 브러시는 UObject를 약하게 들고 있어서 GC 방지용으로 여기서 잡는다
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Icon;

	int32 Count = 0;
	int32 SlotIndex = INDEX_NONE;
	FText InfoText;
};