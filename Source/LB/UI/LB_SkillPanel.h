// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LB_SkillPanel.generated.h"

class UTextBlock;
class UUniformGridPanel;
class ULB_Button;
class ULB_SkillComponent;
class ULB_SkillSlotEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnSkillSelected, FName, SkillCode);

/**
 * 스킬 목록 패널 (SkillStatPanel 오른쪽)
 * - SkillComponent의 AvailableSkills를 카드로 표시 (안 배운 스킬 포함)
 * - [액티브]/[패시브] 탭: DA의 bIsPassive로 거름
 * - 카드 클릭 → OnSkillSelected(SkillCode). 상세창은 바깥(SkillStatPanel)이 띄운다
 */
UCLASS()
class LB_API ULB_SkillPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Skill")
	FLB_OnSkillSelected OnSkillSelected;

	UFUNCTION(BlueprintPure, Category = "Skill")
	FName GetSelectedSkill() const { return SelectedSkill; }

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SetShowPassive(bool bInShowPassive);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ==========================================
	// 위젯
	// ==========================================

	/** 카드가 들어갈 그리드 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> SkillGrid;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillPointText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> Tab_Active;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULB_Button> Tab_Passive;

	// ==========================================
	// 설정
	// ==========================================

	UPROPERTY(EditAnywhere, Category = "Skill")
	TSubclassOf<ULB_SkillSlotEntry> EntryClass;

	UPROPERTY(EditAnywhere, Category = "Skill", meta = (ClampMin = "1"))
	int32 Columns = 2;

	UPROPERTY(EditAnywhere, Category = "Skill|Style")
	FLinearColor TabActiveBorderColor = FLinearColor(0.f, 0.9f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Skill|Style")
	FLinearColor TabInactiveBorderColor = FLinearColor(0.11f, 0.23f, 0.29f, 1.f);

private:
	void TryBindSkillComponent();

	UFUNCTION()
	void HandleSkillUpdated();

	UFUNCTION()
	void HandleTabActiveClicked();

	UFUNCTION()
	void HandleTabPassiveClicked();

	void HandleEntryClicked(FName SkillCode);

	void Rebuild();
	void UpdateTabVisuals();

	TWeakObjectPtr<ULB_SkillComponent> SkillComp;
	FTimerHandle BindRetryHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULB_SkillSlotEntry>> EntryWidgets;

	FName SelectedSkill;
	bool bShowPassive = false;
};
