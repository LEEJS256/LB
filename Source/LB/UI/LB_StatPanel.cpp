// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_StatPanel.h"

#include "AbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "PlayerState/LB_PlayerState.h"
#include "TimerManager.h"
#include "UI/LB_StatRow.h"
#include "UI/SlateWidget/LB_NeonProgressBar.h"

namespace
{
	FLB_StatRowDef MakeRow(const FGameplayAttribute& Attribute, const FText& Label,
		int32 Decimals = 0, const FText& Suffix = FText::GetEmpty())
	{
		FLB_StatRowDef Row;
		Row.Attribute = Attribute;
		Row.Label = Label;
		Row.Decimals = Decimals;
		Row.Suffix = Suffix;
		return Row;
	}

	FText FormatNumber(float Value, int32 Decimals)
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(Decimals);
		Options.SetMaximumFractionalDigits(Decimals);
		return FText::AsNumber(Value, &Options);
	}

	/** 바 퍼센트 + "현재 / 최대" 텍스트 */
	void SetBarAndText(ULB_NeonProgressBar* Bar, UTextBlock* Text, float Current, float Max)
	{
		if (Bar)
		{
			Bar->SetPercent(Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f);
		}
		if (Text)
		{
			Text->SetText(FText::Format(NSLOCTEXT("LB", "StatCurMax", "{0} / {1}"),
				FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
		}
	}
}

// ============================================================
// 생명주기
// ============================================================

ULB_StatPanel::ULB_StatPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 목록 (WBP 디테일에서 순서/이름 수정 가능)
	StatRows.Add(MakeRow(ULB_AttributeSet::GetAttackAttribute(),       NSLOCTEXT("LB", "Stat_Attack", "공격력")));
	StatRows.Add(MakeRow(ULB_AttributeSet::GetArmorAttribute(),        NSLOCTEXT("LB", "Stat_Armor", "방어력")));
	StatRows.Add(MakeRow(ULB_AttributeSet::GetPoiseAttribute(),        NSLOCTEXT("LB", "Stat_Poise", "강인도")));
	StatRows.Add(MakeRow(ULB_AttributeSet::GetSpeedAttribute(),        NSLOCTEXT("LB", "Stat_Speed", "이동 속도")));
	StatRows.Add(MakeRow(ULB_AttributeSet::GetDashDistanceAttribute(), NSLOCTEXT("LB", "Stat_DashDist", "대시 거리")));
	StatRows.Add(MakeRow(ULB_AttributeSet::GetDashDurationAttribute(), NSLOCTEXT("LB", "Stat_DashTime", "대시 시간"),
		2, NSLOCTEXT("LB", "Unit_Sec", "초")));
	StatRows.Add(MakeRow(ULB_AttributeSet::GetGoldAttribute(),         NSLOCTEXT("LB", "Stat_Gold", "골드")));
}

void ULB_StatPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildRows();
}

void ULB_StatPanel::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindFromPlayerState();
}

void ULB_StatPanel::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}
	UnbindFromASC();
	Super::NativeDestruct();
}

// ============================================================
// 바인딩
// ============================================================

void ULB_StatPanel::TryBindFromPlayerState()
{
	if (BoundASC.IsValid())
		return;

	if (ALB_PlayerState* PS = GetOwningPlayerState<ALB_PlayerState>())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			BindToASC(ASC);
			return;
		}
	}

	// 클라에서 PS가 아직 복제 전이면 잠깐 뒤 재시도 (EquipPanel과 같은 방식)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryHandle, this, &ULB_StatPanel::TryBindFromPlayerState, 0.2f, false);
	}
}

void ULB_StatPanel::BindToASC(UAbilitySystemComponent* InASC)
{
	UnbindFromASC();
	if (!InASC)
		return;

	BoundASC = InASC;

	for (const FGameplayAttribute& Attribute : CollectWatchedAttributes())
	{
		const FDelegateHandle Handle = InASC->GetGameplayAttributeValueChangeDelegate(Attribute)
			.AddUObject(this, &ULB_StatPanel::HandleAttributeChanged);
		AttributeHandles.Emplace(Attribute, Handle);
	}

	// 델리게이트는 바뀔 때만 오니까 처음 한 번은 직접
	RefreshAll();
}

