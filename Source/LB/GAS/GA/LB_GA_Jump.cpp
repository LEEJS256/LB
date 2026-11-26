// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_Jump.h"

#include "GameFramework/Character.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_GA_Jump::ULB_GA_Jump()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(TAG_Ability_Jump));

	// 평타/스킬/대쉬 중에는 해당 GA의 OwnedTags로 State.Block.Jump가 붙어 있어서 발동 불가
	ActivationBlockedTags.AddTag(TAG_State_Block_Jump);
	ActivationBlockedTags.AddTag(TAG_State_Hit);
	ActivationBlockedTags.AddTag(TAG_State_Death);
}

bool ULB_GA_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayTagContainer* SourceTags,
                                     const FGameplayTagContainer* TargetTags,
                                     FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	// CanJump()가 JumpCurrentCount < JumpMaxCount까지 체크 → 더블점프 횟수 자동 처리
	return Character && Character->CanJump();
}

void ULB_GA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Character->IsLocallyControlled())
	{
		Character->Jump();
	}


	// 버튼을 뗄 때까지 유지. 종료는 컨트롤러의 StopJumping → CancelAbilities(Ability.Jump)
}

void ULB_GA_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle,
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo,
                             bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo)
	{
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (Character->IsLocallyControlled())
			{
				Character->StopJumping();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
