// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_SkillDetailPanel.generated.h"

class ULB_InventoryBox;;
class UTextBlock;
class ULB_Button;
class ULB_SkillComponent;
class ULB_DA_SkillInfo;
class ULB_SkillPanel;
class ULB_SkillDetailPanel;

/**
 * 스킬 상세창 (SkillStatPanel 오른쪽에 떠 있는 창)
 * - ShowSkill(SkillCode)로 열고, X 버튼으로 닫음(Collapsed)
 * - 레벨업 버튼: CanLevelUp으로 활성화, 누르면 Server_LevelUpSkill
 * - SkillComponent가 바뀌면(레벨업/SP 변화) 열려 있는 스킬을 다시 그림
 * - 쿨타임/소모량은 DA가 아니라 GA의 Cooldown/Cost GE에서 읽는다
 */
UCLASS()
class LB_API ULB_SkillDetailPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ShowSkill(FName InSkillCode);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void HidePanel();

	UFUNCTION(BlueprintPure, Category = "Skill")
	FName GetShownSkill() const { return SkillCode; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ==========================================
	// 위젯
	// ==========================================

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_SkillPanel> SkillPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_SkillDetailPanel> DetailPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULB_InventoryBox> SkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	/** "액티브 · Lv.3 / 5" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescText;

	/** 다음 레벨 설명. MAX거나 다음 설명이 없으면 숨김 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextLevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> LevelUpButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeStateText; // "액티브 / 미습득"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelValueText; // "Lv. 0 / 5"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EffectTitleText; // "습득 시 효과"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EffectLevelText; // "Lv. 1 / 5"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageValueText; // "공격력의 120%"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PointCostText; // "필요 포인트 1"

private:
	UFUNCTION()
	void HandleSkillSelected(FName SelectedSkillCode);

	void TryBindSkillComponent();
	void Refresh();

	UFUNCTION()
	void HandleSkillUpdated();

	UFUNCTION()
	void HandleLevelUpClicked();

	UFUNCTION()
	void HandleCloseClicked();

	/** GA CDO의 Cooldown GE 지속시간 (없으면 false) */
	static bool GetCooldownSeconds(const ULB_DA_SkillInfo* Info, int32 Level, float& OutSeconds);
	/** GA CDO의 Cost GE 첫 Modifier 크기 (없으면 false) */
	static bool GetCostAmount(const ULB_DA_SkillInfo* Info, int32 Level, float& OutAmount);

	TWeakObjectPtr<ULB_SkillComponent> SkillComp;
	FTimerHandle BindRetryHandle;
	FName SkillCode;
};