void ULB_StatPanel::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const TPair<FGameplayAttribute, FDelegateHandle>& Pair : AttributeHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
		}
	}
	AttributeHandles.Reset();
	BoundASC.Reset();
}

TArray<FGameplayAttribute> ULB_StatPanel::CollectWatchedAttributes() const
{
	TArray<FGameplayAttribute> Out = {
		ULB_AttributeSet::GetLevelAttribute(),
		ULB_AttributeSet::GetExpAttribute(),
		ULB_AttributeSet::GetMaxExpAttribute(),
		ULB_AttributeSet::GetHealthAttribute(),
		ULB_AttributeSet::GetMaxHealthAttribute(),
		ULB_AttributeSet::GetStaminaAttribute(),
		ULB_AttributeSet::GetMaxStaminaAttribute(),
	};
	for (const FLB_StatRowDef& Row : StatRows)
	{
		if (Row.Attribute.IsValid())
		{
			Out.AddUnique(Row.Attribute);
		}
	}
	return Out;
}

// ============================================================
// 목록 생성 / 갱신
// ============================================================

void ULB_StatPanel::BuildRows()
{
	if (!StatRowBox || !StatRowClass)
		return;

	StatRowBox->ClearChildren();
	RowWidgets.Reset();

	for (int32 i = 0; i < StatRows.Num(); ++i)
	{
		ULB_StatRow* Row = CreateWidget<ULB_StatRow>(this, StatRowClass);
		if (Row)
		{
			StatRowBox->AddChild(Row);
		}
		RowWidgets.Add(Row);   // 실패해도 칸은 채워서 StatRows와 인덱스를 맞춘다
	}
}

void ULB_StatPanel::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 값 10개 남짓이라 바뀐 것만 고르지 않고 전부 다시 그린다
	RefreshAll();
}

void ULB_StatPanel::RefreshAll()
{
	RefreshSummary();
	RefreshRows();
}

void ULB_StatPanel::RefreshSummary()
{
	const UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
		return;

	if (LevelText)
	{
		const int32 Level = FMath::RoundToInt(ASC->GetNumericAttribute(ULB_AttributeSet::GetLevelAttribute()));
		LevelText->SetText(FText::Format(NSLOCTEXT("LB", "StatLevel", "Lv.{0}"), FText::AsNumber(Level)));
	}

	SetBarAndText(ExpBar, ExpText,
		ASC->GetNumericAttribute(ULB_AttributeSet::GetExpAttribute()),
		ASC->GetNumericAttribute(ULB_AttributeSet::GetMaxExpAttribute()));

	SetBarAndText(HealthBar, HealthText,
		ASC->GetNumericAttribute(ULB_AttributeSet::GetHealthAttribute()),
		ASC->GetNumericAttribute(ULB_AttributeSet::GetMaxHealthAttribute()));

	SetBarAndText(StaminaBar, StaminaText,
		ASC->GetNumericAttribute(ULB_AttributeSet::GetStaminaAttribute()),
		ASC->GetNumericAttribute(ULB_AttributeSet::GetMaxStaminaAttribute()));
}

void ULB_StatPanel::RefreshRows()
{
	const UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
		return;

	const int32 Count = FMath::Min(StatRows.Num(), RowWidgets.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		ULB_StatRow* Row = RowWidgets[i];
		const FLB_StatRowDef& Def = StatRows[i];
		if (!Row || !Def.Attribute.IsValid())
			continue;

		const float Current = ASC->GetNumericAttribute(Def.Attribute);
		const float Base = ASC->GetNumericAttributeBase(Def.Attribute);

		FText Value = FormatNumber(Current, Def.Decimals);
		if (!Def.Suffix.IsEmpty())
		{
			Value = FText::Format(NSLOCTEXT("LB", "StatWithSuffix", "{0}{1}"), Value, Def.Suffix);
		}

		// Current - Base = 장비/버프 GE로 바뀐 만큼
		Row->SetStat(Def.Label, Value, Current - Base, Def.Decimals);
	}
}
