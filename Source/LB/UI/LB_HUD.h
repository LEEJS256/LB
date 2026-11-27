// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_HUD.generated.h"

class ULB_PlayerStatus;
class ULB_InventoryPanel;
class ULB_QuickSlotPanel;
class ULB_SkillStatPanel;
class ULB_ChatPanel;
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

	ULB_ChatPanel* GetChatPanel() const { return BP_ChatPanel; }
	bool IsChatInputOpen() const;
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventory();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsInventoryVisible() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsSkillStatVisible() const;
	
	void SetInventoryVisible(bool bVisible);

	void SetSkillInfoVisible(bool bVisible);
protected:

	virtual void NativeConstruct() override;

	
	UFUNCTION()
	void HandleSkillInfoCloseRequested();
	
	UFUNCTION()
	void HandleInventoryCloseRequested();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_PlayerStatus> PlayerStatus_P1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_PlayerStatus> PlayerStatus_P2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_InventoryPanel> BP_Inventory;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_SkillStatPanel> BP_SkillInfo;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_QuickSlotPanel> BP_QuickSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_ChatPanel> BP_ChatPanel;
};
