// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_SkillSlotEntry.generated.h"

class UBorder;
class UTextBlock;
class ULB_InventoryBox;
class ULB_DA_SkillInfo;

DECLARE_MULTICAST_DELEGATE_OneParam(FLB_OnSkillSlotEntryClicked, FName /*SkillCode*/);

/**
 * 스킬창 카드 한 개: [아이콘] 이름 / Lv
 * - 아이콘 칸은 LB_InventoryBox 재사용 (Group "Skill", SlotIndex = AvailableSkills 인덱스) → 드래그 출발점
 * - 카드 클릭(마우스 업) → OnClicked(SkillCode)
 */
UCLASS()
class LB_API ULB_SkillSlotEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Level 0 = 미습득 */
	void SetSkill(int32 InIndex, FName InSkillCode, const ULB_DA_SkillInfo* Info, int32 Level);
	void SetSelected(bool bInSelected);

	FName GetSkillCode() const { return SkillCode; }

	FLB_OnSkillSlotEntryClicked OnClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_InventoryBox> SkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	/** (선택) 카드 배경. 선택되면 색이 바뀐다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBorder;

	UPROPERTY(EditAnywhere, Category = "Skill|Style")
	FLinearColor NormalColor = FLinearColor(0.10f, 0.13f, 0.16f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Skill|Style")
	FLinearColor SelectedColor = FLinearColor(0.11f, 0.23f, 0.27f, 1.f);

	/** 미습득 스킬 투명도 */
	UPROPERTY(EditAnywhere, Category = "Skill|Style", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float UnlearnedOpacity = 0.6f;

private:
	FName SkillCode;
};