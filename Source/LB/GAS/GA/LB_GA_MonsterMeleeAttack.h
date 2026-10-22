// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GA_NormalAttack.h"
#include "LB_GA_MonsterMeleeAttack.generated.h"

/**
 * 근접 몬스터 전용 공격 Ability. 서버에서만 실행되며 Montage·팀/사망 필터·피해·종료 정리는 부모를 그대로 사용한다.
 * 공격 방향은 활성화 전에 ALB_MeleeMonster::SetCombatFacingFromTarget으로 설정된 Hitbox 값을 유지한다.
 */
UCLASS()
class LB_API ULB_GA_MonsterMeleeAttack : public ULB_GA_NormalAttack
{
	GENERATED_BODY()

public:
	ULB_GA_MonsterMeleeAttack();

	// 부모 조건에 더해 서버 권한·ALB_MeleeMonster Avatar·생존·기존 공격 미진행을 검사
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
};
