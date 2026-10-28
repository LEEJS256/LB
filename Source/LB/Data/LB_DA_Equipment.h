// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LB_DA_Equipment.generated.h"

class UTexture2D;

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class LB_API ULB_DA_Equipment : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// 아이템 종류 구분용 태그. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTag ItemTypeTag;

	//장비이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	FText DisplayName;

	//장비설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI", meta = (MultiLine = true))
	FText Description;
 
	// UI에서 필요할 때만 비동기 로드하도록 Soft로 둠
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	TSoftObjectPtr<UTexture2D> Icon;
 
	//인벤토리 내- 최대 소유수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|GAS")
	TArray<TSubclassOf<UGameplayEffect>> Granted_GE;
 
	// Asset Manager용 ID. 타입은 전부 "LBItem"으로 통일하고, 이름은 에셋 이름 사용
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("LBItem")), GetFName());
	}
};
