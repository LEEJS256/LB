// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GameplayAbility.h"
#include "LB_GA_MonsterMeleeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
class ULB_MeleeHitboxComponent;

/**
 * 근접 몬스터 전용 공격 Ability. 서버에서만 실행되며 Attack Montage를 한 번 재생하고 수명 동안 공격/방향 잠금/이동 차단 태그를 부여한다.
 * 공격 방향은 활성화 전에 ALB_MeleeMonster::SetCombatFacingFromTarget으로 설정된 Hitbox 값을 유지한다.
 * 플레이어 콤보·PlayerState·BattleComponent에 의존하지 않는다.
 */
UCLASS()
class LB_API ULB_GA_MonsterMeleeAttack : public ULB_GameplayAbility
{
	GENERATED_BODY()

public:
	ULB_GA_MonsterMeleeAttack();

	// 서버 권한·ALB_MeleeMonster Avatar·생존·기존 공격 미진행을 검사
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Attack", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.f;

	// Data.Damage SetByCaller로 Health를 감소시키는 피해 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	// 서버에서 Hitbox 명중 시 팀·사망·Health 필터 후 피해 GE 적용
	UFUNCTION()
	void OnMeleeHit(AActor* HitActor, const FHitResult& HitResult);

	// 이번 실행의 Hitbox 구독 해제. 중복 호출에도 안전
	void UnbindMeleeHit();

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();

	// 이번 실행에서 OnMeleeHit을 구독한 Hitbox
	TWeakObjectPtr<ULB_MeleeHitboxComponent> BoundHitbox;
};
