// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LB_SkillEntry.generated.h"

/** 플레이어가 배운 스킬 한 개 (런타임, 복제용) */
USTRUCT(BlueprintType)
struct FLB_SkillEntry
{
	GENERATED_BODY()

	// 마스터 스킬 DT의 RowName. DA는 이 코드로 조회
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FName SkillCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 Level = 0;

	FLB_SkillEntry() = default;
	FLB_SkillEntry(FName InCode, int32 InLevel) : SkillCode(InCode), Level(InLevel) {}
};
