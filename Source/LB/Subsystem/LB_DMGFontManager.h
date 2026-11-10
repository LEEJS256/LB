// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LB_DMGFontManager.generated.h"
class ULB_DamageFont;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LB_API ULB_DMGFontManager : public UActorComponent
{
	GENERATED_BODY()

public:
	void Spawn(const FVector& WorldLocation, float Amount, bool bCrit = false);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULB_DamageFont> WidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float RandomOffsetX = 30.f;

private:
	UPROPERTY() TArray<TObjectPtr<ULB_DamageFont>> Pool;
	void Release(ULB_DamageFont* Widget) { Pool.Add(Widget); }
};
