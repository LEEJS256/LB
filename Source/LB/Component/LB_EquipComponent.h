// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/LB_DT_WeaponInformation.h"
#include "LB_EquipComponent.generated.h"


class UDataTable;
class ALB_Equipment;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LB_API ULB_EquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULB_EquipComponent();

	UFUNCTION(BlueprintCallable, Category = "Equip")
	TArray<ALB_Equipment*> EquipWeaponSet(FGameplayTag EquipTag, TSubclassOf<ALB_Equipment> EquipClass);
 
	/** 장착 중인 무기 세트 전부 제거 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Equip")
	void UnequipWeaponSet();
 
	UFUNCTION(BlueprintPure, Category = "Equip")
	FGameplayTag GetCurrentWeaponTag() const { return CurrentWeaponTag; }
	
	/** 전투 모드 전환 (서버 전용). true면 손, false면 등 */
	UFUNCTION(BlueprintCallable, Category = "Equip")
	void SetCombatMode(bool bCombat);

	UFUNCTION(BlueprintPure, Category = "Equip")
	bool IsCombatMode() const { return bCombatMode; }


	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 
	// 행 구조체: FLB_DT_WeaponInformation / 행 이름 = 태그 전체 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equip|Settings",
		meta = (RowType = "/Script/LB.LB_DT_WeaponInformation"))
	TObjectPtr<UDataTable> DT_WeaponInfo;

private:
	void AttachWeapon(int32 Index, bool bCombat);
	
	// 정확히 일치하는 행을 찾고, 없으면 부모 태그로 끝까지 올라가며 탐색
	const FLB_DT_WeaponInformation* FindWeaponRow(const FGameplayTag& WeaponTag) const;
 
	// 현재 장착 중인 무기 액터들 (슬롯 순서대로)
	UPROPERTY()
	TArray<TObjectPtr<ALB_Equipment>> SpawnedWeapons;
 
	UPROPERTY()
	FGameplayTag CurrentWeaponTag;

	TArray<FLB_WeaponSlot> CurrentSlots;
	bool bCombatMode = false;
};
