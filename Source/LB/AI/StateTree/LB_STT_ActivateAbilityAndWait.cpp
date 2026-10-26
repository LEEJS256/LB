// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateTree/LB_STT_ActivateAbilityAndWait.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/Monster/LB_MeleeMonster.h"
#include "Controller/LB_MonsterAIController.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "StateTreeExecutionContext.h"
#include "Utility/LB_NativeGameplayTag.h"

FLB_STT_ActivateAbilityAndWait::FLB_STT_ActivateAbilityAndWait()
{
	// Target·사망·Timeout 감시는 State가 활성인 동안의 Task Tick만 담당
	bShouldCallTick = true;
}

EStateTreeRunStatus FLB_STT_ActivateAbilityAndWait::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	Cleanup(InstanceData);

	if (!IsValidTolerance(InstanceData.AttackRange)
		|| !IsValidTolerance(InstanceData.AttackDepthTolerance)
		|| !IsValidTolerance(InstanceData.AttackHeightTolerance)
		|| !FMath::IsFinite(InstanceData.TimeoutSeconds) || InstanceData.TimeoutSeconds <= 0.f)
	{
		return EStateTreeRunStatus::Failed;
	}

	ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	ALB_MeleeMonster* Monster = Cast<ALB_MeleeMonster>(AIController->GetPawn());
	AActor* Target = AIController->GetCombatTarget();
	if (!IsAliveMonster(Monster) || !IsAlivePlayerPawn(Target))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 진입 조건 평가 이후 위치가 바뀌었을 수 있으므로 현재 위치로 다시 확인
	if (!IsInAttackRange(InstanceData, Target->GetActorLocation() - Monster->GetActorLocation()))
	{
		return EStateTreeRunStatus::Failed;
	}

	AIController->StopMovement();

	UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	const FGameplayAbilitySpecHandle SpecHandle = Monster->FindMeleeAttackSpecHandle();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(SpecHandle) : nullptr;

	// 이미 실행 중인 Spec의 종료를 이번 실행으로 오인하지 않도록 요청하지 않음
	if (!Spec || Spec->IsActive())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 동기 종료도 수집하도록 요청 전에 구독. 이 몬스터 ASC의 이번 Handle 종료만 이번 결과에 기록
	const TSharedPtr<FLB_AbilityExecutionResult> ExecutionResult = MakeShared<FLB_AbilityExecutionResult>();
	InstanceData.ExecutionResult = ExecutionResult;
	InstanceData.SpecHandle = SpecHandle;
	InstanceData.AttackTarget = Target;
	InstanceData.ElapsedTime = 0.f;
	InstanceData.SubscribedASC = ASC;
	InstanceData.AbilityEndedHandle = ASC->OnAbilityEnded.AddStatic(&FLB_STT_ActivateAbilityAndWait::OnAbilityEnded,
		TWeakPtr<FLB_AbilityExecutionResult>(ExecutionResult), SpecHandle);

	// 방향 확정과 활성화는 RequestMeleeAttack이 한 번에 처리
	if (!Monster->RequestMeleeAttack(Target))
	{
		Cleanup(InstanceData);
		return EStateTreeRunStatus::Failed;
	}

	if (ExecutionResult->bEnded)
	{
		// 요청 안에서 이미 끝난 실행. 정상 종료와 취소 종료를 구분
		const EStateTreeRunStatus Status = ExecutionResult->bWasCancelled ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Succeeded;
		Cleanup(InstanceData);
		return Status;
	}

	// 요청은 성공했지만 실행 중이 아니면 기다릴 종료가 없음
	Spec = ASC->FindAbilitySpecFromHandle(SpecHandle);
	if (!Spec || !Spec->IsActive())
	{
		Cleanup(InstanceData);
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bOwnsExecution = true;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FLB_STT_ActivateAbilityAndWait::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.bOwnsExecution)
	{
		return EStateTreeRunStatus::Failed;
	}

	const EStateTreeRunStatus EndedStatus = ConsumeEndedResult(InstanceData);
	if (EndedStatus != EStateTreeRunStatus::Running)
	{
		return EndedStatus;
	}

	const ALB_MonsterAIController* AIController = InstanceData.AIController;
	const bool bControllerValid = IsValid(AIController) && AIController->HasAuthority();
	const APawn* Monster = bControllerValid ? AIController->GetPawn() : nullptr;
	const AActor* CurrentTarget = bControllerValid ? AIController->GetCombatTarget() : nullptr;

	// 공격 대상이 바뀌거나 사라졌거나, 어느 한쪽이 죽었으면 중단
	const bool bTargetLost = !CurrentTarget || CurrentTarget != InstanceData.AttackTarget.Get() || !IsAlivePlayerPawn(CurrentTarget);
	if (!bControllerValid || !IsAliveMonster(Monster) || bTargetLost || !InstanceData.SubscribedASC.IsValid())
	{
		CancelOwnExecution(InstanceData);
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ElapsedTime += DeltaTime;
	if (InstanceData.ElapsedTime >= InstanceData.TimeoutSeconds)
	{
		CancelOwnExecution(InstanceData);
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FLB_STT_ActivateAbilityAndWait::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Cleanup(Context.GetInstanceData(*this));
}

void FLB_STT_ActivateAbilityAndWait::OnAbilityEnded(const FAbilityEndedData& EndedData, TWeakPtr<FLB_AbilityExecutionResult> WeakResult, FGameplayAbilitySpecHandle SpecHandle)
{
	if (EndedData.AbilitySpecHandle != SpecHandle)
	{
		return;
	}

	// 실행이 정리된 뒤 늦게 온 종료나 같은 실행의 중복 종료는 무시
	const TSharedPtr<FLB_AbilityExecutionResult> Result = WeakResult.Pin();
	if (!Result || Result->bEnded)
	{
		return;
	}

	Result->bEnded = true;
	Result->bWasCancelled = EndedData.bWasCancelled;
}

void FLB_STT_ActivateAbilityAndWait::Unsubscribe(FInstanceDataType& InstanceData)
{
	if (UAbilitySystemComponent* ASC = InstanceData.SubscribedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(InstanceData.AbilityEndedHandle);
	}
	InstanceData.AbilityEndedHandle.Reset();
}

void FLB_STT_ActivateAbilityAndWait::CancelOwnExecution(FInstanceDataType& InstanceData)
{
	if (!InstanceData.bOwnsExecution)
	{
		return;
	}

	// 취소 중 동기 종료 콜백이 와도 다시 취소하지 않도록 먼저 소유 해제
	InstanceData.bOwnsExecution = false;

	const bool bAlreadyEnded = InstanceData.ExecutionResult.IsValid() && InstanceData.ExecutionResult->bEnded;
	UAbilitySystemComponent* ASC = InstanceData.SubscribedASC.Get();
	if (!bAlreadyEnded && ASC)
	{
		ASC->CancelAbilityHandle(InstanceData.SpecHandle);
	}
}

void FLB_STT_ActivateAbilityAndWait::Cleanup(FInstanceDataType& InstanceData)
{
	// 구독을 먼저 끊어 취소로 인한 종료가 결과에 다시 기록되지 않게 함
	Unsubscribe(InstanceData);
	CancelOwnExecution(InstanceData);

	InstanceData.ExecutionResult.Reset();
	InstanceData.SubscribedASC.Reset();
	InstanceData.SpecHandle = FGameplayAbilitySpecHandle();
	InstanceData.AttackTarget.Reset();
	InstanceData.ElapsedTime = 0.f;
}

EStateTreeRunStatus FLB_STT_ActivateAbilityAndWait::ConsumeEndedResult(FInstanceDataType& InstanceData)
{
	const FLB_AbilityExecutionResult* Result = InstanceData.ExecutionResult.Get();
	if (!Result || !Result->bEnded)
	{
		return EStateTreeRunStatus::Running;
	}

	// 이미 끝난 실행이므로 이후 정리에서 취소하지 않음
	InstanceData.bOwnsExecution = false;
	return Result->bWasCancelled ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Succeeded;
}

bool FLB_STT_ActivateAbilityAndWait::IsValidTolerance(float Value)
{
	return FMath::IsFinite(Value) && Value >= 0.f;
}

bool FLB_STT_ActivateAbilityAndWait::IsInAttackRange(const FInstanceDataType& InstanceData, const FVector& Delta)
{
	return FMath::Abs(Delta.X) <= InstanceData.AttackRange
		&& FMath::Abs(Delta.Y) <= InstanceData.AttackDepthTolerance
		&& FMath::Abs(Delta.Z) <= InstanceData.AttackHeightTolerance;
}

bool FLB_STT_ActivateAbilityAndWait::IsAliveMonster(const APawn* Pawn)
{
	const ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(Pawn);
	if (!IsValid(Monster) || Monster->IsDead())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(TAG_Team_Monster) && !ASC->HasMatchingGameplayTag(TAG_State_Death);
}

bool FLB_STT_ActivateAbilityAndWait::IsAlivePlayerPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	if (!IsValid(Pawn) || !Pawn->IsPlayerControlled())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC || !ASC->HasMatchingGameplayTag(TAG_Team_Player) || ASC->HasMatchingGameplayTag(TAG_State_Death))
	{
		return false;
	}

	if (!ASC->HasAttributeSetForAttribute(ULB_AttributeSet::GetHealthAttribute()))
	{
		return false;
	}

	return ASC->GetNumericAttribute(ULB_AttributeSet::GetHealthAttribute()) > 0.f;
}
