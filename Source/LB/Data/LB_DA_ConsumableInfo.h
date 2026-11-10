// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/LB_DA_ItemInfo.h"

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	float HealAmount = 0.f;
};
