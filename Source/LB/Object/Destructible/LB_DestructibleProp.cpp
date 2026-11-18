// Fill out your copyright notice in the Description page of Project Settings.

#include "Object/Destructible/LB_DestructibleProp.h"

#include "LB.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Data/Loot/LB_LootTableDataAsset.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Object/Item/LB_DropItem.h"
#include "Subsystem/LB_GISubsystem_Data.h"

ALB_DestructibleProp::ALB_DestructibleProp()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	HitReceiver = CreateDefaultSubobject<UBoxComponent>(TEXT("HitReceiver"));
	HitReceiver->SetupAttachment(Root);
	HitReceiver->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitReceiver->SetCollisionObjectType(ECC_WorldDynamic);
	HitReceiver->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitReceiver->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitReceiver->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HitReceiver->SetGenerateOverlapEvents(true);
	HitReceiver->SetCanEverAffectNavigation(false);
}

void ALB_DestructibleProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALB_DestructibleProp, BreakState);
}

bool ALB_DestructibleProp::TryBreak(AActor* DamageCauser, const FVector& ImpactPoint, const FVector& ImpactDirection)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(DamageCauser) || DamageCauser == this)
	{
		return false;
	}

	if (BreakState.bBroken)
	{
		return false;
	}

	// 1. 파괴 가드 확정 (상태 하나를 원자적으로 갱신). Spawn/BP 호출 전에 설정.
	FLB_PropBreakState NewState;
	NewState.bBroken = true;
	NewState.ImpactPoint = ImpactPoint;
	NewState.ImpactDirection = ImpactDirection.GetSafeNormal();
	BreakState = NewState;

	// 2. 추가 피격 차단
	if (HitReceiver)
	{
		HitReceiver->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 3. 원본 지연 제거 수명 예약 (외부 BP 호출 전)
	SetLifeSpan(FMath::Max(BreakLifetime, 1.f));

	// 4. 서버 보상 처리 (파괴당 한 번)
	SpawnBreakLoot();

	if (!IsValid(this) || IsActorBeingDestroyed())
	{
		return true;
	}

	// 5. 복제 갱신
	ForceNetUpdate();

	// 6. (Listen Server라면) 표현. 보상 처리 이후 서버 함수 호출 순서만 보장한다.
	ApplyBreakState();

	return true;
}

void ALB_DestructibleProp::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitLootRandomIfNeeded();
	}
}

void ALB_DestructibleProp::InitLootRandomIfNeeded()
{
	if (bLootRandomInitialized || !HasAuthority())
	{
		return;
	}
	bLootRandomInitialized = true;
	LootRandom.GenerateNewSeed();
}

void ALB_DestructibleProp::SpawnBreakLoot()
{
	if (!HasAuthority() || !BreakState.bBroken || bLootProcessed)
	{
		return;
	}
	// 재진입·재호출·실패 시에도 재추첨하지 않도록 먼저 가드
	bLootProcessed = true;

	if (!LootTable)
	{
		UE_LOG(LogLB, Warning, TEXT("[%s] LootTable 미설정. 드롭 생략."), *GetName());
		return;
	}

	if (!DropItemClass)
	{
		UE_LOG(LogLB, Warning, TEXT("[%s] DropItemClass 미설정. 드롭 생략."), *GetName());
		return;
	}

	ULB_GISubsystem_Data* DataSubsystem = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	if (!DataSubsystem)
	{
		UE_LOG(LogLB, Warning, TEXT("[%s] Data Subsystem 없음. 드롭 생략."), *GetName());
		return;
	}

	InitLootRandomIfNeeded();

	TArray<FLB_LootResult> Results;
	if (!LootTable->RollLoot(LootRandom, Results) || Results.Num() == 0)
	{
		return;
	}

	const float Spacing = FMath::Max(DropSpacing, 0.f);
	const float CenterIndex = (Results.Num() - 1) * 0.5f;
	const FTransform& PropTransform = GetActorTransform();

	for (int32 Index = 0; Index < Results.Num(); ++Index)
	{
		// 원본 종료 시 남은 처리 중단
		if (!IsValid(this) || IsActorBeingDestroyed())
		{
			return;
		}

		UWorld* World = GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		const FLB_LootResult& Result = Results[Index];

		if (Result.ItemCode.IsNone() || Result.Count <= 0 || !DataSubsystem->GetItemInfo(Result.ItemCode))
		{
			UE_LOG(LogLB, Warning, TEXT("[%s] 무효 Loot 결과 건너뜀 (ItemCode=%s, Count=%d)"),
				*GetName(), *Result.ItemCode.ToString(), Result.Count);
			continue;
		}

		const FVector LocalOffset = DropOffset + FVector(0.f, (Index - CenterIndex) * Spacing, 0.f);
		const FVector SpawnLocation = PropTransform.TransformPosition(LocalOffset);
		const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

		ALB_DropItem* Drop = World->SpawnActorDeferred<ALB_DropItem>(
			DropItemClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!IsValid(Drop))
		{
			UE_LOG(LogLB, Warning, TEXT("[%s] DropItem 생성 실패 (ItemCode=%s)"),
				*GetName(), *Result.ItemCode.ToString());
			continue;
		}

		Drop->InitDrop(Result.ItemCode, Result.Count);
		Drop->FinishSpawning(SpawnTransform);
	}
}

void ALB_DestructibleProp::OnRep_BreakState()
{
	ApplyBreakState();
}

void ALB_DestructibleProp::ApplyBreakState()
{
	if (!BreakState.bBroken || bBreakApplied)
	{
		return;
	}
	bBreakApplied = true;

	if (HitReceiver)
	{
		HitReceiver->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	OnBreakPresentation(BreakState.ImpactPoint, BreakState.ImpactDirection);
}
