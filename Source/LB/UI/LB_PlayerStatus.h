// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "LB_PlayerStatus.generated.h"

class ULB_NeonProgressBar;
class ULB_InfoText;
class UTextBlock;
class UAbilitySystemComponent;
class ULB_AttributeSet;

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
	void SetSpeed(float InSpeed);

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
	TObjectPtr<ULB_InfoText> SpeedText;

	void OnHealthChanged(const FOnAttributeChangeData& Data) { RefreshHealth(); }
	void OnStaminaChanged(const FOnAttributeChangeData& Data) { RefreshStamina(); }
	void OnSpeedChanged(const FOnAttributeChangeData& Data) { RefreshSpeed(); }

	void RefreshHealth();
	void RefreshStamina();
	void RefreshSpeed();

#pragma endregion

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<const ULB_AttributeSet> BoundSet;

	FDelegateHandle HealthHandle, MaxHealthHandle, StaminaHandle, MaxStaminaHandle, SpeedHandle;
};
