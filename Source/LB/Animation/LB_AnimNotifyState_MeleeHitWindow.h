// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Component/LB_MeleeHitboxComponent.h"
#include "LB_AnimNotifyState_MeleeHitWindow.generated.h"

/**
 * Montage 타임라인의 근접 판정 구간. Begin/End에서 Avatar의 ULB_MeleeHitboxComponent에 판정 시작/종료만 요청한다.
 * NotifyState 객체는 여러 캐릭터가 공유하므로 캐릭터별 런타임 상태를 저장하지 않는다.
 * 명중 권한, 대상 필터링, 중복 제거는 ULB_MeleeHitboxComponent가 담당한다.
 */
UCLASS(meta = (DisplayName = "LB Melee Hit Window"))
class LB_API ULB_AnimNotifyState_MeleeHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	// 이 구간의 BoxExtent, RelativeLocation, RelativeRotation, SocketName
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LB|Hitbox", meta = (ShowOnlyInnerProperties))
	FLB_MeleeHitboxSettings HitboxSettings;

private:
	static ULB_MeleeHitboxComponent* FindHitbox(const USkeletalMeshComponent* MeshComp);
};
