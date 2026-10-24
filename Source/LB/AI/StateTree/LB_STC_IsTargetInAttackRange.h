// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LB_STC_IsTargetInAttackRange.generated.h"

class ALB_MonsterAIController;

USTRUCT()
struct FLB_STC_IsTargetInAttackRangeInstanceData
{
	GENERATED_BODY()

	// 서버 몬스터 AIController (Context 바인딩)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALB_MonsterAIController> AIController = nullptr;

	// 월드 X축 허용 거리. 초기값은 에셋에서 지정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackRange = 0.f;

	// 월드 Y축(깊이) 허용 거리
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackDepthTolerance = 0.f;

	// 월드 Z축(높이) 허용 거리
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackHeightTolerance = 0.f;
};

/**
 * Controller의 현재 CombatTarget이 월드 축별 공격 범위 안에 있는지 검사한다.
 * abs(Delta.X)<=AttackRange, abs(Delta.Y)<=AttackDepthTolerance, abs(Delta.Z)<=AttackHeightTolerance를 모두 만족해야 한다.
 * 매 평가마다 현재 위치로 계산하며, 거리나 결과를 저장하지 않는다
 */
USTRUCT(meta = (DisplayName = "LB Is Target In Attack Range"))
struct LB_API FLB_STC_IsTargetInAttackRange : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLB_STC_IsTargetInAttackRangeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	// 유한한 0 이상 값인지
	static bool IsValidTolerance(float Value);

	// 유효한 몬스터이며 ASC가 Team.Monster, State.Death 미보유인지
	static bool IsAliveMonster(const APawn* Pawn);

	// 플레이어가 조종하는 Pawn이며 ASC가 Team.Player, Health>0, State.Death 미보유인지
	static bool IsAlivePlayerPawn(const AActor* Actor);
};
