#pragma once
 
#include "CoreMinimal.h"
#include "FLB_SquareBoxStyle.generated.h"
 
/** SquareBox 계열 공용 스타일. 위젯 블루프린트 디테일 패널에서 편집 */
USTRUCT(BlueprintType)
struct LB_API FLB_SquareBoxStyle
{
	GENERATED_BODY()
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FVector2D BoxSize = FVector2D(64.0, 64.0);
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	float BorderThickness = 2.f;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FLinearColor BorderColor = FLinearColor(0.f, 0.9f, 1.f, 1.f);
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FLinearColor NormalColor = FLinearColor(0.02f, 0.03f, 0.05f, 0.85f);
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FLinearColor HoverColor = FLinearColor(0.05f, 0.15f, 0.2f, 0.9f);
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FLinearColor PressedColor = FLinearColor(0.f, 0.35f, 0.45f, 0.95f);
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FLinearColor SelectedColor = FLinearColor(0.f, 0.25f, 0.3f, 0.95f);
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB")
	FLinearColor HighlightColor = FLinearColor(0.f, 0.5f, 0.6f, 0.95f);
};