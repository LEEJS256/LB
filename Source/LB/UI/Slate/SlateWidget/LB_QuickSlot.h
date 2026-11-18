#pragma once

#include "CoreMinimal.h"
#include "LB_SquareBox.h"
#include "Component/LB_QuickSlotComponent.h"
#include "UI/Slate/LB_QuickSlotTextStyle.h"
#include "LB_QuickSlot.generated.h"

class SLB_QuickSlot;
class UTexture2D;

/** (내 슬롯 종류, 원본 그룹, 원본 인덱스, 내 인덱스) — 퀵슬롯 칸에 드롭됐을 때 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLB_OnQuickSlotDropped, ELB_QuickSlotType, DstType, FName, SrcGroup, int32, SrcIndex, int32, DstIndex);

/**
 * 퀵슬롯 한 칸 (UMG). ULB_SquareBox 상속.
 * 보통은 ULB_QuickSlotPanel이 8개를 만들어 관리하므로 직접 배치할 일은 없다.
 * ULB_QuickSlotComponent의 OnSlotChanged를 받아 스스로 갱신하고, 드롭은 OnSlotDropped로 바깥에 알린다.
 * 디자이너에서 자식 위젯을 넣지 말 것.
 */
UCLASS()
class LB_API ULB_QuickSlot : public ULB_SquareBox
{
	GENERATED_BODY()

public:
	
	ULB_QuickSlot(const FObjectInitializer& ObjectInitializer);
	/** 패널이 생성 직후 호출 */
	UFUNCTION(BlueprintCallable, Category = "LB|QuickSlot")
	void Setup(ELB_QuickSlotType InType, int32 InIndex, const FText& InHotkeyText);

	UFUNCTION(BlueprintCallable, Category = "LB|QuickSlot")
	void BindToComponent(ULB_QuickSlotComponent* InComponent);

	UFUNCTION(BlueprintCallable, Category = "LB|QuickSlot")
	void SetTextStyles(const FLB_QuickSlotTextStyle& InHotkeyStyle, const FLB_QuickSlotTextStyle& InCountStyle);

	UFUNCTION(BlueprintCallable, Category = "LB|QuickSlot")
	void SetCooldownPercent(float InPercent);

	UPROPERTY(BlueprintAssignable, Category = "LB|QuickSlot")
	FLB_OnQuickSlotDropped OnSlotDropped;

	virtual void SynchronizeProperties() override;

protected:
	virtual const ULB_DA_ItemInfo* GetCurrentItemInfo() const override;
	

	virtual TSharedRef<SLB_SquareBox> CreateSquareBox() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|QuickSlot")
	ELB_QuickSlotType SlotType = ELB_QuickSlotType::Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|QuickSlot")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|QuickSlot")
	FText HotkeyText;

	/** 단축키 라벨 모양/위치 (기본: 좌상단) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|QuickSlot|Style")
	FLB_QuickSlotTextStyle HotkeyStyle = FLB_QuickSlotTextStyle::HotkeyDefault();
 
	/** 수량 모양/위치 (기본: 우하단) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|QuickSlot|Style")
	FLB_QuickSlotTextStyle CountStyle = FLB_QuickSlotTextStyle::CountDefault();

private:
	UFUNCTION()
	void HandleSlotChanged(ELB_QuickSlotType Type, int32 Index);

	UFUNCTION()
	void HandleQuickSlotUpdated(ELB_QuickSlotType InSlotType, int32 InSlotIndex, FName NewItemCode);

	void HandleSlotDropped(FName SrcGroup, int32 SrcIndex, int32 DstIndex);
	void HandleClearRequested();

	void RefreshFromComponent();
	void ApplyState();

	// 아이템 코드 → 아이콘/수량/툴팁. 마스터테이블(ULB_GISubsystem_Data) + 인벤토리 연동 지점
	void ResolveItemVisual(FName ItemCode, UTexture2D*& OutIcon, int32& OutCount, FText& OutInfo) const;
 
	TSharedPtr<SLB_QuickSlot> GetQuickSlotBox() const;

	TWeakObjectPtr<ULB_QuickSlotComponent> Component;

	// Slate 브러시는 UObject를 약하게 들고 있어서 GC 방지용으로 여기서 잡는다
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Icon;

	int32 Count = 0;
	FText InfoText;
	float CooldownPercent = 0.f;
};