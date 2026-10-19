// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LB_ANS_Invincible.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Utility/LB_NativeGameplayTag.h"

void ULB_ANS_Invincible::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                     float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner()))
	{
		ASC->AddLooseGameplayTag(TAG_State_Invincible);
	}
}

void ULB_ANS_Invincible::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(TAG_State_Invincible))
		{
			ASC->RemoveLooseGameplayTag(TAG_State_Invincible);
		}
	}
}
