// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LB_InventorySlotEntry.generated.h"

class ULB_InventoryBox;
class ULB_InventoryItemObject;
class ULB_InventoryComponent;


UCLASS(Abstract)
class LB_API ULB_InventorySlotEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	// IUserObjectListEntry
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_InventoryBox> InventoryBox;

	virtual void NativePreConstruct() override;

	/** 슬롯 테두리색 (hex 입력 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot|Color")
	FColor BorderColor = FColor(28, 58, 74);   // #1C3A4A

	/** 슬롯 안쪽 배경색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot|Color")
	FColor FillColor = FColor(10, 18, 28);
	
private:
	void ApplySlotColors();
	
	UFUNCTION()
	void HandleUse(int32 InSlotIndex);

	UFUNCTION()
	void HandleSlotDropped(FName SrcGroup, int32 SrcIndex, int32 DstIndex);

	ULB_InventoryComponent* GetInventoryComponent() const;
	void ClearView();

	UPROPERTY(Transient)
	TObjectPtr<ULB_InventoryItemObject> Item;
};