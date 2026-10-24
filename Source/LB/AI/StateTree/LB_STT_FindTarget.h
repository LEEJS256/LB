// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "LB_STT_FindTarget.generated.h"

class ALB_MonsterAIController;

USTRUCT()
struct FLB_STT_FindTargetInstanceData
{
	GENERATED_BODY()

	// 서버 몬스터 AIController (Context 바인딩)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALB_MonsterAIController> AIController = nullptr;

	// 선택된 타깃. Controller의 CombatTarget과 항상 같은 값
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
};

/**
 * 살아 있는 플레이어 중 몬스터와 가장 가까운 대상을 Controller 타깃으로 지정한다.
 * 동거리면 PlayerId가 작은 쪽을 고르며, 후보가 없으면 타깃을 해제하고 Failed. 재시도는 StateTree 에셋에서 처리
 */
USTRUCT(meta = (DisplayName = "LB Find Target"))
struct LB_API FLB_STT_FindTarget : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLB_STT_FindTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	// PlayerController Pawn이며 ASC가 Team.Player, Health>0, State.Death 미보유인지
	static bool IsAlivePlayerPawn(const APawn* Pawn);
};
