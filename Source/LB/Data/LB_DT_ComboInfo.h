// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Utility/LB_NativeGameplayTag.h"
#include "LB_DT_ComboInfo.generated.h"

USTRUCT(BlueprintType)
struct FLB_DT_ComboInfo : public FTableRowBase
{
	GENERATED_BODY()

	// 현재 콤보 상태 (최초 진입은 Combo.None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag FromState;

	// 다음 상태로 넘어가는 입력
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InputTag = TAG_Input_Normal;

	// 전이될 상태
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag ToState;

	// ToState에서 재생할 몽타주 섹션 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName MontageSection;
};
