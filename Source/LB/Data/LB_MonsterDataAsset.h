// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LB_MonsterDataAsset.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/**
 * 몬스터 초기 GAS 설정. 런타임 상태는 기록하지 않는다.
 */
UCLASS()
class LB_API ULB_MonsterDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupGA;

	// 기본 Attribute, Team.Monster 태그 등을 부여하는 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupGE;
};
