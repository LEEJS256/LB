// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GameplayAbility.h"
#include "LB_GA_Jump.generated.h"

/**
 * 점프 / 더블점프 GA
 * - 입력 1회 = 발동 1회. 실제 점프는 ACharacter::Jump() (CMC 예측 그대로 사용)
 * - 점프 횟수는 CMC의 JumpMaxCount로 관리 (더블점프 = 2)
 * - 버튼을 떼면 컨트롤러가 CancelAbilities로 종료 → EndAbility에서 StopJumping (홀드 점프 높이)
 * - 공중 상태(State.Movement.InAir)는 이 GA가 아니라 캐릭터의 OnMovementModeChanged에서 관리
 */
UCLASS()
class LB_API ULB_GA_Jump : public ULB_GameplayAbility
{
	GENERATED_BODY()

public:
	ULB_GA_Jump();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                                const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr,
	                                const FGameplayTagContainer* TargetTags = nullptr,
	                                FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility, bool bWasCancelled) override;
};
