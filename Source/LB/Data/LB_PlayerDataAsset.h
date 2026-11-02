// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Abilities/GameplayAbility.h"
#include "LB_PlayerDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_PlayerDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
#pragma region AttributeSet
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Name")
	FName Player_Name = TEXT("Default_Hero");
	
	//  체력 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Speed")
	float Speed = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Speed")
	float SprintWeight = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Speed")
	float DashDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Speed")
	float DashDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Health")
	float Health = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Health")
	float MaxHealth = 100.f;

	//  스태미나 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Stamina")
	float Stamina = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Stamina")
	float MaxStamina = 50.f;
	
	// 경험치 & 레벨 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Progression")
	float Exp = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Progression")
	float MaxExp = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Progression")
	float Level = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Progression")
	float Gold = 0.f;

	//  전투 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Combat")
	float Attack = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Combat")
	float Armor = 5.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
	FGameplayTag TeamTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer InitialStateTags;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	UMaterialInstance* MaterialInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupGA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags|Effect")
	TSubclassOf<UGameplayEffect> GiveTeamtagEffect;
	
#pragma endregion

#pragma region DROPDATA

	
	
#pragma endregion 
	
};
