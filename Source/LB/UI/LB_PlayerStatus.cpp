// ZS_PlayerStatusWidget.cpp
#include "LB_PlayerStatus.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/SlateWidget/LB_NeonProgressBar.h"

void ULB_PlayerStatus::SetPlayerName(const FText& InName)
{
	if (UserNameText) UserNameText->SetText(InName);
}

void ULB_PlayerStatus::SetHealth(float Current, float Max)
{
	const float Ratio = Max > 0.f ? Current / Max : 0.f;

	if (HealthBar)
		HealthBar->SetPercent(Ratio);

	if (!HPText)
		return;

	HPText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Current), FMath::RoundToInt(Max))));
	HPText->SetColorAndOpacity(Ratio <= LowHealthRatio ? LowHPTextColor : NormalHPTextColor);
}

void ULB_PlayerStatus::SetStamina(float Current, float Max)
{
	if (StaminaBar)
		StaminaBar->SetPercent(Max > 0.f ? Current / Max : 0.f);

	if (!StaminaText)
		return;

	StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::FloorToInt(Current), FMath::RoundToInt(Max))));
}


void ULB_PlayerStatus::SetExp(float Current, float Max)
{
	if (!ExpRing)
		return;

	if (!ExpRingMID)
		ExpRingMID = ExpRing->GetDynamicMaterial();

	if (!ExpRingMID)
		return;

	const float Ratio = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;
	ExpRingMID->SetScalarParameterValue(ExpPercentParam, Ratio);
}

void ULB_PlayerStatus::SetLevel(int32 InLevel)
{
	if (!LevelText)
		return;

	LevelText->SetText(FText::AsNumber(InLevel));
}

void ULB_PlayerStatus::BindToASC(UAbilitySystemComponent* InASC)
{
	UnbindFromASC();
	if (!InASC) return;

	const ULB_AttributeSet* Set = InASC->GetSet<ULB_AttributeSet>();
	if (!Set) return;

	BoundASC = InASC;
	BoundSet = Set;

	HealthHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetHealthAttribute()).AddUObject(
		this, &ThisClass::OnHealthChanged);
	MaxHealthHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetMaxHealthAttribute()).
	                         AddUObject(this, &ThisClass::OnHealthChanged);
	StaminaHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetStaminaAttribute()).AddUObject(
		this, &ThisClass::OnStaminaChanged);
	MaxStaminaHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetMaxStaminaAttribute()).
	                          AddUObject(this, &ThisClass::OnStaminaChanged);
	ExpHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetExpAttribute()).AddUObject(
		this, &ThisClass::OnExpChanged);
	MaxExpHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetMaxExpAttribute()).AddUObject(
		this, &ThisClass::OnExpChanged);
	LevelHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetLevelAttribute()).AddUObject(
		this, &ThisClass::OnLevelChanged);


	// 델리게이트는 변경될 때만
	RefreshExp();
	RefreshLevel();
	RefreshHealth();
	RefreshStamina();
}

void ULB_PlayerStatus::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetStaminaAttribute()).Remove(StaminaHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetMaxStaminaAttribute()).Remove(
			MaxStaminaHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetExpAttribute()).Remove(ExpHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetMaxExpAttribute()).Remove(MaxExpHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetLevelAttribute()).Remove(LevelHandle);
	}
	BoundASC.Reset();
	BoundSet.Reset();
}

void ULB_PlayerStatus::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

void ULB_PlayerStatus::RefreshExp()
{
	if (const ULB_AttributeSet* Set = BoundSet.Get())
		SetExp(Set->GetExp(), Set->GetMaxExp());
}

void ULB_PlayerStatus::RefreshLevel()
{
	if (const ULB_AttributeSet* Set = BoundSet.Get())
		SetLevel(FMath::RoundToInt(Set->GetLevel()));
}

void ULB_PlayerStatus::RefreshHealth()
{
	if (const ULB_AttributeSet* Set = BoundSet.Get())
		SetHealth(Set->GetHealth(), Set->GetMaxHealth());
}

void ULB_PlayerStatus::RefreshStamina()
{
	if (const ULB_AttributeSet* Set = BoundSet.Get())
		SetStamina(Set->GetStamina(), Set->GetMaxStamina());
}
