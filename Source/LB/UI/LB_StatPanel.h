// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"            // FGameplayAttribute
#include "GameplayEffectTypes.h"     // FOnAttributeChangeData
#include "LB_StatPanel.generated.h"

class UAbilitySystemComponent;
class UPanelWidget;
class UTextBlock;
class ULB_NeonProgressBar;
class ULB_StatRow;

/** 능력치 목록 한 줄 설정. WBP 디테일에서 순서/이름을 바꿀 수 있다 */
USTRUCT(BlueprintType)
struct FLB_StatRowDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Stat")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, Category = "Stat")
	FText Label;

	/** 소수점 자리 (대시 시간 등) */
	UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0", ClampMax = "3"))
	int32 Decimals = 0;

	/** 단위 (예: "초"). 비우면 숫자만 */
	UPROPERTY(EditAnywhere, Category = "Stat")
	FText Suffix;
};

/**
 * 캐릭터 능력치 패널 (CharacterPanel 왼쪽)
 * - PlayerState의 ASC에 바인딩해서 어트리뷰트가 바뀔 때만 갱신
 * - 상단: 레벨 / EXP / 체력 / 스태미나 (바 + 텍스트)
 * - 목록: StatRows 순서대로 StatRow 생성. 보너스 = CurrentValue - BaseValue
 */
UCLASS()
class LB_API ULB_StatPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	ULB_StatPanel(const FObjectInitializer& ObjectInitializer);

	/** 보통은 NativeConstruct에서 PS의 ASC를 자동으로 찾는다. 다른 ASC를 보여줄 때만 직접 호출 */
	void BindToASC(UAbilitySystemComponent* InASC);
	void UnbindFromASC();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ==========================================
	// 상단 요약 (전부 선택)
	// ==========================================

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ExpText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_NeonProgressBar> ExpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_NeonProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StaminaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_NeonProgressBar> StaminaBar;

	// ==========================================
	// 스탯 목록
	// ==========================================

	/** StatRow들이 들어갈 컨테이너 (VerticalBox 등) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> StatRowBox;

	UPROPERTY(EditAnywhere, Category = "Stat")
	TSubclassOf<ULB_StatRow> StatRowClass;

	/** 표시할 스탯과 순서. 기본값은 생성자에서 채움 */
	UPROPERTY(EditAnywhere, Category = "Stat")
	TArray<FLB_StatRowDef> StatRows;

private:
	void TryBindFromPlayerState();
	void BuildRows();
	TArray<FGameplayAttribute> CollectWatchedAttributes() const;

	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void RefreshAll();
	void RefreshSummary();
	void RefreshRows();

	/** StatRows와 같은 순서 (생성 실패한 칸은 nullptr) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULB_StatRow>> RowWidgets;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TArray<TPair<FGameplayAttribute, FDelegateHandle>> AttributeHandles;
	FTimerHandle BindRetryHandle;
};