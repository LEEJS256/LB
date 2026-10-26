// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateTree/LB_STT_ChaseTarget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/Monster/LB_MeleeMonster.h"
#include "Controller/LB_MonsterAIController.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"
#include "Utility/LB_NativeGameplayTag.h"

FLB_STT_ChaseTarget::FLB_STT_ChaseTarget()
{
	// 재판정은 타이머가 담당
	bShouldCallTick = false;
}

EStateTreeRunStatus FLB_STT_ChaseTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	Cleanup(InstanceData);
	InstanceData.bMoveRequested = false;
	InstanceData.bArrived = false;
	InstanceData.bMoveFailed = false;
	InstanceData.ApproachSide = 0.f;

	ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	const APawn* Monster = AIController->GetPawn();
	const AActor* Target = AIController->GetCombatTarget();
	if (!IsAliveMonster(Monster) || !IsAlivePlayerPawn(Target))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 진입 시점에 몬스터가 있는 쪽으로 고정해 Target이 X를 오가도 좌우가 반복 반전되지 않게 함
	InstanceData.ApproachSide = Monster->GetActorLocation().X >= Target->GetActorLocation().X ? 1.f : -1.f;

	const EStateTreeRunStatus FirstStatus = UpdateChase(InstanceData);
	if (FirstStatus != EStateTreeRunStatus::Running)
	{
		Cleanup(InstanceData);
		return FirstStatus;
	}

	if (UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent())
	{
		InstanceData.MoveFinishedHandle = PathFollowing->OnRequestFinished.AddLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](FAIRequestID RequestId, const FPathFollowingResult& Result)
			{
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();
				if (FInstanceDataType* Data = StrongContext.GetInstanceDataPtr<FInstanceDataType>())
				{
					OnMoveFinished(*Data, RequestId, Result);
				}
			});
	}

	if (UWorld* World = AIController->GetWorld())
	{
		const float Interval = FMath::Clamp(InstanceData.UpdateInterval, 0.1f, 0.2f);
		World->GetTimerManager().SetTimer(InstanceData.UpdateTimerHandle,
			FTimerDelegate::CreateLambda([WeakContext = Context.MakeWeakExecutionContext()]()
			{
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();
				FInstanceDataType* Data = StrongContext.GetInstanceDataPtr<FInstanceDataType>();
				if (!Data)
				{
					return;
				}

				const EStateTreeRunStatus Status = UpdateChase(*Data);
				if (Status == EStateTreeRunStatus::Succeeded)
				{
					StrongContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
				}
				else if (Status == EStateTreeRunStatus::Failed)
				{
					StrongContext.FinishTask(EStateTreeFinishTaskType::Failed);
				}
			}),
			Interval, true);
	}

	return EStateTreeRunStatus::Running;
}

void FLB_STT_ChaseTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Cleanup(Context.GetInstanceData(*this));
}

EStateTreeRunStatus FLB_STT_ChaseTarget::UpdateChase(FInstanceDataType& InstanceData)
{
	ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	const APawn* Monster = AIController->GetPawn();
	if (!IsAliveMonster(Monster))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 과거 Output이 아닌 Controller의 현재 타깃 기준
	const AActor* Target = AIController->GetCombatTarget();
	if (!IsAlivePlayerPawn(Target))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 추적 중에는 Target 쪽을 바라봄. 접근 측면이 고정되어 있어 이동 중 좌우가 반복 반전되지 않음
	if (ALB_MeleeMonster* MeleeMonster = Cast<ALB_MeleeMonster>(AIController->GetPawn()))
	{
		MeleeMonster->SetCombatFacingFromTarget(AIController->GetCombatTarget());
	}

	const FVector TargetLocation = Target->GetActorLocation();
	if (IsInAttackRange(InstanceData, TargetLocation - Monster->GetActorLocation()))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (InstanceData.bMoveFailed)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector Destination(TargetLocation.X + InstanceData.ApproachSide * InstanceData.ApproachOffsetX, TargetLocation.Y, TargetLocation.Z);
	const bool bDestinationMoved = !InstanceData.bMoveRequested
		|| FVector::DistSquared(Destination, InstanceData.LastDestination) >= FMath::Square(InstanceData.RepathDistance);

	if (!bDestinationMoved)
	{
		// 목적지에 도착했는데도 범위 밖(높이 차 등)이면 같은 요청을 반복하지 않고 실패로 넘김
		return InstanceData.bArrived ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Running;
	}

	const EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(Destination, -1.f, false, true, true, false);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.LastDestination = Destination;
	InstanceData.bMoveRequested = true;
	InstanceData.bArrived = Result == EPathFollowingRequestResult::AlreadyAtGoal;
	InstanceData.MoveRequestId = AIController->GetCurrentMoveRequestID();
	return EStateTreeRunStatus::Running;
}

void FLB_STT_ChaseTarget::OnMoveFinished(FInstanceDataType& InstanceData, FAIRequestID RequestId, const FPathFollowingResult& Result)
{
	// 목적지 갱신으로 중단된 이전 요청은 무시
	if (!InstanceData.bMoveRequested || RequestId != InstanceData.MoveRequestId)
	{
		return;
	}

	if (Result.IsSuccess())
	{
		InstanceData.bArrived = true;
	}
	else
	{
		InstanceData.bMoveFailed = true;
	}
}

void FLB_STT_ChaseTarget::Cleanup(FInstanceDataType& InstanceData)
{
	ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController))
	{
		InstanceData.UpdateTimerHandle.Invalidate();
		InstanceData.MoveFinishedHandle.Reset();
		InstanceData.bMoveRequested = false;
		return;
	}

	if (UWorld* World = AIController->GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstanceData.UpdateTimerHandle);
	}

	if (UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent())
	{
		PathFollowing->OnRequestFinished.Remove(InstanceData.MoveFinishedHandle);
	}
	InstanceData.MoveFinishedHandle.Reset();

	// 다른 이동 요청은 건드리지 않고 이 Task가 낸 요청만 중단
	if (InstanceData.bMoveRequested && AIController->GetCurrentMoveRequestID() == InstanceData.MoveRequestId)
	{
		AIController->StopMovement();
	}
	InstanceData.bMoveRequested = false;
}

bool FLB_STT_ChaseTarget::IsInAttackRange(const FInstanceDataType& InstanceData, const FVector& Delta)
{
	return FMath::Abs(Delta.X) <= InstanceData.AttackRange
		&& FMath::Abs(Delta.Y) <= InstanceData.AttackDepthTolerance
		&& FMath::Abs(Delta.Z) <= InstanceData.AttackHeightTolerance;
}

bool FLB_STT_ChaseTarget::IsAliveMonster(const APawn* Pawn)
{
	const ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(Pawn);
	if (!IsValid(Monster) || Monster->IsDead())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(TAG_Team_Monster) && !ASC->HasMatchingGameplayTag(TAG_State_Death);
}

bool FLB_STT_ChaseTarget::IsAlivePlayerPawn(const AActor* Actor)
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
