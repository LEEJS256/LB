// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_Dodge.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

ULB_GA_Dodge::ULB_GA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void ULB_GA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, DodgeMontage, 1.f);

	Task->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
	Task->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
	Task->ReadyForActivation();

	// 노티파이 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EndEventTag);
	WaitTask->EventReceived.AddDynamic(this, &ThisClass::OnEndEvent);
	WaitTask->ReadyForActivation();
}

void ULB_GA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                              bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULB_GA_Dodge::OnMontageFinished()
{
 	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULB_GA_Dodge::OnEndEvent(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
