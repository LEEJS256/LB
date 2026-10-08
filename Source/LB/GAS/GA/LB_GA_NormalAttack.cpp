// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_NormalAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Component/LB_MeleeHitboxComponent.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_GA_NormalAttack::ULB_GA_NormalAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(TAG_ATK_Normal));

	// ActivationOwnedTags는 활성화 시 부여되고 EndAbility에서 자동 제거되므로 어떤 종료 경로에서도 남지 않음
	ActivationOwnedTags.AddTag(TAG_State_Action_Attacking);
	ActivationOwnedTags.AddTag(TAG_State_Movement_FacingLocked);
	ActivationOwnedTags.AddTag(TAG_State_Movement_Blocked);
}

void ULB_GA_NormalAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, MontagePlayRate);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ULB_GA_NormalAttack::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ULB_GA_NormalAttack::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ULB_GA_NormalAttack::OnMontageCancelled);
	// 재생 실패(AnimInstance 없음 등) 시에도 OnCancelled가 호출되어 즉시 종료됨
	MontageTask->OnCancelled.AddDynamic(this, &ULB_GA_NormalAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void ULB_GA_NormalAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Montage가 NotifyEnd 전에 중단되거나 Ability가 취소돼도 판정이 남지 않도록 정리
	// 이미 종료된 Ability에 대한 중복 EndAbility 호출(Blend Out 후 Completed 등)은 건너뜀
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		if (ULB_MeleeHitboxComponent* Hitbox = Avatar ? Avatar->FindComponentByClass<ULB_MeleeHitboxComponent>() : nullptr)
		{
			Hitbox->ForceEndHitWindow();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULB_GA_NormalAttack::OnMontageFinished()
{
	// Blend Out 이후 Completed가 이어서 오더라도 EndAbility는 이미 비활성 상태면 무시됨
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULB_GA_NormalAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
