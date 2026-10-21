// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Monster/LB_MonsterBase.h"
#include "GameplayAbilitySpec.h"
#include "LB_MeleeMonster.generated.h"

class ULB_MeleeHitboxComponent;

// 근접 공격 몬스터. GAS·초기화는 ALB_MonsterBase를 그대로 사용하고 Mesh에 부착된 공용 Hitbox만 추가
UCLASS(Blueprintable)
class LB_API ALB_MeleeMonster : public ALB_MonsterBase
{
	GENERATED_BODY()

public:
	ALB_MeleeMonster();

	UFUNCTION(BlueprintPure, Category = "LB|Hitbox")
	ULB_MeleeHitboxComponent* GetMeleeHitbox() const { return MeleeHitbox; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버 전용. Target과의 월드 X 차이로 공격 방향(CombatDirectionX)을 설정하고 Hitbox에 적용. 차이가 거의 0이면 기존 방향 유지
	UFUNCTION(BlueprintCallable, Category = "LB|Combat")
	void SetCombatFacingFromTarget(AActor* TargetActor);

	// 복제되는 현재 공격 방향(+1/-1)
	UFUNCTION(BlueprintPure, Category = "LB|Combat")
	float GetCombatDirectionX() const;

	// 서버 전용. 적대·생존 Target을 확인하고 공격 방향을 정한 뒤 부여된 근접 공격 Spec 활성화를 요청
	// true는 활성화 요청 결과일 뿐 피해·완료 성공을 의미하지 않음. Target은 저장하지 않음
	UFUNCTION(BlueprintCallable, Category = "LB|Combat")
	bool RequestMeleeAttack(AActor* TargetActor);

	// 이미 부여된 ATK.Normal 몬스터 근접 공격 Spec 하나를 찾음. 없거나 여러 개면 경고 후 무효 Handle
	FGameplayAbilitySpecHandle FindMeleeAttackSpecHandle() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_CombatDirectionX();

	// 근접 판정 Box. 기본값(NoCollision, Tick·복제 비활성)은 컴포넌트 설정을 그대로 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LB|Hitbox")
	TObjectPtr<ULB_MeleeHitboxComponent> MeleeHitbox;

	// 공격 방향의 단일 원본. +1(Right) 또는 -1(Left). 서버에서만 변경하며 Hitbox에는 적용값으로만 전달
	UPROPERTY(ReplicatedUsing = OnRep_CombatDirectionX, VisibleInstanceOnly, Category = "LB|Combat")
	int8 CombatDirectionX = 1;

private:
	// CombatDirectionX를 Hitbox 공격 방향에 적용
	void ApplyCombatDirectionToHitbox();
};
