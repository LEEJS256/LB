// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateTree/LB_STC_IsTargetValid.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/Monster/LB_MonsterBase.h"
#include "Controller/LB_MonsterAIController.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "StateTreeExecutionContext.h"
#include "Utility/LB_NativeGameplayTag.h"

bool FLB_STC_IsTargetValid::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		return false;
	}

	if (!IsAliveMonster(AIController->GetPawn()))
	{
		return false;
	}

	// 과거 Output이 아닌 Controller의 현재 타깃을 검사
	return IsAlivePlayerPawn(AIController->GetCombatTarget());
}

bool FLB_STC_IsTargetValid::IsAliveMonster(const APawn* Pawn)
{
	const ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(Pawn);
	if (!IsValid(Monster) || Monster->IsDead())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(TAG_Team_Monster);
}

bool FLB_STC_IsTargetValid::IsAlivePlayerPawn(const AActor* Actor)
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
