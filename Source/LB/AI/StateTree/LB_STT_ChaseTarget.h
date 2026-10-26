// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "StateTreeTaskBase.h"
#include "LB_STT_ChaseTarget.generated.h"

class ALB_MonsterAIController;
struct FPathFollowingResult;

USTRUCT()
struct FLB_STT_ChaseTargetInstanceData
{
	GENERATED_BODY()

	// 서버 몬스터 AIController (Context 바인딩)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALB_MonsterAIController> AIController = nullptr;

	// Target 옆에 설 월드 X 거리. 초기값은 에셋에서 지정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float ApproachOffsetX = 0.f;

	// 공격 범위. IsTargetInAttackRange와 같은 Parameter에 바인딩
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackRange = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackDepthTolerance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackHeightTolerance = 0.f;

	// 재판정 주기(초). 0.1~0.2로 제한
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1", ClampMax = "0.2"))
	float UpdateInterval = 0.15f;

	// 목적지가 이 거리 이상 바뀔 때만 MoveTo 재요청
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float RepathDistance = 25.f;

	FTimerHandle UpdateTimerHandle;
	FDelegateHandle MoveFinishedHandle;
	FAIRequestID MoveRequestId;
	FVector LastDestination = FVector::ZeroVector;

	// Target 기준 접근 방향. +1이면 Target의 +X 쪽에 선다. 진입 시 한 번 정하고 유지
	float ApproachSide = 0.f;

	bool bMoveRequested = false;
	bool bArrived = false;
	bool bMoveFailed = false;
};

/**
 * Controller의 현재 CombatTarget 옆(Target.X ± ApproachOffsetX, Target.Y)으로 이동한다.
 * Tick 대신 UpdateInterval 타이머로 재판정하며, 목적지가 RepathDistance 이상 바뀔 때만 MoveTo를 갱신한다.
 * 월드 축별 공격 범위에 들어오면 Succeeded, Target 상실·사망 또는 이동 실패 시 Failed
 */
USTRUCT(meta = (DisplayName = "LB Chase Target"))
struct LB_API FLB_STT_ChaseTarget : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLB_STT_ChaseTargetInstanceData;

	FLB_STT_ChaseTarget();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	// 범위·Target·이동 상태를 확인하고 필요하면 MoveTo를 갱신. Running이 아니면 Task 종료
	static EStateTreeRunStatus UpdateChase(FInstanceDataType& InstanceData);

	static void OnMoveFinished(FInstanceDataType& InstanceData, FAIRequestID RequestId, const FPathFollowingResult& Result);

	// 타이머·경로 완료 구독 해제와 진행 중인 자기 이동 요청 중단
	static void Cleanup(FInstanceDataType& InstanceData);

	static bool IsInAttackRange(const FInstanceDataType& InstanceData, const FVector& Delta);

	// 유효한 몬스터이며 ASC가 Team.Monster, State.Death 미보유인지
	static bool IsAliveMonster(const APawn* Pawn);

	// 플레이어가 조종하는 Pawn이며 ASC가 Team.Player, Health>0, State.Death 미보유인지
	static bool IsAlivePlayerPawn(const AActor* Actor);
};
