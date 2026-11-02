// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_NeonProgressBar.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_NeonProgressBar : public UWidget   
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Percent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BarColor = FLinearColor(1.0f, 0.17f, 0.33f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor TrackColor = FLinearColor(0.05f, 0.06f, 0.09f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "-30.0", ClampMax = "30.0"))
	float SlantOffset = 6.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float GlowIntensity = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	bool bIsBarVisible = true;

	UFUNCTION(BlueprintCallable, Category = "Neon|Progress")
	void SetBarVisibility(bool bInIsVisible);
	
	UFUNCTION(BlueprintCallable, Category = "Neon|Progress")
	void SetPercent(float InPercent);

	UFUNCTION(BlueprintCallable, Category = "Neon|Slant")
	void SetSlentOffset(float InPercent);

	
	UFUNCTION(BlueprintCallable, Category = "Neon|Appearance")
	void SetBarColor(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Neon|Appearance")
	void SetTrackColor(FLinearColor InColor);
	
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	TSharedPtr<class SLB_NeonProgressbar> MyProgressBar;
};
