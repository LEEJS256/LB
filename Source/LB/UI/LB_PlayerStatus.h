// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "LB_PlayerStatus.generated.h"

class ULB_NeonProgressBar;
class UTextBlock;
class UAbilitySystemComponent;
class ULB_AttributeSet;
class UImage;
class UMaterialInstanceDynamic;

/**
 * 
 */
UCLASS()
class LB_API ULB_PlayerStatus : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Status")
	void SetPlayerName(const FText& InName);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void SetHealth(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void SetStamina(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void SetExp(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void SetLevel(int32 InLevel);

	void BindToASC(UAbilitySystemComponent* InASC);
	void UnbindFromASC();

protected:
	virtual void NativeDestruct() override;

#pragma region  UI
	// UI 위젯 바인딩 변수 (중복 제거 및 TObjectPtr로 통일)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UserNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_NeonProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_NeonProgressBar> StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HPText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StaminaText;


	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ExpRing;

	UPROPERTY(EditAnywhere, Category = "Status|Exp")
	FName ExpPercentParam = TEXT("Percent");

	UPROPERTY(EditAnywhere, Category = "Status|HP", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthRatio = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Status|HP")
	FSlateColor NormalHPTextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Status|HP")
	FSlateColor LowHPTextColor = FLinearColor(1.f, 0.3f, 0.3f);

	void OnHealthChanged(const FOnAttributeChangeData& Data) { RefreshHealth(); }
	void OnStaminaChanged(const FOnAttributeChangeData& Data) { RefreshStamina(); }

	void OnExpChanged(const FOnAttributeChangeData& Data) { RefreshExp(); }
	void OnLevelChanged(const FOnAttributeChangeData& Data) { RefreshLevel(); }

	void RefreshExp();
	void RefreshLevel();
	void RefreshHealth();
	void RefreshStamina();

#pragma endregion

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<const ULB_AttributeSet> BoundSet;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ExpRingMID;

	FDelegateHandle HealthHandle, ExpHandle, MaxExpHandle, LevelHandle, MaxHealthHandle, StaminaHandle,
	                MaxStaminaHandle;
};
