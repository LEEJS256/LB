// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_SkillDetailPanel.h"

#include "Abilities/GameplayAbility.h"
#include "Component/LB_SkillComponent.h"
#include "UI/SlateWidget/LB_InventoryBox.h"
#include "Components/TextBlock.h"
#include "Data/LB_DA_SkillInfo.h"
#include "GameplayEffect.h"
#include "PaperSprite.h"
#include "PlayerState/LB_PlayerState.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "TimerManager.h"
#include "UI/SlateWidget/LB_Button.h"

// ============================================================
// 생명주기
// ============================================================

void ULB_SkillDetailPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (LevelUpButton)
		LevelUpButton->OnClicked.AddUniqueDynamic(this, &ULB_SkillDetailPanel::HandleLevelUpClicked);
	if (CloseButton)
		CloseButton->OnClicked.AddUniqueDynamic(this, &ULB_SkillDetailPanel::HandleCloseClicked);
	if (DetailPanel)
		DetailPanel->HidePanel(); // 처음엔 닫힌 상태
}

void ULB_SkillDetailPanel::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindSkillComponent();
}

void ULB_SkillDetailPanel::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}
	if (ULB_SkillComponent* Comp = SkillComp.Get())
	{
		Comp->OnSkillUpdated.RemoveDynamic(this, &ULB_SkillDetailPanel::HandleSkillUpdated);
	}
	SkillComp.Reset();
	Super::NativeDestruct();
}

void ULB_SkillDetailPanel::HandleSkillSelected(FName SelectedSkillCode)
{
	if (DetailPanel)
		DetailPanel->ShowSkill(SelectedSkillCode);
}

void ULB_SkillDetailPanel::TryBindSkillComponent()
{
	if (SkillComp.IsValid())
		return;

	if (const ALB_PlayerState* PS = GetOwningPlayerState<ALB_PlayerState>())
	{
		if (ULB_SkillComponent* Comp = PS->Get_SkillComponent())
		{
			SkillComp = Comp;
			Comp->OnSkillUpdated.AddUniqueDynamic(this, &ULB_SkillDetailPanel::HandleSkillUpdated);
			Refresh();
			return;
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryHandle, this, &ULB_SkillDetailPanel::TryBindSkillComponent, 0.2f,
		                                  false);
	}
}

// ============================================================
// 열기 / 닫기
// ============================================================

void ULB_SkillDetailPanel::ShowSkill(FName InSkillCode)
{
	SkillCode = InSkillCode;
	SetVisibility(ESlateVisibility::Visible);
	Refresh();
}

void ULB_SkillDetailPanel::HidePanel()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULB_SkillDetailPanel::HandleCloseClicked()
{
	HidePanel();
}

void ULB_SkillDetailPanel::HandleSkillUpdated()
{
	// 레벨업 결과가 서버에서 돌아오면 Lv/버튼 상태 갱신
	Refresh();
}

void ULB_SkillDetailPanel::HandleLevelUpClicked()
{
	ULB_SkillComponent* Comp = SkillComp.Get();
	if (Comp && !SkillCode.IsNone() && Comp->CanLevelUp(SkillCode))
	{
		Comp->Server_LevelUpSkill(SkillCode);
	}
}

// ============================================================
// 표시
// ============================================================

