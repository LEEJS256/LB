// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "LB_InfoText.generated.h"


/**
 * 
 */
UCLASS()
class LB_API ULB_InfoText : public UWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText IconText = FText::FromString(TEXT("◆"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText LabelText = FText::FromString(TEXT("GOLD"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText ValueText = FText::FromString(TEXT("0"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor IconColor = FLinearColor(1.0f, 0.82f, 0.29f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "1"))
	int32 LabelFontSize = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "1"))
	int32 ValueFontSize = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "1"))
	float IconBoxSize = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BoxColor = FLinearColor(0.02f, 0.03f, 0.05f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FMargin BoxPadding = FMargin(8.0f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "1"))
	int32 IconFontSize = 14;

	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetBoxColor(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetIconFontSize(int32 InSize);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor LabelColor = FLinearColor(0.7f, 0.7f, 0.7f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor ValueColor = FLinearColor::White;

	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetLabelColor(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetValueColor(FLinearColor InColor);
	
	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetLabelFontSize(int32 InSize);

	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetValueFontSize(int32 InSize);
	
	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetValue(const FText& InValue);

	UFUNCTION(BlueprintCallable, Category = "Info")
	void SetValueInt(int32 InValue);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	TSharedPtr<class SLB_InfoText> MyInfoText;
};
