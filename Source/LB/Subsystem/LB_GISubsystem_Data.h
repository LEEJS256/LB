// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Data/LB_DT_WeaponInformation.h"
#include "LB_GISubsystem_Data.generated.h"


class UDataTable;
class ULB_DA_ItemInfo;
struct FLB_DT_MasterItem;

UCLASS()
class LB_API ULB_GISubsystem_Data : public UGameInstanceSubsystem
{

	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	const FLB_DT_MasterItem*		FindItemRow(FName ItemCode) const;
	const FLB_DT_WeaponInformation* FindWeaponRow(const FGameplayTag& WeaponTag) const;

	static ULB_GISubsystem_Data* Get_Subsystem_Data(const UObject* WorldContext);
	
	UFUNCTION(BlueprintPure, Category = "LB|Data")
	ULB_DA_ItemInfo* GetItemInfo(FName ItemCode) const;

private:
	UPROPERTY()
	TObjectPtr<UDataTable> MasterItemTable;

	UPROPERTY()
	TObjectPtr<UDataTable> WeaponInfoTable;
	
};
