// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "LB_DungeonAssemblyTypes.generated.h"

/**
 * 던전 조각 하나의 확정 배치 정보.
 * 작성용 Layout과 실행용 Assembly State가 같은 규격을 공유한다.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonPiecePlacement
{
	GENERATED_BODY()

	// 조각 식별자. 영문/숫자/underscore만 허용, None/중복 금지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName PieceId = NAME_None;

	// 스트리밍할 맵 조각. 같은 맵을 다른 PieceId로 재사용할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	TSoftObjectPtr<UWorld> Level;

	// 배치 Transform. 유한한 위치, 회전 0, Scale 1만 허용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FTransform Transform = FTransform::Identity;
};

/**
 * 런타임에 확정된 던전 조립 상태.
 * Layout Data Asset 참조 없이 독립 복사본으로 유지된다.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonAssemblyState
{
	GENERATED_BODY()

	// 작은 고정 던전 프로토타입 경계. 대형 던전 전송 방식은 별도 검토한다.
	static constexpr int32 MaxPieces = 32;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName InstanceKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	TArray<FLB_DungeonPiecePlacement> Pieces;

	// InstanceKey와 Pieces 전체를 검사한다. 실패 시 OutError에 원인을 남긴다.
	bool Validate(FString& OutError) const;

	// Layout과 State가 공유하는 조각 목록 규격 검사.
	static bool ValidatePieces(const TArray<FLB_DungeonPiecePlacement>& InPieces, FString& OutError);

	// 영문/숫자/underscore로만 이루어진 None이 아닌 이름인지 검사.
	static bool IsValidIdentifier(const FName& InName);
};
