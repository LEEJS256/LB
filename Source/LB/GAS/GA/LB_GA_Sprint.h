// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GameplayAbility.h"
#include "LB_GA_Sprint.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_GA_Sprint : public ULB_GameplayAbility
{
	GENERATED_BODY()

public:
	ULB_GA_Sprint();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Ability|Cost")
	TSubclassOf<UGameplayEffect> GE_SprintCost;
	//대충 스프린트 가중치
	UPROPERTY(EditDefaultsOnly, Category="Sprint")
	TSubclassOf<UGameplayEffect> GE_SprintWeight;

private:
	UFUNCTION()
	void OnStaminaChanged();

	FActiveGameplayEffectHandle DrainHandle;
	
	FActiveGameplayEffectHandle SpeedHandle;
};
