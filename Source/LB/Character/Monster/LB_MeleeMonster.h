// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Monster/LB_MonsterBase.h"
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

	// 서버 전용. Target과의 월드 X 차이로 Hitbox 공격 방향을 설정. 차이가 거의 0이면 기존 방향 유지
	UFUNCTION(BlueprintCallable, Category = "LB|Combat")
	void SetCombatFacingFromTarget(AActor* TargetActor);

	// Hitbox의 현재 공격 방향(+1/-1)
	UFUNCTION(BlueprintPure, Category = "LB|Combat")
	float GetCombatDirectionX() const;

protected:
	// 근접 판정 Box. 기본값(NoCollision, Tick·복제 비활성)은 컴포넌트 설정을 그대로 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LB|Hitbox")
	TObjectPtr<ULB_MeleeHitboxComponent> MeleeHitbox;
};
