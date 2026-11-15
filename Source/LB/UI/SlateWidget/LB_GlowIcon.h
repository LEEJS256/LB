// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/UMG/Public/Components/Widget.h"
#include "LB_GlowIcon.generated.h"

/**
 * 
 */
UCLASS()
class LB_API ULB_GlowIcon : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText IconText = FText::FromString(TEXT("♥"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor IconColor = FLinearColor(1.0f, 0.17f, 0.33f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.03f, 0.05f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float GlowIntensity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D BoxSize = FVector2D(34.0f, 34.0f);

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetIconText(FText InText);

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetIconColor(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetGlowIntensity(float InIntensity);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	TSharedPtr<class SLB_GlowIcon> MyGlowIcon;
};