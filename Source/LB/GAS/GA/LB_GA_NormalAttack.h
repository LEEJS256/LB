// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/LB_GameplayAbility.h"
#include "LB_GA_NormalAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
class ULB_MeleeHitboxComponent;

/**
 * 기본 공격 Ability. Attack Montage 재생 수명 동안 공격/방향 잠금/이동 차단 태그를 부여한다.
 * 활성 중 서버에서 MeleeHitbox 명중을 구독해 DamageEffectClass Spec으로 대상 Health를 감소시킨다.
 * 판정 타이밍과 콤보는 이 클래스의 범위가 아니다.
 */
UCLASS()
class LB_API ULB_GA_NormalAttack : public ULB_GameplayAbility
{
	GENERATED_BODY()

public:
	ULB_GA_NormalAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 완료/Blend Out/Interrupt/Cancel 등 모든 종료 경로가 거치므로 여기서 근접 판정을 강제 종료
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// Ability BP의 Class Defaults에서 지정. AnimBP가 평가하는 Slot을 사용하는 Montage여야 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Attack", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.f;

	// Ability BP의 Class Defaults에서 GE_NormalAttackDamage 지정. Data.Damage SetByCaller로 Health를 변경하는 Instant GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	// 서버에서만 브로드캐스트되는 명중. 대상 ASC에 피해 Spec 적용
	UFUNCTION()
	void OnMeleeHit(AActor* HitActor, const FHitResult& HitResult);

	void UnbindMeleeHit();

	// 활성 중 OnMeleeHit을 구독한 판정 컴포넌트. EndAbility에서 해제
	TWeakObjectPtr<ULB_MeleeHitboxComponent> BoundHitbox;

	// 완료/Blend Out은 정상 종료, Interrupt/Cancel은 취소 종료로 처리
	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();
};
