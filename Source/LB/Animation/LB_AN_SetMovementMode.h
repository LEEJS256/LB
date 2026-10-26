// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "LB_AN_SetMovementMode.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_AN_SetMovementMode : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	TEnumAsByte<EMovementMode> NewMovementMode = MOVE_Flying;

	// MOVE_Custom일 때만 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (EditCondition = "NewMovementMode == EMovementMode::MOVE_Custom"))
	uint8 CustomMode = 0;
};
