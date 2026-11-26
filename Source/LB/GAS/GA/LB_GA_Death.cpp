// LB_GA_Death.cpp
#include "LB_GA_Death.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/LB_CharacterBase.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_GA_Death::ULB_GA_Death()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    SetAssetTags(FGameplayTagContainer(TAG_Ability_Death));

    FAbilityTriggerData Trigger;
    Trigger.TriggerTag    = TAG_Event_Death;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    // 베이스에서 State.Dead를 막아뒀으면 자기 자신은 예외로 풀기
    // ActivationBlockedTags.RemoveTag(TAG_State_Death);
}

void ULB_GA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (TriggerEventData)
        Killer = const_cast<AActor*>(TriggerEventData->Instigator.Get());

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    // 1) 진행 중인 공격/스프린트 등 전부 취소 (자기 자신 제외)
    ASC->CancelAllAbilities(this);

    // 2) State.Dead 부여 (GE라서 클라에 복제됨 → 각 클라에서 태그 이벤트 발생)
    ASC->AddLooseGameplayTag(TAG_State_Death, 1, EGameplayTagReplicationState::TagOnly);

    // 3) 사망 몽타주 (ASC가 몽타주를 복제해 줌)
    if (DeathMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task =
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                this, NAME_None, DeathMontage, 1.f, NAME_None, /*bStopWhenAbilityEnds*/ false);
        // Task->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
        // Task->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
        // Task->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
        // Task->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
        Task->ReadyForActivation();

        UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, DeathMontage->GetPlayLength());
        Wait->OnFinish.AddDynamic(this, &ThisClass::OnMontageEnded);
        Wait->ReadyForActivation();
    }
    else
    {
        FinishDeath();
    }
}

void ULB_GA_Death::OnMontageEnded()
{
    FinishDeath();
}

void ULB_GA_Death::FinishDeath()
{
    if (ALB_CharacterBase* Char = Cast<ALB_CharacterBase>(GetAvatarActorFromActorInfo()))
    {
        Char->OnDeathFinished(Killer.Get());
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}