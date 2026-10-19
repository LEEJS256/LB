// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Attribute/LB_AttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectTypes.h"   // FGameplayEffectModCallbackData 포함
#include "GameplayEffectExtension.h" // 일부 확장 관련 기능
#include "Utility/LB_NativeGameplayTag.h"

ULB_AttributeSet::ULB_AttributeSet()
{
	InitHealth(180.f);
	InitMaxHealth(200.f);
	InitStamina(50.f);
	InitMaxStamina(100.f);
	InitAttack(10.f);
	InitArmor(5.f);
	InitPoise(20.f);
	InitExp(10.f);
	InitLevel(1.f);
	InitGold(100.f);
	InitSpeed(500.f);
	InitSprintWeight(1.3f);
}

void ULB_AttributeSet::Set_PlayerName(FName ArgName)
{
	PlayerName = ArgName;
}

void ULB_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, PlayerName, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Gold, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Exp, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Attack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Poise, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, Speed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULB_AttributeSet, SprintWeight, COND_None, REPNOTIFY_Always);
}

void ULB_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		//MaxHealth 넘지않게 하기
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}

	//스태미나 
	if (Attribute == GetStaminaAttribute())
	{
		//MaxStatmina 넘지않게 하기
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
}

void ULB_AttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void ULB_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

#pragma region Stat
	//Health
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float Delta = Data.EvaluatedData.Magnitude;

		if (Delta < 0.f)
		{
			//데미지 처리
			FVector HitLocation = Data.Target.GetAvatarActor()->GetActorLocation();

			// OnDamageTaken.Broadcast(-Delta, HitLocation);

			const FGameplayEffectSpec& Spec = Data.EffectSpec;

			// 🔹 크리티컬 여부
			bool bCritical = Spec.DynamicGrantedTags.HasTag(TAG_ATKType_Crit);

			// 🔹 데미지 타입
			FGameplayTag DamageType = TAG_DMGType_Normal;

			if (Spec.DynamicGrantedTags.HasTag(TAG_DMGType_Fire))
				DamageType = TAG_DMGType_Fire;
			else if (Spec.DynamicGrantedTags.HasTag(TAG_DMGType_Ice))
				DamageType = TAG_DMGType_Ice;


			OnDamageTaken.Broadcast(-Delta, HitLocation, bCritical, DamageType);
		}

		//최종체력
		float NewHealth = GetHealth();

		if (NewHealth <= 0.f)
		{
			// AActor* Owner = GetOwningActor();
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

			if (ASC)
			{
				ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Death"));
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		const float DeltaValue = Data.EvaluatedData.Magnitude;

		// 현재 체력에 변화량만큼 더하기
		SetHealth(FMath::Clamp(GetHealth() + DeltaValue, 0.0f, GetMaxHealth()));
	}

	//Stamina

	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute())
	{
		//변화량
		const float DeltaValue = Data.EvaluatedData.Magnitude;

		SetStamina(FMath::Clamp(GetStamina() + DeltaValue, 0.0f, GetMaxStamina()));
	}
#pragma endregion
}

void ULB_AttributeSet::OnRep_PlayerName(const FName& OldName)
{
	// GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Name, OldName);
}


void ULB_AttributeSet::OnRep_Speed(const FGameplayAttributeData& OldSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Speed, OldSpeed);
}

void ULB_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Health, OldHealth);
}

void ULB_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, MaxHealth, OldMaxHealth);
}

void ULB_AttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Stamina, OldStamina);
}

void ULB_AttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, MaxStamina, OldMaxStamina);
}

void ULB_AttributeSet::OnRep_Exp(const FGameplayAttributeData& OldExp)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Exp, OldExp);
}

void ULB_AttributeSet::OnRep_MaxExp(const FGameplayAttributeData& OldMaxExp)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, MaxExp, OldMaxExp);
}

void ULB_AttributeSet::OnRep_Level(const FGameplayAttributeData& OldLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Level, OldLevel);
}

void ULB_AttributeSet::OnRep_Gold(const FGameplayAttributeData& OldGold)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Gold, OldGold);
}

void ULB_AttributeSet::OnRep_Attack(const FGameplayAttributeData& OldAttack)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Attack, OldAttack);
}

void ULB_AttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Armor, OldArmor);
}

void ULB_AttributeSet::OnRep_Poise(const FGameplayAttributeData& OldPoise)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, Poise, OldPoise);
}

void ULB_AttributeSet::OnRep_SprintWeight(const FGameplayAttributeData& OldSprintWeight)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULB_AttributeSet, SprintWeight, OldSprintWeight);
}
