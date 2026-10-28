// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "StateTreeTaskBase.h"
#include "LB_STT_ActivateAbilityAndWait.generated.h"

class ALB_MonsterAIController;
class UAbilitySystemComponent;
struct FAbilityEndedData;

// 공격 실행 한 번의 종료 결과. Delegate는 이 상태만 약참조로 기록하며 실행마다 새로 만든다
struct FLB_AbilityExecutionResult
{
	bool bEnded = false;
	bool bWasCancelled = false;
};

USTRUCT()
struct FLB_STT_ActivateAbilityAndWaitInstanceData
{
	GENERATED_BODY()

	// 서버 몬스터 AIController (Context 바인딩)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALB_MonsterAIController> AIController = nullptr;

	// 공격 범위. IsTargetInAttackRange와 같은 Parameter에 바인딩
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackRange = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackDepthTolerance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AttackHeightTolerance = 0.f;

	// 공격 종료 대기 최대 시간(초). 0 이하면 진입 실패
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.01"))
	float TimeoutSeconds = 3.f;

	// 이번 실행의 결과. 재진입 시 새로 만들어 이전 실행의 종료가 섞이지 않게 함
	TSharedPtr<FLB_AbilityExecutionResult> ExecutionResult;

	// 구독한 몬스터 ASC. Pawn이 바뀌어도 같은 ASC에서 구독을 해제
	TWeakObjectPtr<UAbilitySystemComponent> SubscribedASC;
	FDelegateHandle AbilityEndedHandle;

	// 이번 실행이 요청한 Spec과 공격 대상
	FGameplayAbilitySpecHandle SpecHandle;
	TWeakObjectPtr<AActor> AttackTarget;

	float ElapsedTime = 0.f;

	// 이 Task가 시작했고 아직 종료·취소되지 않은 실행이 있는지
	bool bOwnsExecution = false;
};

/**
 * 범위 안의 현재 CombatTarget에게 근접 공격을 한 번 요청하고, 해당 Spec Handle의 종료를 기다린다.
 * 정상 종료만 Succeeded, 요청 실패·취소 종료·Target 상실/사망·몬스터 사망·Timeout은 Failed.
 * 실패나 ExitState에서는 이 Task가 시작한 실행만 취소하며 피해는 Ability가 담당한다
 */
USTRUCT(meta = (DisplayName = "LB Activate Ability And Wait"))
struct LB_API FLB_STT_ActivateAbilityAndWait : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLB_STT_ActivateAbilityAndWaitInstanceData;

	FLB_STT_ActivateAbilityAndWait();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	// 이번 실행 결과에 지정 Handle의 첫 종료만 기록
	static void OnAbilityEnded(const FAbilityEndedData& EndedData, TWeakPtr<FLB_AbilityExecutionResult> WeakResult, FGameplayAbilitySpecHandle SpecHandle);

	// 종료 구독 해제
	static void Unsubscribe(FInstanceDataType& InstanceData);

	// 아직 남은 자기 실행만 취소. 중복 취소하지 않음
	static void CancelOwnExecution(FInstanceDataType& InstanceData);

	// 구독 해제, 자기 실행 취소, 실행 상태 초기화
	static void Cleanup(FInstanceDataType& InstanceData);

	// 종료가 기록되었으면 결과 상태를 반환하고 소유를 해제. 아니면 Running
	static EStateTreeRunStatus ConsumeEndedResult(FInstanceDataType& InstanceData);

	// 유한한 0 이상 값인지
	static bool IsValidTolerance(float Value);

	static bool IsInAttackRange(const FInstanceDataType& InstanceData, const FVector& Delta);

	// 유효한 몬스터이며 ASC가 Team.Monster, State.Death 미보유인지
	static bool IsAliveMonster(const APawn* Pawn);

	// 플레이어가 조종하는 Pawn이며 ASC가 Team.Player, Health>0, State.Death 미보유인지
	static bool IsAlivePlayerPawn(const AActor* Actor);
};
