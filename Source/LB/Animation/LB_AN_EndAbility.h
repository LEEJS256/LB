// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "LB_AN_EndAbility.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_AN_EndAbility : public UAnimNotify
{
	GENERATED_BODY()
	//엔드어빌 호출용 태그는 BP에서
public:
	void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	            const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere)
	FGameplayTag Event_GA_Tag;
};
