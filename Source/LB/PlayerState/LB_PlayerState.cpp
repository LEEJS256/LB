// LB_PlayerState.cpp
#include "LB_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "Data/LB_PlayerDataAsset.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "Utility/LB_NativeGameplayTag.h"

ALB_PlayerState::ALB_PlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 같은 액터의 서브오브젝트라 ASC에 자동 등록됨
	AttributeSet = CreateDefaultSubobject<ULB_AttributeSet>(TEXT("AttributeSet"));

	// s나중에 조정해야함
	SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* ALB_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALB_PlayerState::InitializePlayerDA()
{
	if (!HasAuthority() || bStartupApplied || !AbilitySystemComponent)
		return;

	const ULB_PlayerDataAsset* PlayerDA = CharacterData.LoadSynchronous();
	if (!PlayerDA)
		return;

	bStartupApplied = true;

	GrantDefaultGA(PlayerDA);
	ApplyDefaultAttributes(PlayerDA);
	AddStateTag(TAG_State_Idle);
}



void ALB_PlayerState::AddStateTag(FGameplayTag Tag)
{
	if (HasAuthority() && AbilitySystemComponent && Tag.IsValid())
		AbilitySystemComponent->AddLooseGameplayTag(Tag);
}

void ALB_PlayerState::RemoveStateTag(FGameplayTag Tag)
{
	if (HasAuthority() && AbilitySystemComponent && Tag.IsValid())
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
}

void ALB_PlayerState::GrantDefaultGA(const ULB_PlayerDataAsset* Data)
{
	if (!Data || !AbilitySystemComponent)
		return;

	// [변경] CharacterData-> 대신 인자로 받은 Data-> 사용
	int32 InputID = 0;
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Data->StartupGA)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, InputID, this));
			++InputID;
		}
	}

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	for (const TSubclassOf<UGameplayEffect>& GEClass : Data->StartupGE)
	{
		if (GEClass)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(GEClass->GetDefaultObject<UGameplayEffect>(), 1.f, Ctx);
		}
	}
}

void ALB_PlayerState::ApplyDefaultAttributes(const ULB_PlayerDataAsset* Data)
{
	if (!Data || !AttributeSet)
		return;

	// [변경] Max 계열을 먼저 세팅 (Health 클램프 대비)
	AttributeSet->SetMaxExp(Data->MaxExp);
	AttributeSet->SetMaxHealth(Data->MaxHealth);
	AttributeSet->SetMaxStamina(Data->MaxStamina);

	AttributeSet->SetHealth(Data->Health);
	AttributeSet->SetStamina(Data->Stamina);

	AttributeSet->SetAttack(Data->Attack);
	AttributeSet->SetArmor(Data->Armor);

	AttributeSet->SetSpeed(Data->Speed);
	AttributeSet->SetSprintWeight(Data->SprintWeight);
	AttributeSet->Set_PlayerName(Data->Player_Name);
}
