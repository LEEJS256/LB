// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/LB_DungeonRoomMarkers.h"

#include "LB.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"

// ---------------------------------------------------------------------------
// ALB_DungeonRoomEntry
// ---------------------------------------------------------------------------

ALB_DungeonRoomEntry::ALB_DungeonRoomEntry()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

bool ALB_DungeonRoomEntry::HasValidEntryId() const
{
	return FLB_DungeonRouteState::IsValidIdentifier(EntryId);
}

FTransform ALB_DungeonRoomEntry::GetEntryTransform() const
{
	return FTransform(GetEntryRotation(), GetEntryLocation());
}

FVector ALB_DungeonRoomEntry::GetEntryLocation() const
{
	return Root ? Root->GetComponentLocation() : GetActorLocation();
}

FRotator ALB_DungeonRoomEntry::GetEntryRotation() const
{
	return Root ? Root->GetComponentRotation() : GetActorRotation();
}

// ---------------------------------------------------------------------------
// ALB_DungeonExit
// ---------------------------------------------------------------------------

ALB_DungeonExit::ALB_DungeonExit()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ExitTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitTrigger"));
	ExitTrigger->SetupAttachment(Root);
	ExitTrigger->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	ExitTrigger->SetCollisionProfileName(TEXT("Trigger"));
	ExitTrigger->SetGenerateOverlapEvents(true);
	ExitTrigger->SetCanEverAffectNavigation(false);
}

bool ALB_DungeonExit::IsValidExitId(FName InExitId)
{
	if (InExitId.IsNone())
	{
		return false;
	}

	const FString IdString = InExitId.ToString();
	if (IdString.IsEmpty())
	{
		return false;
	}

	for (const TCHAR Ch : IdString)
	{
		const bool bAlnum = (Ch >= TEXT('a') && Ch <= TEXT('z'))
			|| (Ch >= TEXT('A') && Ch <= TEXT('Z'))
			|| (Ch >= TEXT('0') && Ch <= TEXT('9'));
		if (!bAlnum && Ch != TEXT('_'))
		{
			return false;
		}
	}
	return true;
}

void ALB_DungeonExit::BeginPlay()
{
	Super::BeginPlay();

	if (!HasValidExitId())
	{
		UE_LOG(LogLB, Error, TEXT("[DungeonExit] %s: 잘못된 ExitId '%s' (None 불가, 영문숫자/underscore만 허용)"),
			*GetName(), *ExitId.ToString());
	}

	// Overlap 추적은 서버에서만.
	if (!HasAuthority() || !ExitTrigger)
	{
		return;
	}

	ExitTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALB_DungeonExit::HandleTriggerBeginOverlap);
	ExitTrigger->OnComponentEndOverlap.AddUniqueDynamic(this, &ALB_DungeonExit::HandleTriggerEndOverlap);
	bTrackingBound = true;

	// 바인딩 이전에 이미 겹쳐 있던 Pawn을 반영한다. BeginPlay 전에 비활성화됐다면 추적하지 않는다.
	if (bExitEnabled && RebuildPawnsFromOverlaps())
	{
		NotifyPresenceChanged();
	}
}

void ALB_DungeonExit::SetExitEnabled(bool bEnabled)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bExitEnabled == bEnabled)
	{
		return;
	}

	if (!bEnabled)
	{
		// 먼저 비활성 플래그를 세워 충돌 변경 중 발생하는 End Overlap을 무시한다.
		bExitEnabled = false;
		if (ExitTrigger)
		{
			SavedPawnResponse = ExitTrigger->GetCollisionResponseToChannel(ECC_Pawn);
			ExitTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		const bool bChanged = PawnsInExit.Num() > 0;
		ClearTrackedPawns();
		if (bChanged)
		{
			NotifyPresenceChanged();
		}
		return;
	}

	// 충돌 복구 중 Begin Overlap은 비활성 플래그로 무시하고, 복구 후 한 번에 재수집한다.
	if (ExitTrigger)
	{
		ExitTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ExitTrigger->SetCollisionResponseToChannel(ECC_Pawn, SavedPawnResponse);
		ExitTrigger->UpdateOverlaps();
	}
	bExitEnabled = true;

	if (RebuildPawnsFromOverlaps())
	{
		NotifyPresenceChanged();
	}
}

bool ALB_DungeonExit::RebuildPawnsFromOverlaps()
{
	bool bChanged = PruneInvalidPawns();
	if (!ExitTrigger)
	{
		return bChanged;
	}

	TArray<AActor*> OverlappingActors;
	ExitTrigger->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	TSet<APawn*> CurrentPawns;
	for (AActor* Actor : OverlappingActors)
	{
		if (APawn* Pawn = Cast<APawn>(Actor); IsValid(Pawn))
		{
			CurrentPawns.Add(Pawn);
		}
	}

	// 더 이상 겹치지 않는 Pawn 제거.
	TArray<APawn*> ToRemove;
	for (const TWeakObjectPtr<APawn>& WeakPawn : PawnsInExit)
	{
		APawn* Pawn = WeakPawn.Get();
		if (Pawn && !CurrentPawns.Contains(Pawn))
		{
			ToRemove.Add(Pawn);
		}
	}
	for (APawn* Pawn : ToRemove)
	{
		bChanged |= RemovePawn(Pawn);
	}

	for (APawn* Pawn : CurrentPawns)
	{
		bChanged |= AddPawn(Pawn);
	}
	return bChanged;
}

