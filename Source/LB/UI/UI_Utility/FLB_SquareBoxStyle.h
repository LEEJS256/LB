#pragma once
 
#include "CoreMinimal.h"
#include "FLB_SquareBoxStyle.generated.h"

class UMaterialInterface;
 
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

	// ---- 머티리얼 모드 ----

	/** true면 배경/테두리를 머티리얼로 그림. false면 위의 단색들로 그림 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Material")
	bool bUseMaterial = false;

	/** 안쪽 배경 머티리얼 (User Interface 도메인). 비우면 단색 박스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Material", meta = (EditCondition = "bUseMaterial"))
	TObjectPtr<UMaterialInterface> FillMaterial = nullptr;

	/** 테두리 머티리얼 (User Interface 도메인). 비우면 단색 박스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Material", meta = (EditCondition = "bUseMaterial"))
	TObjectPtr<UMaterialInterface> BorderMaterial = nullptr;

	/**
	 * 머티리얼 모드에서도 상태색(Normal~Highlight / BorderColor)을 VertexColor로 넘길지.
	 * false: 흰색을 넘김 → 머티리얼 자체 색만 보임
	 * true : 상태색을 넘김 → 머티리얼에서 VertexColor 노드로 받아서 곱해 쓸 수 있음
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Material", meta = (EditCondition = "bUseMaterial"))
	bool bTintMaterialWithStateColors = false;
};
