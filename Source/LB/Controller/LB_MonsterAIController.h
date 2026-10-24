// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LB_MonsterAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;
class ALB_MonsterBase;

/**
 * 근접 몬스터용 서버 AI 컨트롤러.
 * StateTree는 자동 시작하지 않고 TryStartCombatLogic()에서 조건이 모두 맞을 때 한 번만 시작한다.
 */
UCLASS()
class LB_API ALB_MonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALB_MonsterAIController();

	// Controller BeginPlay, 몬스터 BeginPlay/ASC ActorInfo, Authority, 생존, Tree 지정이 모두 충족되면 Logic 시작
	void TryStartCombatLogic();

	UStateTreeAIComponent* GetStateTreeAIComponent() const { return StateTreeAIComponent; }

	// 현재 전투 타깃. 무효(파괴/GC)된 대상은 nullptr
	UFUNCTION(BlueprintPure, Category = "LB|AI")
	AActor* GetCombatTarget() const;

	// 서버 전용. nullptr은 타깃 해제, 자기 자신(Controller/Pawn)은 무시
	void SetCombatTarget(AActor* NewTarget);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, Category = "LB|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	// BP_MonsterAIController Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category = "LB|AI")
	TObjectPtr<UStateTree> CombatStateTree;

private:
	// 몬스터 초기화(BeginPlay/ASC ActorInfo)가 끝났는지
	bool IsMonsterReady(const ALB_MonsterBase* Monster) const;

	void ScheduleStartRetry();
	void OnStartRetryTick();

	// 재시도 예약, Logic, 이동 정리
	void StopCombatLogic(const FString& Reason);

	// 타깃은 Controller만 소유하며 다른 객체에 복사하지 않음
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	FTimerHandle StartRetryHandle;
	int32 StartRetryCount = 0;
	bool bCombatLogicStarted = false;

	// Pawn 초기화 대기 최대 틱 수
	static constexpr int32 MaxStartRetryCount = 30;
};
