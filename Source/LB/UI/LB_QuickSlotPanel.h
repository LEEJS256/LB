#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/LB_QuickSlotComponent.h"
#include "Slate/FLB_SquareBoxStyle.h"
#include "Slate/LB_QuickSlotTextStyle.h"
#include "LB_QuickSlotPanel.generated.h"

class UHorizontalBox;
class ULB_QuickSlot;

/**
 * 퀵슬롯 통합 위젯 (스킬 4칸 A/S/D/F + 아이템 4칸 Num1~4)
 * WBP_QuickSlotPanel 하나를 HUD에 붙이면 끝.
 *  - 슬롯 8개는 C++에서 동적으로 생성
 *  - PlayerState가 준비되면 스스로 컴포넌트에 바인딩
 *  - 슬롯에 드롭된 드래그(인벤토리 → 퀵슬롯, 퀵슬롯끼리 교환)를 여기서 받아 컴포넌트에 반영
 *
 * WBP 필수 구성: 이름이 정확히 SkillBox / ItemBox 인 HorizontalBox 두 개
 */
UCLASS()
class LB_API ULB_QuickSlotPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	ULB_QuickSlotPanel(const FObjectInitializer& ObjectInitializer);

	// 자동 바인딩 대신 직접 묶고 싶을 때
	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void BindToComponent(ULB_QuickSlotComponent* InComponent);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override; // 디자이너에서도 8칸이 보이도록
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override; // 스킬 쿨타임 갱신

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> SkillBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> ItemBox;

	// 칸에 표시할 단축키 글자 (인덱스 순서)
	UPROPERTY(EditDefaultsOnly, Category="LB|QuickSlot")
	TArray<FText> SkillKeyLabels;

	UPROPERTY(EditDefaultsOnly, Category="LB|QuickSlot")
	TArray<FText> ItemKeyLabels;

	UPROPERTY(EditDefaultsOnly, Category="LB|QuickSlot")
	FMargin SlotPadding = FMargin(4.f);

	UPROPERTY(EditAnywhere, Category="LB|QuickSlot|Style")
	FLB_SquareBoxStyle SlotStyle;
 
	/** 단축키 라벨 모양/위치 — 8칸 전부에 적용 (기본: 좌상단) */
	UPROPERTY(EditAnywhere, Category="LB|QuickSlot|Style")
	FLB_QuickSlotTextStyle HotkeyStyle = FLB_QuickSlotTextStyle::HotkeyDefault();
 
	/** 수량 모양/위치 — 8칸 전부에 적용 (기본: 우하단) */
	UPROPERTY(EditAnywhere, Category="LB|QuickSlot|Style")
	FLB_QuickSlotTextStyle CountStyle = FLB_QuickSlotTextStyle::CountDefault();
	
private:
	void BuildSlots(ELB_QuickSlotType Type, UHorizontalBox* Box, int32 Count, const TArray<FText>& Labels);
	void RebuildSlots();
	void TryAutoBind();

	UFUNCTION()
	void HandleSlotDropped(ELB_QuickSlotType DstType, FName SrcGroup, int32 SrcIndex, int32 DstIndex);

	// 인벤토리 칸 인덱스 → 아이템 코드. ULB_InventoryComponent 연동 지점
	FName ResolveInventoryItemCode(int32 InventoryIndex) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULB_QuickSlot>> Slots;

	TWeakObjectPtr<ULB_QuickSlotComponent> Component;
	FTimerHandle BindRetryHandle;

	// 쿨타임 갱신용: 스킬 칸만 인덱스 순서대로
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULB_QuickSlot>> SkillSlotWidgets;
 
	
};