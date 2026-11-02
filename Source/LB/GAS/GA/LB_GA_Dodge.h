// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GameplayAbility.h"
#include "LB_GA_Dodge.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_GA_Dodge : public ULB_GameplayAbility
{
	GENERATED_BODY()
public:
	ULB_GA_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Dodge") float FallbackDashDistance = 200.f;
	UPROPERTY(EditDefaultsOnly, Category="Dodge") float FallbackDashDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

	// Event.Ability.End
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	FGameplayTag EndEventTag;
	
	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnEndEvent(FGameplayEventData Payload);
	
};
