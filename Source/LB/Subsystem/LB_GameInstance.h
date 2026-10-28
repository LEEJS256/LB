// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LB_GameInstance.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "LB|Data")
	TSoftObjectPtr<UDataTable> MasterItemTable;

	UPROPERTY(EditDefaultsOnly, Category = "LB|Data")
	TSoftObjectPtr<UDataTable> WeaponInfoTable;
};
