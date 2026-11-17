// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Data/LB_DA_ItemInfo.h"
#include "LB_ItemTooltip.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UVerticalBox;
class ULB_TooltipLineRow;
/**
 *
 */
UCLASS()
class LB_API ULB_ItemTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	ULB_ItemTooltip(const FObjectInitializer& ObjectInitializer); 
	void SetItem(const ULB_DA_ItemInfo* Info);
	
protected:
#pragma region Bind_Widget
	// 공통
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SubNameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SellPriceText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> GradeBorder;

	// 장비 부위 텍스트. 
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotText;

#pragma  endregion 
	/** 장비 슬롯 태그 → 표시 이름 (생성자에서 기본값 채움) */
	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TMap<FGameplayTag, FText> SlotNames;

	/** 능력치 이름 → 표시 이름. 키는 Attribute 이름 (예: "Health" → "체력"). 없으면 이름 그대로 */
	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TMap<FString, FText> AttributeNames;
	
	// 타입별 효과 줄이 들어가는 곳 (장비 능력치, 소비템 효과 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> LineContainer;

	/** 한 줄을 그리는 위젯 (WBP_TooltipLine) */
	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TSubclassOf<ULB_TooltipLineRow> LineRowClass;

	/** 줄 스타일 → 값 글자색 (Normal/Bonus/Highlight) */
	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TMap<ELB_TooltipLineStyle, FLinearColor> LineColors;

	// 등급 태그 → 색상 (에디터에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TMap<FGameplayTag, FLinearColor> GradeColors;

private:
	FText GetSlotName(const ULB_DA_ItemInfo* Info) const;
	void AppendEffectLines(const ULB_DA_ItemInfo* Info, TArray<FLB_TooltipLine>& Out) const;
};
