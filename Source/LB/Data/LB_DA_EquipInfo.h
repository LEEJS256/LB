// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Data/LB_TooltipTypes.h"
#include "LB_DA_EquipInfo.generated.h"
class ALB_Equipment;

UCLASS()
class LB_API ULB_DA_EquipInfo : public ULB_DA_ItemInfo
{
	GENERATED_BODY()

public:
	ULB_DA_EquipInfo() { ItemTypeTag = TAG_Item_Equip; }

	virtual void GetTooltipLines(TArray<FLB_TooltipLine>& Out) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equip")
	TArray<FLB_StatBonus> BonusStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equip")
	FGameplayTag WeaponTag;   // 무기 DT 행 조회용 (예: Weapon.Dual.Dagger)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equip")
	TSoftClassPtr<ALB_Equipment> EquipClass;

};
