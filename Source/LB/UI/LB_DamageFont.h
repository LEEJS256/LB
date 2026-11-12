// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_DamageFont.generated.h"

/**
 * 
 */
class UTextBlock;
class UWidgetAnimation;
DECLARE_DELEGATE_OneParam(FOnDamageNumberFinished, class ULB_DamageFont*);

UCLASS()
class LB_API ULB_DamageFont : public UUserWidget
{
	GENERATED_BODY()


public:
	void Play(float Amount, bool bCrit);
	FOnDamageNumberFinished OnFinished;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

	// WBP 애니메이션 이름을 PopAnim으로 맞추기
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PopAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor NormalColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor CritColor = FLinearColor(1.f, 0.82f, 0.25f);

	virtual void NativeOnInitialized() override;
	void HandleAnimFinished();
};


