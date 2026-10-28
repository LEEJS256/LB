// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateTree/LB_STC_IsMonsterDead.h"

#include "Character/Monster/LB_MonsterBase.h"
#include "Controller/LB_MonsterAIController.h"
#include "StateTreeExecutionContext.h"

bool FLB_STC_IsMonsterDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		return false;
	}

	const ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(AIController->GetPawn());
	return IsValid(Monster) && Monster->IsDead();
}
