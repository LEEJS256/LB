// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_HUD.generated.h"

class ULB_PlayerStatus;
class ULB_InventoryPanel;
UCLASS()
class LB_API ULB_HUD : public UUserWidget
{
	GENERATED_BODY()
public:
	/** P1(나), 1 = P2, 2 = P3. 없으면 nullptr */
	UFUNCTION(BlueprintPure, Category = "HUD")
	ULB_PlayerStatus* GetPlayerStatus(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	ULB_PlayerStatus* GetLocalPlayerStatus() const;
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventory();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsInventoryVisible() const;

	void SetInventoryVisible(bool bVisible);


protected:

	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleInventoryCloseRequested();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_PlayerStatus> PlayerStatus_P1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_PlayerStatus> PlayerStatus_P2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_InventoryPanel> BP_Inventory;
};
