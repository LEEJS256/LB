// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/LB_ItemEntry.h"
#include "LB_InventoryComponent.generated.h"




// DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LB_API ULB_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULB_InventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==========================================
	// 인벤토리 조작 기능
	// ==========================================

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemCode, int32 Count = 1);
	
	/** 인스턴스 ID 기준 아이템 수량 차감/제거 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FGuid ItemInstanceId, int32 Count = 1);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_UseItem(FGuid ItemInstanceId);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_DropItem(FGuid ItemInstanceId, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemCode) const;

	// 첫 번째 엔트리의 GUID. 퀵슬롯에서 ItemCode로 사용할 때 필요
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindFirstEntryByCode(FName ItemCode, FGuid& OutId) const;

	
	/** 인벤토리 전체 항목 조회 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FLB_ItemEntry>& GetInventoryItems() const { return InventoryItems; }

	// ==========================================
	// 장착 / 해제 연동
	// ==========================================

	/** 아이템 장착 시도 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equip")
	bool EquipItem(FGuid ItemInstanceId);

	/** 아이템 해제 시도 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equip")
	bool UnequipItem(FGuid ItemInstanceId);

	// UPROPERTY(BlueprintAssignable, Category = "Inventory")
	// FOnInventoryUpdated OnInventoryUpdated;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FLB_ItemEntry> InventoryItems;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 MaxSlots = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class ALB_DropItem> DropItemClass;
	
	UFUNCTION()
	void OnRep_InventoryItems();

private:
	bool UseItem(FGuid ItemInstanceId);   // 서버 내부용
	const class ULB_DA_ItemInfo* GetItemInfo(FName ItemCode) const;
	class ULB_EquipComponent* GetEquipComponent() const;
		
};
