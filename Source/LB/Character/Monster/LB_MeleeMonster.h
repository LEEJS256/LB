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

protected:
	// 근접 판정 Box. 기본값(NoCollision, Tick·복제 비활성)은 컴포넌트 설정을 그대로 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LB|Hitbox")
	TObjectPtr<ULB_MeleeHitboxComponent> MeleeHitbox;
};
