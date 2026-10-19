// ZS_PlayerStatusWidget.cpp
#include "LB_PlayerStatus.h"

#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "UI/Slate/LB_NeonProgressBar.h"
#include "Slate/LB_InfoText.h"

void ULB_PlayerStatus::SetPlayerName(const FText& InName)
{
	if (UserNameText) UserNameText->SetText(InName);
}

void ULB_PlayerStatus::SetHealth(float Current, float Max)
{
	if (HealthBar) HealthBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
}

void ULB_PlayerStatus::SetStamina(float Current, float Max)
{
	if (StaminaBar) StaminaBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
}

void ULB_PlayerStatus::SetSpeed(float InSpeed)
{
	if (SpeedText) SpeedText->SetValueInt(FMath::RoundToInt(InSpeed));
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
	SpeedHandle = InASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetSpeedAttribute()).AddUObject(
		this, &ThisClass::OnSpeedChanged);

	// 델리게이트는 변경될 때만
	RefreshHealth();
	RefreshStamina();
	RefreshSpeed();
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
		ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetSpeedAttribute()).Remove(SpeedHandle);
	}
	BoundASC.Reset();
	BoundSet.Reset();
}

void ULB_PlayerStatus::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
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

void ULB_PlayerStatus::RefreshSpeed()
{
	if (const ULB_AttributeSet* Set = BoundSet.Get())
		SetSpeed(Set->GetSpeed());
}