void ALB_DungeonExit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bTrackingBound && ExitTrigger)
	{
		ExitTrigger->OnComponentBeginOverlap.RemoveDynamic(this, &ALB_DungeonExit::HandleTriggerBeginOverlap);
		ExitTrigger->OnComponentEndOverlap.RemoveDynamic(this, &ALB_DungeonExit::HandleTriggerEndOverlap);
	}
	bTrackingBound = false;

	// 종료 중이므로 알림 없이 정리만 한다.
	ClearTrackedPawns();
	OnExitPresenceChanged.Clear();

	Super::EndPlay(EndPlayReason);
}

TArray<APawn*> ALB_DungeonExit::GetPawnsInExit() const
{
	TArray<APawn*> Result;
	if (!bExitEnabled)
	{
		return Result;
	}
	Result.Reserve(PawnsInExit.Num());
	for (const TWeakObjectPtr<APawn>& WeakPawn : PawnsInExit)
	{
		APawn* Pawn = WeakPawn.Get();
		if (IsValid(Pawn))
		{
			Result.Add(Pawn);
		}
	}
	return Result;
}

int32 ALB_DungeonExit::GetPawnCountInExit() const
{
	int32 Count = 0;
	if (!bExitEnabled)
	{
		return Count;
	}
	for (const TWeakObjectPtr<APawn>& WeakPawn : PawnsInExit)
	{
		if (IsValid(WeakPawn.Get()))
		{
			++Count;
		}
	}
	return Count;
}

bool ALB_DungeonExit::IsPawnInExit(const APawn* Pawn) const
{
	if (!bExitEnabled || !IsValid(Pawn))
	{
		return false;
	}
	return PawnsInExit.Contains(TWeakObjectPtr<APawn>(const_cast<APawn*>(Pawn)));
}

void ALB_DungeonExit::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bExitEnabled)
	{
		return;
	}

	bool bChanged = PruneInvalidPawns();
	bChanged |= AddPawn(Cast<APawn>(OtherActor));
	if (bChanged)
	{
		NotifyPresenceChanged();
	}
}

void ALB_DungeonExit::HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || !bExitEnabled)
	{
		return;
	}

	bool bChanged = PruneInvalidPawns();

	APawn* Pawn = Cast<APawn>(OtherActor);
	// Pawn의 다른 Component가 아직 겹쳐 있으면 유지한다.
	const bool bStillOverlapping = IsValid(Pawn) && ExitTrigger && ExitTrigger->IsOverlappingActor(Pawn);
	if (Pawn && !bStillOverlapping)
	{
		bChanged |= RemovePawn(Pawn);
	}

	if (bChanged)
	{
		NotifyPresenceChanged();
	}
}

void ALB_DungeonExit::HandlePawnDestroyed(AActor* DestroyedActor)
{
	bool bChanged = RemovePawn(Cast<APawn>(DestroyedActor));
	bChanged |= PruneInvalidPawns();
	if (bChanged)
	{
		NotifyPresenceChanged();
	}
}

bool ALB_DungeonExit::AddPawn(APawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		return false;
	}

	bool bAlreadyInSet = false;
	PawnsInExit.Add(TWeakObjectPtr<APawn>(Pawn), &bAlreadyInSet);
	if (bAlreadyInSet)
	{
		return false;
	}

	// End Overlap 누락 대비: Destroy 시 직접 제거한다.
	Pawn->OnDestroyed.AddUniqueDynamic(this, &ALB_DungeonExit::HandlePawnDestroyed);
	return true;
}

bool ALB_DungeonExit::RemovePawn(APawn* Pawn)
{
	if (!Pawn)
	{
		return false;
	}

	const int32 Removed = PawnsInExit.Remove(TWeakObjectPtr<APawn>(Pawn));
	if (Removed > 0)
	{
		Pawn->OnDestroyed.RemoveDynamic(this, &ALB_DungeonExit::HandlePawnDestroyed);
	}
	return Removed > 0;
}

bool ALB_DungeonExit::PruneInvalidPawns()
{
	bool bChanged = false;
	for (auto It = PawnsInExit.CreateIterator(); It; ++It)
	{
		APawn* Pawn = It->Get();
		if (!IsValid(Pawn))
		{
			if (Pawn)
			{
				Pawn->OnDestroyed.RemoveDynamic(this, &ALB_DungeonExit::HandlePawnDestroyed);
			}
			It.RemoveCurrent();
			bChanged = true;
		}
	}
	return bChanged;
}

void ALB_DungeonExit::NotifyPresenceChanged()
{
	OnExitPresenceChanged.Broadcast(this);
}

void ALB_DungeonExit::ClearTrackedPawns()
{
	for (const TWeakObjectPtr<APawn>& WeakPawn : PawnsInExit)
	{
		if (APawn* Pawn = WeakPawn.Get())
		{
			Pawn->OnDestroyed.RemoveDynamic(this, &ALB_DungeonExit::HandlePawnDestroyed);
		}
	}
	PawnsInExit.Reset();
}
