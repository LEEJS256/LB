// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_StatRow.generated.h"

class UTextBlock;

/** 능력치 한 줄: 이름 / 값 / (+보너스) */
UCLASS()
class LB_API ULB_StatRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bonus = CurrentValue - BaseValue (장비/버프 GE로 바뀐 만큼). 0이면 숨김 */
	void SetStat(const FText& Label, const FText& Value, float Bonus, int32 Decimals);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BonusText;

	UPROPERTY(EditAnywhere, Category = "Stat")
	FLinearColor PositiveColor = FLinearColor(0.24f, 0.86f, 0.59f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Stat")
	FLinearColor NegativeColor = FLinearColor(0.90f, 0.28f, 0.30f, 1.f);
};
