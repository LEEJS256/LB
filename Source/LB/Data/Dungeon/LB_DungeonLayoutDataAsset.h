// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Dungeon/LB_DungeonAssemblyTypes.h"
#include "LB_DungeonLayoutDataAsset.generated.h"

/**
 * 작성용 고정 던전 Layout. 조각 배치 목록만 보유한다.
 * 런타임에는 BuildAssembly로 만든 독립 State 복사본을 사용한다.
 */
UCLASS(BlueprintType)
class LB_API ULB_DungeonLayoutDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dungeon")
	TArray<FLB_DungeonPiecePlacement> Pieces;

	// 조각 목록을 런타임 State와 같은 규격으로 검사한다.
	bool ValidateLayout(FString& OutError) const;

	// OutState를 기본값으로 초기화한 뒤, 성공 시 Pieces를 복사한 독립 State를 채운다.
	// Data Asset은 수정하지 않는다. 서버 전용 호출 보장은 호출 측(GameMode) 책임.
	bool BuildAssembly(FName InstanceKey, FLB_DungeonAssemblyState& OutState, FString& OutError) const;
};
