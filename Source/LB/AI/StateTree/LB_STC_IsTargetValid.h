// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LB_STC_IsTargetValid.generated.h"

class ALB_MonsterAIController;

USTRUCT()
struct FLB_STC_IsTargetValidInstanceData
{
	GENERATED_BODY()

	// 서버 몬스터 AIController (Context 바인딩)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALB_MonsterAIController> AIController = nullptr;
};

/**
 * Controller의 현재 CombatTarget이 살아 있는 플레이어인지 검사한다.
 * 매 평가마다 Controller에서 타깃을 다시 조회하며, 타깃을 저장하거나 변경하지 않는다
 */
USTRUCT(meta = (DisplayName = "LB Is Target Valid"))
struct LB_API FLB_STC_IsTargetValid : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLB_STC_IsTargetValidInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	// 유효한 몬스터이며 ASC가 Team.Monster, State.Death 미보유인지
	static bool IsAliveMonster(const APawn* Pawn);

	// 플레이어가 조종하는 Pawn이며 ASC가 Team.Player, Health>0, State.Death 미보유인지
	static bool IsAlivePlayerPawn(const AActor* Actor);
};
