// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LB_STC_IsMonsterDead.generated.h"

class ALB_MonsterAIController;

USTRUCT()
struct FLB_STC_IsMonsterDeadInstanceData
{
	GENERATED_BODY()

	// 서버 몬스터 AIController (Context 바인딩)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALB_MonsterAIController> AIController = nullptr;
};

/**
 * Controller가 조종하는 몬스터의 ASC가 State.Death를 보유했는지 검사한다.
 * 별도 사망 상태를 저장하지 않고 매 평가마다 ALB_MonsterBase::IsDead()를 조회한다
 */
USTRUCT(meta = (DisplayName = "LB Is Monster Dead"))
struct LB_API FLB_STC_IsMonsterDead : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLB_STC_IsMonsterDeadInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
