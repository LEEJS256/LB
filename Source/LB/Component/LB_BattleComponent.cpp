// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_BattleComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_BattleComponent::ULB_BattleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULB_BattleComponent::RequestNormalAttack()
{
	return RequestAttackByTag(TAG_ATK_Normal);
}

bool ULB_BattleComponent::RequestStrongAttack()
{
	UAbilitySystemComponent* ASC = ResolveAbilitySystemComponent();
	if (!CanRequestAttack(ASC))
	{
		return false;
	}

	// 기존 StrongATK 흐름 유지: Input.RightClick 이벤트를 ASC OwnerActor(PlayerState)에 먼저 전달
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		ASC->GetOwnerActor(),
		TAG_Input_RightClick,
		FGameplayEventData()
	);

	return RequestAttackByTag(TAG_ATK_Strong);
}

bool ULB_BattleComponent::RequestAttackByTag(const FGameplayTag& AttackTag)
{
	if (!AttackTag.IsValid())
	{
		return false;
	}

	// GAS 초기화(InitAbilityActorInfo) 전이거나 Avatar가 Owner가 아니면 요청하지 않음
	UAbilitySystemComponent* ASC = ResolveAbilitySystemComponent();
	if (!CanRequestAttack(ASC))
	{
		return false;
	}

	// 클라이언트에서 호출해도 ASC가 Ability의 Net Execution Policy에 따라 서버로 전달하며, 실행 가능 여부는 서버가 확정
	return ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag));
}

UAbilitySystemComponent* ULB_BattleComponent::ResolveAbilitySystemComponent() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

bool ULB_BattleComponent::CanRequestAttack(const UAbilitySystemComponent* ASC) const
{
	return ASC && ASC->GetAvatarActor() == GetOwner();
}
