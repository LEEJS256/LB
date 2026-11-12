// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LB_TooltipTypes.h"
#include "LB_TooltipLineRow.generated.h"

class UTextBlock;

/**
 * 툴팁 한 줄 (왼쪽 Label, 오른쪽 Value).
 * WBP_TooltipLine에서 LabelText / ValueText 이름으로 TextBlock 두 개를 만들 것.
 */
UCLASS()
class LB_API ULB_TooltipLineRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Color는 값(ValueText)에 적용. Label 색은 WBP에서 지정한 그대로 */
	void SetLine(const FLB_TooltipLine& Line, const FLinearColor& Color);

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ValueText;
};
