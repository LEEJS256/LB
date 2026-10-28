// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/LB_DA_ItemInfo.h"
#include "LB_DT_MasterItem.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DT_MasterItem : public FTableRowBase
{
	GENERATED_BODY()

	//아이템코드는 DT의 로우로

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<ULB_DA_ItemInfo> ItemInfo;
};
