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
	ULB_DMGFontManager();
	void Spawn(const FVector& WorldLocation, float Amount, bool bCrit = false);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction) override;
protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULB_DamageFont> WidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float RandomOffsetX = 30.f;

private:

	struct FActiveEntry
	{
		TObjectPtr<ULB_DamageFont> Widget;
		FVector WorldLocation;      // 맞은 순간의 월드 좌표 (고정)
		FVector2D ScreenOffset;     // 겹침 방지용 랜덤 오프셋
	};

	UPROPERTY() TArray<TObjectPtr<ULB_DamageFont>> Pool;
	TArray<FActiveEntry> Active;
	
	void Release(ULB_DamageFont* Widget);
};
