// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LB_BattleComponent.generated.h"

class UAbilitySystemComponent;

// 플레이어 공격 요청의 공통 진입점. ASC는 소유하지 않고 Owner(IAbilitySystemInterface)가 노출하는 기존 ASC에 활성화를 요청한다
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LB_API ULB_BattleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULB_BattleComponent();

	// ATK.Normal 태그 Ability 활성화 요청
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestNormalAttack();

	// Input.RightClick 이벤트를 ASC OwnerActor에 전달한 뒤 ATK.Strong 태그 Ability 활성화 요청
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestStrongAttack();

	// 태그/ASC/Avatar 확인 후 TryActivateAbilitiesByTag 요청 결과를 반환 (서버 확정 결과가 아님)
	UFUNCTION(BlueprintCallable, Category = "LB|Battle")
	bool RequestAttackByTag(const FGameplayTag& AttackTag);

protected:
	// Owner가 IAbilitySystemInterface로 노출하는 ASC를 매번 조회 (GAS 초기화 전에는 nullptr)
	UAbilitySystemComponent* ResolveAbilitySystemComponent() const;

private:
	// ASC가 있고 Avatar가 이 컴포넌트의 Owner인지 확인
	bool CanRequestAttack(const UAbilitySystemComponent* ASC) const;
};
