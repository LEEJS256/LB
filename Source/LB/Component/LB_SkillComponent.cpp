// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_SkillComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Data/LB_DA_SkillInfo.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/LB_GISubsystem_Data.h"

// ============================================================
// 생명주기 / 복제
// ============================================================

ULB_SkillComponent::ULB_SkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULB_SkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 스킬창은 본인만 보니까 다른 클라엔 안 보낸다
	DOREPLIFETIME_CONDITION(ULB_SkillComponent, LearnedSkills, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ULB_SkillComponent, SkillPoints, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ULB_SkillComponent, AvailableSkills, COND_OwnerOnly);
}

// ============================================================
// 조회
// ============================================================

int32 ULB_SkillComponent::GetSkillLevel(FName SkillCode) const
{
	const int32 Index = FindSkillIndex(SkillCode);
	return Index != INDEX_NONE ? LearnedSkills[Index].Level : 0;
}

bool ULB_SkillComponent::CanLevelUp(FName SkillCode) const
{
	if (!AvailableSkills.Contains(SkillCode))
		return false;
	
	const ULB_DA_SkillInfo* Info = GetSkillInfo(SkillCode);
	if (!Info || !Info->AbilityClass)
		return false;

	const int32 Level = GetSkillLevel(SkillCode);
	return Level < Info->MaxLevel && SkillPoints >= Info->GetPointCost(Level + 1);
}

// ============================================================
// 스킬 조작 (서버)
// ============================================================

void ULB_SkillComponent::InitAvailableSkills(const TArray<FName>& InSkills)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return;

	AvailableSkills = InSkills;
	NotifySkillChanged(); 
}

bool ULB_SkillComponent::LevelUpSkill(FName SkillCode)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return false;

	// UI와 같은 규칙으로 서버에서 다시 검사
	if (!CanLevelUp(SkillCode))
		return false;

	const ULB_DA_SkillInfo* Info = GetSkillInfo(SkillCode);
	const int32 NewLevel = GetSkillLevel(SkillCode) + 1;

	const int32 Index = FindSkillIndex(SkillCode);
	if (Index == INDEX_NONE)
	{
		LearnedSkills.Emplace(SkillCode, NewLevel);
	}
	else
	{
		LearnedSkills[Index].Level = NewLevel;
	}

	SkillPoints -= Info->GetPointCost(NewLevel);
	GrantOrUpdateAbility(SkillCode, Info, NewLevel);

	NotifySkillChanged();
	return true;
}

void ULB_SkillComponent::AddSkillPoints(int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Count <= 0)
		return;

	SkillPoints += Count;
	NotifySkillChanged();
}

// ============================================================
// RPC
// ============================================================

void ULB_SkillComponent::Server_LevelUpSkill_Implementation(FName SkillCode)
{
	LevelUpSkill(SkillCode);
}

// ============================================================
// OnRep
// ============================================================

void ULB_SkillComponent::OnRep_AvailableSkills()
{
	OnSkillUpdated.Broadcast();
}

void ULB_SkillComponent::OnRep_LearnedSkills()
{
	OnSkillUpdated.Broadcast();
}

void ULB_SkillComponent::OnRep_SkillPoints()
{
	OnSkillUpdated.Broadcast();
}

// ============================================================
// 내부
// ============================================================

const ULB_DA_SkillInfo* ULB_SkillComponent::GetSkillInfo(FName SkillCode) const
{
	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	return Data ? Data->GetSkillInfo(SkillCode) : nullptr;
}

UAbilitySystemComponent* ULB_SkillComponent::GetASC() const
{
	// ASC는 같은 PlayerState에 있다
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

int32 ULB_SkillComponent::FindSkillIndex(FName SkillCode) const
{
	return LearnedSkills.IndexOfByPredicate(
		[&](const FLB_SkillEntry& E) { return E.SkillCode == SkillCode; });
}

void ULB_SkillComponent::GrantOrUpdateAbility(FName SkillCode, const ULB_DA_SkillInfo* Info, int32 Level)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !Info || !Info->AbilityClass)
		return;

	// 이미 부여한 스킬이면 스펙 레벨만 올린다 (사용 중인 GA는 다음 발동부터 새 레벨 적용)
	if (const FGameplayAbilitySpecHandle* Handle = SkillSpecHandles.Find(SkillCode))
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(*Handle))
		{
			Spec->Level = Level;
			ASC->MarkAbilitySpecDirty(*Spec);
			return;
		}
	}
	FGameplayAbilitySpec NewSpec(Info->AbilityClass, Level, INDEX_NONE, const_cast<ULB_DA_SkillInfo*>(Info));
	// FGameplayAbilitySpec NewSpec(Info->AbilityClass, Level, INDEX_NONE, GetOwner());
	SkillSpecHandles.Add(SkillCode, ASC->GiveAbility(NewSpec));
}

void ULB_SkillComponent::NotifySkillChanged()
{
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
	// 서버(리슨서버 호스트)는 OnRep이 안 불려서 직접 브로드캐스트
	OnSkillUpdated.Broadcast();
}