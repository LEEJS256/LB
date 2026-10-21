// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_MonsterMeleeAttack.h"

#include "AbilitySystemComponent.h"
#include "Character/Monster/LB_MeleeMonster.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_GA_MonsterMeleeAttack::ULB_GA_MonsterMeleeAttack()
{
	// 몬스터는 클라이언트 예측 없이 서버에서만 공격. Montage·태그는 부모 경로로 복제됨
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(TAG_State_Death);
	ActivationBlockedTags.AddTag(TAG_State_Action_Attacking);
}

bool ULB_GA_MonsterMeleeAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}

	const ALB_MeleeMonster* MeleeMonster = Cast<ALB_MeleeMonster>(ActorInfo->AvatarActor.Get());
	if (!MeleeMonster || MeleeMonster->IsDead())
	{
		return false;
	}

	// ActivationBlockedTags와 별개로 ASC 상태를 직접 확인해 중복 공격 요청을 거부
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC || ASC->HasMatchingGameplayTag(TAG_State_Action_Attacking))
	{
		return false;
	}

	return true;
}
