// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateTree/LB_STC_IsTargetInAttackRange.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/Monster/LB_MonsterBase.h"
#include "Controller/LB_MonsterAIController.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "StateTreeExecutionContext.h"
#include "Utility/LB_NativeGameplayTag.h"

bool FLB_STC_IsTargetInAttackRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValidTolerance(InstanceData.AttackRange)
		|| !IsValidTolerance(InstanceData.AttackDepthTolerance)
		|| !IsValidTolerance(InstanceData.AttackHeightTolerance))
	{
		return false;
	}

	const ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		return false;
	}

	const APawn* Monster = AIController->GetPawn();
	if (!IsAliveMonster(Monster))
	{
		return false;
	}

	const AActor* Target = AIController->GetCombatTarget();
	if (!IsAlivePlayerPawn(Target))
	{
		return false;
	}

	// 구형 거리가 아닌 월드 축별로 현재 위치를 비교
	const FVector Delta = Target->GetActorLocation() - Monster->GetActorLocation();
	return FMath::Abs(Delta.X) <= InstanceData.AttackRange
		&& FMath::Abs(Delta.Y) <= InstanceData.AttackDepthTolerance
		&& FMath::Abs(Delta.Z) <= InstanceData.AttackHeightTolerance;
}

bool FLB_STC_IsTargetInAttackRange::IsValidTolerance(float Value)
{
	return FMath::IsFinite(Value) && Value >= 0.f;
}

bool FLB_STC_IsTargetInAttackRange::IsAliveMonster(const APawn* Pawn)
{
	const ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(Pawn);
	if (!IsValid(Monster) || Monster->IsDead())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(TAG_Team_Monster) && !ASC->HasMatchingGameplayTag(TAG_State_Death);
}

bool FLB_STC_IsTargetInAttackRange::IsAlivePlayerPawn(const AActor* Actor)
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
