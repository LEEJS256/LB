#include "GAS/GA/LB_GA_BaseSkill.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Data/LB_DA_SkillInfo.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_GA_BaseSkill::ULB_GA_BaseSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void ULB_GA_BaseSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	// 스태미나 소모 + 쿨타임 시작
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SkillMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage,
			                                                               MontagePlayRate);
		Task->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
		Task->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
		Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
		Task->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
		Task->ReadyForActivation();
	}
	else if (!bEndManually)
	{
		// 몽타주가 없는데 수동 종료도 아니면 바로 끝낸다 (GA가 안 끝나서 다음 발동이 막히는 것 방지)
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	OnSkillActivated(ActorInfo);
}

float ULB_GA_BaseSkill::GetDamageMultiplier() const
{
	const ULB_DA_SkillInfo* Info = Cast<ULB_DA_SkillInfo>(GetCurrentSourceObject());

	float Percent = 0.f;
	if (Info && Info->GetDamagePercent(GetSkillLevel(), Percent))
	{
		return Percent * 0.01f;
	}
	return 1.f;
}

void ULB_GA_BaseSkill::ApplyDamageTo(AActor* Target)
{
	if (!DamageEffectClass || !Target || !HasAuthority(&CurrentActivationInfo))
		return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC)
		return;

	bool bFound = false;
	const float Attack = SourceASC->GetGameplayAttributeValue(ULB_AttributeSet::GetAttackAttribute(), bFound);
	const float Damage = (bFound ? Attack : 0.f) * GetDamageMultiplier();

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetSkillLevel());
	if (Spec.IsValid())
	{
		// NormalAttack의 피해 GE와 같은 규칙. 부호(감소)는 GE 쪽 설정에 맞춰야 함
		Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ULB_GA_BaseSkill::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULB_GA_BaseSkill::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}