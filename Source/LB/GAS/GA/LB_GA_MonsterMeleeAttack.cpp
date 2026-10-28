// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA/LB_GA_MonsterMeleeAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Character/Monster/LB_MeleeMonster.h"
#include "Component/LB_MeleeHitboxComponent.h"
#include "GameplayEffect.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "LB.h"
#include "Utility/LB_NativeGameplayTag.h"
#include "Utility/LB_TeamTagUtility.h"

ULB_GA_MonsterMeleeAttack::ULB_GA_MonsterMeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 몬스터는 클라이언트 예측 없이 서버에서만 공격. Montage·태그는 ASC 경로로 복제됨
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// ALB_MeleeMonster::FindMeleeAttackSpecHandle이 정확 일치로 검색하는 계약 태그
	SetAssetTags(FGameplayTagContainer(TAG_Ability_Monster_Attack_Melee));

	// ActivationOwnedTags는 활성화 시 부여되고 EndAbility에서 자동 제거되므로 어떤 종료 경로에서도 남지 않음
	ActivationOwnedTags.AddTag(TAG_State_Action_Attacking);
	ActivationOwnedTags.AddTag(TAG_State_Movement_FacingLocked);
	ActivationOwnedTags.AddTag(TAG_State_Movement_Blocked);

	ActivationBlockedTags.AddTag(TAG_State_Death);
	ActivationBlockedTags.AddTag(TAG_State_Action_Attacking);
}

bool ULB_GA_MonsterMeleeAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}

	const ALB_MeleeMonster* MeleeMonster = Cast<ALB_MeleeMonster>(ActorInfo->AvatarActor.Get());
	if (!MeleeMonster || MeleeMonster->IsDead())
	{
		return false;
	}

	// ActivationBlockedTags와 별개로 ASC 상태를 직접 확인해 중복 공격 요청을 거부
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC || ASC->HasMatchingGameplayTag(TAG_State_Action_Attacking))
	{
		return false;
	}

	return true;
}

void ULB_GA_MonsterMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!AttackMontage)
	{
		UE_LOG(LogLB, Warning, TEXT("%s: AttackMontage is not set."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 명중 확정과 피해는 서버 권한. Montage 시작 전에 구독해 첫 판정 구간을 놓치지 않음
	// 공격 방향은 활성화 전에 SetCombatFacingFromTarget이 Hitbox에 적용한 값을 그대로 사용
	const ALB_MeleeMonster* MeleeMonster = Cast<ALB_MeleeMonster>(ActorInfo->AvatarActor.Get());
	if (ULB_MeleeHitboxComponent* Hitbox = MeleeMonster ? MeleeMonster->GetMeleeHitbox() : nullptr)
	{
		if (HasAuthority(&ActivationInfo))
		{
			Hitbox->OnMeleeHit.AddUniqueDynamic(this, &ULB_GA_MonsterMeleeAttack::OnMeleeHit);
			BoundHitbox = Hitbox;
		}
	}

	// 시작 Section·콤보 입력 없이 Montage를 처음부터 한 번 재생
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, MontagePlayRate);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ULB_GA_MonsterMeleeAttack::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ULB_GA_MonsterMeleeAttack::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ULB_GA_MonsterMeleeAttack::OnMontageCancelled);
	// 재생 실패(AnimInstance 없음 등) 시에도 OnCancelled가 동기 호출되어 즉시 종료·정리됨
	MontageTask->OnCancelled.AddDynamic(this, &ULB_GA_MonsterMeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void ULB_GA_MonsterMeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Montage가 NotifyEnd 전에 중단되거나 Ability가 취소(StateTree·사망)돼도 판정이 남지 않도록 정리
	// 이미 종료된 Ability에 대한 중복 EndAbility 호출(Blend Out 후 Completed 등)은 건너뜀
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		ULB_MeleeHitboxComponent* Hitbox = BoundHitbox.Get();
		if (!Hitbox)
		{
			const ALB_MeleeMonster* MeleeMonster = ActorInfo ? Cast<ALB_MeleeMonster>(ActorInfo->AvatarActor.Get()) : nullptr;
			Hitbox = MeleeMonster ? MeleeMonster->GetMeleeHitbox() : nullptr;
		}

		if (Hitbox)
		{
			Hitbox->ForceEndHitWindow();
		}
	}

	// 모든 종료 경로에서 명중 구독 해제 (중복 호출에도 안전)
	UnbindMeleeHit();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULB_GA_MonsterMeleeAttack::OnMeleeHit(AActor* HitActor, const FHitResult& HitResult)
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

	// 이미 사망한 대상에게는 추가 피해를 적용하지 않음
	if (TargetASC->HasMatchingGameplayTag(TAG_State_Death))
	{
		return;
	}

	// 팀의 단일 진실 소스는 ASC의 Team 태그. 팀을 판단할 수 없거나 같은 팀이면 피해 차단
	const FGameplayTag SourceTeam = LBTeamTagUtility::FindTeamTag(SourceASC);
	const FGameplayTag TargetTeam = LBTeamTagUtility::FindTeamTag(TargetASC);
	if (!SourceTeam.IsValid() || !TargetTeam.IsValid())
	{
		UE_LOG(LogLB, Warning, TEXT("%s: Missing Team tag (Source %s: %s, Target %s: %s). Skipping damage."),
			*GetName(), *GetNameSafe(SourceASC->GetAvatarActor()), *SourceTeam.ToString(),
			*GetNameSafe(HitActor), *TargetTeam.ToString());
		return;
	}

	if (SourceTeam == TargetTeam)
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

void ULB_GA_MonsterMeleeAttack::UnbindMeleeHit()
{
	if (ULB_MeleeHitboxComponent* Hitbox = BoundHitbox.Get())
	{
		Hitbox->OnMeleeHit.RemoveDynamic(this, &ULB_GA_MonsterMeleeAttack::OnMeleeHit);
	}
	BoundHitbox.Reset();
}

void ULB_GA_MonsterMeleeAttack::OnMontageFinished()
{
	// Blend Out 이후 Completed가 이어서 오더라도 EndAbility는 이미 비활성 상태면 무시됨
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULB_GA_MonsterMeleeAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
