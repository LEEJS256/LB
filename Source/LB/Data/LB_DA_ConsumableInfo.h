// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Data/LB_TooltipTypes.h"

#include "GameplayTagContainer.h"
#include "LB_DA_ConsumableInfo.generated.h"

/**
 *
 */
UCLASS()
class LB_API ULB_DA_ConsumableInfo : public ULB_DA_ItemInfo
{
	GENERATED_BODY()

public:
	ULB_DA_ConsumableInfo() { ItemTypeTag = TAG_Item_Consumable; }

	virtual void GetTooltipLines(TArray<FLB_TooltipLine>& Out) const override;

	/** 툴팁에 표시할 효과 목록 (HP/MP/스태미나 회복량 등). 표시용이고 실제 효과는 Granted_GE가 적용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	TArray<FLB_StatBonus> Effects;

	/** (구) Effects가 비어 있을 때만 HP 회복 줄로 표시. 에셋 값을 Effects로 옮긴 뒤 삭제해도 됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	float HealAmount = 0.f;
};
