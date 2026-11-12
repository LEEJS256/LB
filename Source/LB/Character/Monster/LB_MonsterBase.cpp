// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Monster/LB_MonsterBase.h"

#include "LB.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "Components/WidgetComponent.h"
#include "Controller/LB_PlayerController.h"
#include "Data/LB_MonsterDataAsset.h"
#include "GAS/Attribute/LB_AttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/LB_DMGFontManager.h"
#include "UI/LB_MonsterHPBar.h"
#include "Utility/LB_NativeGameplayTag.h"

// Sets default values
ALB_MonsterBase::ALB_MonsterBase()
{
	Set_HPComp();

	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// 같은 액터의 서브오브젝트라 ASC에 자동 등록됨
	AttributeSet = CreateDefaultSubobject<ULB_AttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ALB_MonsterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool ALB_MonsterBase::IsDead() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Death);
}

// Called when the game starts or when spawned
void ALB_MonsterBase::BeginPlay()
{
	Super::BeginPlay();

	Begin_HPComp();

	// 서버/클라이언트 모두 Owner와 Avatar를 몬스터 자신으로 초기화
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	InitializeMonsterData();
}

void ALB_MonsterBase::Set_HPComp()
{
	HPBarComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComp"));
	HPBarComp->SetupAttachment(GetRootComponent());
	HPBarComp->SetWidgetSpace(EWidgetSpace::Screen); // 횡스크롤이라 항상 정면으로
	HPBarComp->SetDrawSize(FVector2D(120.f, 14.f));
	HPBarComp->SetRelativeLocation(FVector(0.f, 0.f, 110.f)); // 캡슐 높이에 맞게 조절
	HPBarComp->SetPivot(FVector2D(0.5f, 1.f));
	HPBarComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HPBarComp->SetVisibility(false); // 피격 전엔 숨김
}

void ALB_MonsterBase::Begin_HPComp()
{
	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	} // 서버는 UI 필요 없음

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) { return; }

	// TODO: 몬스터 AttributeSet의 Health/MaxHealth 이름
	ASC->GetGameplayAttributeValueChangeDelegate(ULB_AttributeSet::GetHealthAttribute())
	   .AddUObject(this, &ALB_MonsterBase::OnHealthChanged);

	// 초기값 세팅
	if (ULB_MonsterHPBar* Bar = Cast<ULB_MonsterHPBar>(HPBarComp->GetUserWidgetObject()))
	{
		Bar->SetHealth(ASC->GetNumericAttribute(ULB_AttributeSet::GetHealthAttribute()),
		               ASC->GetNumericAttribute(ULB_AttributeSet::GetMaxHealthAttribute()));
	}
}

void ALB_MonsterBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const float Max = ASC->GetNumericAttribute(ULB_AttributeSet::GetMaxHealthAttribute());

	if (ULB_MonsterHPBar* Bar = Cast<ULB_MonsterHPBar>(HPBarComp->GetUserWidgetObject()))
	{
		Bar->SetHealth(Data.NewValue, Max);
	}

#pragma region DMGFONT
	const float Delta = Data.OldValue - Data.NewValue;
	if (Delta > 0.f) // 깎인 경우만 (회복/초기 복제값은 무시)
	{
		if (ALB_PlayerController* LB_PC = Cast<ALB_PlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			if (auto* Mgr = LB_PC->FindComponentByClass<ULB_DMGFontManager>())
			{
				Mgr->Spawn(GetActorLocation() + FVector(0.f, 0.f, 100.f), Delta);
			}
		}
	}
#pragma endregion

	if (Data.NewValue <= 0.f)
	{
		HPBarComp->SetVisibility(false);
		return;
	}

	// 평소 피격 시에는 체력바를 켜두고 유지 (타이머로 자동 숨김 안 함)
	HPBarComp->SetVisibility(true);
}


void ALB_MonsterBase::HideHPBar()
{
	HPBarComp->SetVisibility(false);
}

void ALB_MonsterBase::ApplyDefaultAttributes(const ULB_MonsterDataAsset* Data)
{
	if (!Data || !AttributeSet)
		return;


	AttributeSet->SetMaxHealth(Data->MaxHealth);


	AttributeSet->SetHealth(Data->Health);


	AttributeSet->SetAttack(Data->Attack);
	AttributeSet->SetArmor(Data->Armor);

	AttributeSet->SetSpeed(Data->Speed);


	AttributeSet->Set_PlayerName(Data->Monster_Name);
}

void ALB_MonsterBase::InitializeMonsterData()
{
	if (!HasAuthority() || bStartupApplied || !AbilitySystemComponent)
		return;

	if (!MonsterData)
	{
		UE_LOG(LogLB, Warning, TEXT("%s: MonsterData가 설정되지 않아 초기 GAS 데이터를 적용하지 않습니다."), *GetName());
		return;
	}

	ApplyDefaultAttributes(MonsterData);

	bStartupApplied = true;

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : MonsterData->StartupGA)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	Ctx.AddSourceObject(this);
	for (const TSubclassOf<UGameplayEffect>& GEClass : MonsterData->StartupGE)
	{
		if (GEClass)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(GEClass->GetDefaultObject<UGameplayEffect>(), 1.f, Ctx);
		}
	}
}
