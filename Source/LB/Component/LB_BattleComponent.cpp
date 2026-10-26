// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_BattleComponent.h"
#include "Component/LB_EquipComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "IPropertyTable.h"
#include "Utility/LB_NativeGameplayTag.h"

ULB_BattleComponent::ULB_BattleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULB_BattleComponent::RequestNormalAttack()
{
	return RequestAttackByTag(TAG_ATK_Normal);
}

bool ULB_BattleComponent::RequestStrongAttack()
{
	UAbilitySystemComponent* ASC = ResolveAbilitySystemComponent();
	if (!CanRequestAttack(ASC))
	{
		return false;
	}

	// 기존 StrongATK 흐름 유지: Input.RightClick 이벤트를 ASC OwnerActor(PlayerState)에 먼저 전달
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		ASC->GetOwnerActor(),
		TAG_Input_Strong,
		FGameplayEventData()
	);

	return RequestAttackByTag(TAG_ATK_Strong);
}

bool ULB_BattleComponent::RequestAttackByTag(const FGameplayTag& AttackTag)
{
	if (!AttackTag.IsValid())
	{
		return false;
	}

	// GAS 초기화(InitAbilityActorInfo) 전이거나 Avatar가 Owner가 아니면 요청하지 않음
	UAbilitySystemComponent* ASC = ResolveAbilitySystemComponent();
	if (!CanRequestAttack(ASC))
	{
		return false;
	}

	// 클라이언트에서 호출해도 ASC가 Ability의 Net Execution Policy에 따라 서버로 전달하며, 실행 가능 여부는 서버가 확정
	return ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag));
}

UAbilitySystemComponent* ULB_BattleComponent::ResolveAbilitySystemComponent() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

bool ULB_BattleComponent::CanRequestAttack(const UAbilitySystemComponent* ASC) const
{
	return ASC && ASC->GetAvatarActor() == GetOwner();
}
void ULB_BattleComponent::BindCombatStateEvents()
{
	if (bATKBound) return;

	UAbilitySystemComponent* ASC = ResolveAbilitySystemComponent();
	if (!ASC) return;

	ASC->RegisterGameplayTagEvent(TAG_State_ATK, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULB_BattleComponent::OnATKTagChanged);
	bATKBound = true;
}

void ULB_BattleComponent::OnATKTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return;

	ULB_EquipComponent* EquipComp = OwnerActor->FindComponentByClass<ULB_EquipComponent>();
	if (!EquipComp) return;

	if (NewCount > 0)
	{
		EquipComp->EnterCombat();
	}
	else
	{
		EquipComp->ReserveExitCombat();
	}
}


bool ULB_BattleComponent::RequestComboInput(const FGameplayTag& InputTag)
{
	UAbilitySystemComponent* ASC = ResolveAbilitySystemComponent();
	if (!CanRequestAttack(ASC) || !InputTag.IsValid()) return false;

	// 콤보 진행 중: 윈도우 열려있을 때만 버퍼에 저장
	if (ASC->HasMatchingGameplayTag(TAG_State_ATK))
	{
		if (bComboWindowOpen)
		{
			BufferedInput = InputTag;
			return true;
		}
		return false;
	}

	// 콤보 시작: 버퍼에 넣고 GA 활성화 (GA가 ConsumeStartSection으로 꺼내감)
	BufferedInput = InputTag;
	return ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Combo));
}

bool ULB_BattleComponent::ConsumeStartSection(FName& OutSection)
{
	const FGameplayTag Input = BufferedInput;
	BufferedInput = FGameplayTag();
	return TryAdvance(Input, OutSection);
}

void ULB_BattleComponent::OpenComboWindow()
{
	bComboWindowOpen = true;
	BufferedInput = FGameplayTag(); // 윈도우 이전 입력은 버림
}

bool ULB_BattleComponent::CloseComboWindow(FName& OutNextSection)
{
	bComboWindowOpen = false;
	const FGameplayTag Input = BufferedInput;
	BufferedInput = FGameplayTag();
	return Input.IsValid() && TryAdvance(Input, OutNextSection);
}

void ULB_BattleComponent::ResetCombo()
{
	CurrentComboState = TAG_Combo_None;
	BufferedInput = FGameplayTag();
	bComboWindowOpen = false;
}

bool ULB_BattleComponent::TryAdvance(const FGameplayTag& InputTag, FName& OutSection)
{
	if (!InputTag.IsValid()) return false;

	const FLB_DT_ComboInfo* Row = ComboMap.Find(TPair<FGameplayTag, FGameplayTag>(CurrentComboState, InputTag));
	if (!Row) return false;

	CurrentComboState = Row->ToState;
	OutSection = Row->MontageSection;
	return true;
}

void ULB_BattleComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentComboState = TAG_Combo_None;
	if (!ComboTable) return;

	ComboTable->ForeachRow<FLB_DT_ComboInfo>(TEXT("ComboCache"),
		[this](const FName& Key, const FLB_DT_ComboInfo& Row)
		{
			const auto MapKey = TPair<FGameplayTag, FGameplayTag>(Row.FromState, Row.InputTag);
			// 중복 키 검증
			if (ComboMap.Contains(MapKey))
			{
				UE_LOG(LogTemp, Error, TEXT("Combo 중복 행: %s"), *Key.ToString());
				return;
			}
			ComboMap.Add(MapKey, Row);
		});
}