void ULB_SkillDetailPanel::Refresh()
{
	if (SkillCode.IsNone())
		return;

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	const ULB_DA_SkillInfo* Info = Data ? Data->GetSkillInfo(SkillCode) : nullptr;
	if (!Info)
		return;

	const ULB_SkillComponent* Comp = SkillComp.Get();
	const int32 Level = Comp ? Comp->GetSkillLevel(SkillCode) : 0;
	const int32 MaxLevel = Info->MaxLevel;
	const int32 ShownLevel = FMath::Max(Level, 1); // 미습득이면 Lv.1 기준 정보를 보여준다

	// 아이콘 / 이름
	if (SkillIcon)
	{
		UPaperSprite* Icon = Info->Icon.LoadSynchronous();
		if (Icon)
		{
			SkillIcon->SetItem(Icon, 0, SkillCode);
		}
		else
		{
			SkillIcon->ClearItem();
		}
	}
	if (NameText)
	{
		NameText->SetText(Info->DisplayName);
	}

	// 종류 · 레벨
	if (LevelText)
	{
		LevelText->SetText(FText::Format(NSLOCTEXT("LB", "SkillDetailLv", "Lv. {0} / {1}"), Level, MaxLevel));
	}

	// 설명 (현재 레벨 / 다음 레벨)
	if (DescText)
	{
		DescText->SetText(Info->GetDescriptionForLevel(ShownLevel));
	}
	if (NextLevelText)
	{
		const int32 NextLevel = Level + 1;
		const bool bHasNext = Level > 0 && NextLevel <= MaxLevel && Info->LevelDescriptions.IsValidIndex(NextLevel - 1);
		if (bHasNext)
		{
			NextLevelText->SetText(FText::Format(NSLOCTEXT("LB", "SkillNextLv", "다음 레벨 (Lv.{0}): {1}"),
			                                     NextLevel, Info->GetDescriptionForLevel(NextLevel)));
			NextLevelText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			NextLevelText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const bool bMax = Level >= MaxLevel;
	const int32 PreviewLevel = FMath::Min(Level + 1, MaxLevel); // 지금 찍으면 되는 레벨

	if (TypeStateText)
	{
		const FText Type = Info->bIsPassive
			                   ? NSLOCTEXT("LB", "SkillType_Passive", "패시브")
			                   : NSLOCTEXT("LB", "SkillType_Active", "액티브");
		const FText State = Level <= 0
			                    ? NSLOCTEXT("LB", "SkillState_None", "미습득")
			                    : NSLOCTEXT("LB", "SkillState_Learned", "습득");
		TypeStateText->SetText(FText::Format(NSLOCTEXT("LB", "SkillTypeState", "{0} / {1}"), Type, State));
	}
	if (LevelValueText)
		LevelValueText->SetText(FText::Format(NSLOCTEXT("LB", "SkillLvValue", "Lv. {0} / {1}"), Level, MaxLevel));

	if (EffectTitleText)
		EffectTitleText->SetText(bMax
			                         ? NSLOCTEXT("LB", "Eff_Max", "현재 효과")
			                         : Level <= 0
			                         ? NSLOCTEXT("LB", "Eff_First", "습득 시 효과")
			                         : NSLOCTEXT("LB", "Eff_Next", "다음 레벨 효과"));
	if (EffectLevelText)
		EffectLevelText->SetText(
			FText::Format(NSLOCTEXT("LB", "SkillLvValue2", "Lv. {0} / {1}"), PreviewLevel, MaxLevel));

	FNumberFormattingOptions OneDigit;
	OneDigit.MinimumFractionalDigits = 1;
	OneDigit.MaximumFractionalDigits = 1;

	float V = 0.f;
	if (DamageValueText)
		DamageValueText->SetText(Info->GetDamagePercent(PreviewLevel, V)
			                         ? FText::Format(
				                         NSLOCTEXT("LB", "SkillDmg", "공격력의 {0}%"),
				                         FText::AsNumber(FMath::RoundToInt(V)))
			                         : FText::FromString(TEXT("-")));
	if (CooldownText)
		CooldownText->SetText(GetCooldownSeconds(Info, PreviewLevel, V)
			                      ? FText::Format(NSLOCTEXT("LB", "SkillCd", "{0}초"), FText::AsNumber(V, &OneDigit))
			                      : FText::FromString(TEXT("-")));
	if (CostText)
		CostText->SetText(GetCostAmount(Info, PreviewLevel, V)
			                  ? FText::AsNumber(FMath::RoundToInt(V))
			                  : FText::FromString(TEXT("-")));

	// 필요 포인트 + 버튼
	const int32 Cost = Info->GetPointCost(PreviewLevel);
	if (PointCostText)
	{
		PointCostText->SetText(bMax
			                       ? FText::GetEmpty()
			                       : FText::Format(NSLOCTEXT("LB", "SkillNeedSP", "필요 포인트 {0}"), Cost));
	}
	if (LevelUpButton)
	{
		LevelUpButton->SetIsEnabled(Comp && Comp->CanLevelUp(SkillCode));

		FText Label;
		if (bMax) Label = NSLOCTEXT("LB", "Btn_Max", "최대 레벨");
		else if (Comp && Comp->GetSkillPoints() < Cost) Label = NSLOCTEXT("LB", "Btn_NoSP", "포인트 부족");
		else if (Level <= 0) Label = NSLOCTEXT("LB", "Btn_Learn", "+ 스킬 습득");
		else Label = NSLOCTEXT("LB", "Btn_LvUp", "+ 레벨 업");
		LevelUpButton->SetLabelText(Label);
	}
}

bool ULB_SkillDetailPanel::GetCooldownSeconds(const ULB_DA_SkillInfo* Info, int32 Level, float& OutSeconds)
{
	const UGameplayAbility* GA = (Info && Info->AbilityClass)
		                             ? Info->AbilityClass->GetDefaultObject<UGameplayAbility>()
		                             : nullptr;
	const UGameplayEffect* CooldownGE = GA ? GA->GetCooldownGameplayEffect() : nullptr;
	if (!CooldownGE)
		return false;

	return CooldownGE->DurationMagnitude.GetStaticMagnitudeIfPossible(static_cast<float>(Level), OutSeconds);
}

bool ULB_SkillDetailPanel::GetCostAmount(const ULB_DA_SkillInfo* Info, int32 Level, float& OutAmount)
{
	const UGameplayAbility* GA = (Info && Info->AbilityClass)
		                             ? Info->AbilityClass->GetDefaultObject<UGameplayAbility>()
		                             : nullptr;
	const UGameplayEffect* CostGE = GA ? GA->GetCostGameplayEffect() : nullptr;
	if (!CostGE || CostGE->Modifiers.Num() == 0)
		return false;

	float Magnitude = 0.f;
	if (!CostGE->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(static_cast<float>(Level), Magnitude))
		return false;

	OutAmount = FMath::Abs(Magnitude); // 비용 GE는 보통 -20 같은 음수
	return true;
}
