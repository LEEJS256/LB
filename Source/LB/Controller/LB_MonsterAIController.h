// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "LB_MonsterAIController.generated.h"

class UAbilitySystemComponent;
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

	// 서버 전용. World의 실행 중인 모든 몬스터 StateTree에 Event.AI.PlayerAvailable 전송
	// 타깃이 없어 NoTarget에서 대기 중인 몬스터만 이 이벤트로 재탐색한다
	static void NotifyPlayerAvailable(UWorld* World);

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

	// 서버 전용. 몬스터 ASC의 State.Death NewOrRemoved 구독. 같은 ASC 중복 구독은 무시, 다른 ASC면 이전 구독 해제
	void BindDeathTagEvent(UAbilitySystemComponent* ASC);
	void UnbindDeathTagEvent();
	void OnDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 사망 정리. 이동 정지, Target 해제, 근접 공격 Spec 취소, Hitbox 강제 종료를 멱등 수행
	// Dead 상태 선택은 StateTree에 맡기므로 Logic은 정지하지 않음
	void HandleMonsterDeath();

	// 타깃은 Controller만 소유하며 다른 객체에 복사하지 않음
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	// 사망 태그를 구독 중인 ASC와 핸들
	TWeakObjectPtr<UAbilitySystemComponent> DeathTagBoundASC;
	FDelegateHandle DeathTagHandle;

	FTimerHandle StartRetryHandle;
	int32 StartRetryCount = 0;
	bool bCombatLogicStarted = false;

	// Pawn 초기화 대기 최대 틱 수
	static constexpr int32 MaxStartRetryCount = 30;
};
