// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_NormalAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Component/LB_MeleeHitboxComponent.h"
#include "GameplayEffect.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "LB.h"
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

	// 명중 확정과 피해는 서버 권한. Montage 시작 전에 구독해 첫 판정 구간을 놓치지 않음
	if (HasAuthority(&ActivationInfo))
	{
		const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		if (ULB_MeleeHitboxComponent* Hitbox = Avatar ? Avatar->FindComponentByClass<ULB_MeleeHitboxComponent>() : nullptr)
		{
			Hitbox->OnMeleeHit.AddUniqueDynamic(this, &ULB_GA_NormalAttack::OnMeleeHit);
			BoundHitbox = Hitbox;
		}
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

	// 모든 종료 경로에서 명중 구독 해제 (중복 호출에도 안전)
	UnbindMeleeHit();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULB_GA_NormalAttack::OnMeleeHit(AActor* HitActor, const FHitResult& HitResult)
{
	if (!IsActive() || !HasAuthority(&CurrentActivationInfo) || !IsValid(HitActor))
	{
		return;
	}

	if (!DamageEffectClass)
	{
		UE_LOG(LogLB, Warning, TEXT("%s: DamageEffectClass is not set. Skipping damage to %s."),
			*GetName(), *GetNameSafe(HitActor));
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
	// ASC 또는 Health Attribute가 없는 Actor는 게임플레이 피해 대상이 아님
	if (!SourceASC || !TargetASC || !TargetASC->HasAttributeSetForAttribute(ULB_AttributeSet::GetHealthAttribute()))
	{
		return;
	}

	const float Attack = SourceASC->GetNumericAttribute(ULB_AttributeSet::GetAttackAttribute());
	const float Armor = TargetASC->GetNumericAttribute(ULB_AttributeSet::GetArmorAttribute());
	const float Damage = FMath::Max(1.f, Attack - Armor);

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->GetContext().AddHitResult(HitResult, true);
	// GE의 Health Add Modifier가 이 값을 그대로 더하므로 감소량을 음수로 전달
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -Damage);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ULB_GA_NormalAttack::UnbindMeleeHit()
{
	if (ULB_MeleeHitboxComponent* Hitbox = BoundHitbox.Get())
	{
		Hitbox->OnMeleeHit.RemoveDynamic(this, &ULB_GA_NormalAttack::OnMeleeHit);
	}
	BoundHitbox.Reset();
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
