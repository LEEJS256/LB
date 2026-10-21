// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LB_DT_WeaponInformation.generated.h"


USTRUCT(BlueprintType)
struct FLB_WeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FGameplayTag SlotTag;

	// 평상시 (등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Sheath")
	FName SpineBone = FName("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Sheath")
	FTransform SpineTransform;

	// 전투 중 (손)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Combat")
	FName CombatBone = FName("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Combat")
	FTransform CombatTransform;
};

USTRUCT(BlueprintType)
struct FLB_DT_WeaponInformation : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FGameplayTag WeaponTag;

	// TargetBoneName, SlotTransforms 대체
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<FLB_WeaponSlot> Slots;
};