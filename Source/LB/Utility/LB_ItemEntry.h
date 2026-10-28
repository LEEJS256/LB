// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"
#include "LB_ItemEntry.generated.h"

class ULB_DA_Equipment;
class ALB_Equipment;

// 인벤토리의 런타임용 구조체
// 아이템 소유,보유수,장착여부,고유ID 등이 정의됨
USTRUCT(BlueprintType)
struct FLB_ItemEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGuid ItemInstanceId;

	// 마스터테이블 RowName. DA는 이 코드로 조회
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 StackCount = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsEquipped = false;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	// TObjectPtr<ULB_DA_Equipment> ItemData = nullptr;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	// TSubclassOf<ALB_Equipment> EquipmentClass = nullptr;


	FLB_ItemEntry()
		: ItemInstanceId(FGuid::NewGuid())
	{}
};