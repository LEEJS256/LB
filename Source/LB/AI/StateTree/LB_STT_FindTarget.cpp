// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateTree/LB_STT_FindTarget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Controller/LB_MonsterAIController.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "StateTreeExecutionContext.h"
#include "Utility/LB_NativeGameplayTag.h"

EStateTreeRunStatus FLB_STT_FindTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ALB_MonsterAIController* AIController = InstanceData.AIController;
	if (!IsValid(AIController) || !AIController->HasAuthority())
	{
		InstanceData.TargetActor = nullptr;
		return EStateTreeRunStatus::Failed;
	}

	const APawn* Monster = AIController->GetPawn();
	UWorld* World = AIController->GetWorld();
	if (!IsValid(Monster) || !World)
	{
		AIController->SetCombatTarget(nullptr);
		InstanceData.TargetActor = nullptr;
		return EStateTreeRunStatus::Failed;
	}

	const FVector MonsterLocation = Monster->GetActorLocation();

	APawn* BestPawn = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	int32 BestPlayerId = TNumericLimits<int32>::Max();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		APawn* Pawn = PC->GetPawn();
		const APlayerState* PS = PC->PlayerState;
		if (!PS || !IsAlivePlayerPawn(Pawn))
		{
			continue;
		}

		const double DistSq = FVector::DistSquared(MonsterLocation, Pawn->GetActorLocation());
		const int32 PlayerId = PS->GetPlayerId();

		// 동거리면 PlayerId 오름차순으로 고정
		const bool bCloser = DistSq < BestDistSq && !FMath::IsNearlyEqual(DistSq, BestDistSq, UE_KINDA_SMALL_NUMBER);
		const bool bTieWin = FMath::IsNearlyEqual(DistSq, BestDistSq, UE_KINDA_SMALL_NUMBER) && PlayerId < BestPlayerId;
		if (!BestPawn || bCloser || bTieWin)
		{
			BestPawn = Pawn;
			BestDistSq = DistSq;
			BestPlayerId = PlayerId;
		}
	}

	// 타깃 소유는 Controller, Output은 같은 값을 노출만 한다
	AIController->SetCombatTarget(BestPawn);
	InstanceData.TargetActor = AIController->GetCombatTarget();

	return InstanceData.TargetActor ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

bool FLB_STT_FindTarget::IsAlivePlayerPawn(const APawn* Pawn)
{
	if (!IsValid(Pawn))
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
