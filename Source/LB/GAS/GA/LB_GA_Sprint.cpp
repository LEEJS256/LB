// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_Sprint.h"

#include "Character/LB_PlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitAttributeChange.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "PlayerState/LB_PlayerState.h"

ULB_GA_Sprint::ULB_GA_Sprint()
{
}

void ULB_GA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	
	ALB_PlayerCharacter* Character = Cast<ALB_PlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetSprinting(true);

	ALB_PlayerState* PS= Character->GetPlayerState<ALB_PlayerState>(); 

	if (PS)
	{
		PS->AddStateTag(TAG_State_Run);
	}
	
	//코스트 소모
	if (GE_SprintCost)
	{
		DrainHandle = ApplyGameplayEffectToOwner(
			Handle, ActorInfo, ActivationInfo, GE_SprintCost.GetDefaultObject(), 1.f);
	}

	
	// 스태미나 0 이하가 되면 종료
	UAbilityTask_WaitAttributeChange* Task = UAbilityTask_WaitAttributeChange::WaitForAttributeChange(
		this, ULB_AttributeSet::GetStaminaAttribute(), FGameplayTag(), FGameplayTag(), false);
	Task->OnChange.AddDynamic(this, &ULB_GA_Sprint::OnStaminaChanged);
	Task->ReadyForActivation();
}
void ULB_GA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (DrainHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(DrainHandle);
		}
		DrainHandle.Invalidate();
	}

	if (ALB_PlayerCharacter* Character = Cast<ALB_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->SetSprinting(false);
		ALB_PlayerState* PS= Character->GetPlayerState<ALB_PlayerState>(); 

		if (PS)
		{
			PS->RemoveStateTag(TAG_State_Run);
		}
	}

	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULB_GA_Sprint::OnStaminaChanged()
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetNumericAttribute(ULB_AttributeSet::GetStaminaAttribute()) <= 0.f)
		{
			CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		}
	}
}