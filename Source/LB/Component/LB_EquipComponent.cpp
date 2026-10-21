// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/LB_Equipment.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/LB_EquipComponent.h"
#include "Data/LB_DT_WeaponInformation.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
ULB_EquipComponent::ULB_EquipComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

TArray<ALB_Equipment*> ULB_EquipComponent::EquipWeaponSet(FGameplayTag EquipTag, TSubclassOf<ALB_Equipment> EquipClass)
{
	TArray<ALB_Equipment*> Result;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !EquipClass || !EquipTag.IsValid())
	{
		return Result;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
	USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	UWorld* World = GetWorld();
	if (!Mesh || !World)
	{
		return Result;
	}

	UnequipWeaponSet();

	const FLB_DT_WeaponInformation* Row = FindWeaponRow(EquipTag);

	TArray<FLB_WeaponSlot> Slots;
	if (Row && !Row->Slots.IsEmpty())
	{
		Slots = Row->Slots;
	}
	else
	{
		Slots.Add(FLB_WeaponSlot());
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FLB_WeaponSlot& Slot : Slots)
	{
		ALB_Equipment* NewWeapon = World->SpawnActor<ALB_Equipment>(EquipClass, SpawnParams);
		if (!NewWeapon) continue;

		NewWeapon->SetOwner(OwnerCharacter);
		NewWeapon->SetActorEnableCollision(false);

		if (USceneComponent* RootComp = NewWeapon->GetRootComponent())
		{
			RootComp->SetMobility(EComponentMobility::Movable);
		}

		SpawnedWeapons.Add(NewWeapon);
		CurrentSlots.Add(Slot);
		AttachWeapon(SpawnedWeapons.Num() - 1, bCombatMode);
		Result.Add(NewWeapon);
	}

	CurrentWeaponTag = EquipTag;
	return Result;
}


void ULB_EquipComponent::BeginPlay()
{
	Super::BeginPlay();

}

void ULB_EquipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnequipWeaponSet();
	Super::EndPlay(EndPlayReason);
}

void ULB_EquipComponent::AttachWeapon(int32 Index, bool bCombat)
{
	if (!SpawnedWeapons.IsValidIndex(Index) || !CurrentSlots.IsValidIndex(Index)) return;

	ALB_Equipment* Weapon = SpawnedWeapons[Index];
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!IsValid(Weapon) || !OwnerCharacter) return;

	const FLB_WeaponSlot& Slot = CurrentSlots[Index];
	const FName UsedBone = bCombat ? Slot.CombatBone : Slot.SpineBone;

	Weapon->AttachToComponent(OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		UsedBone);
	Weapon->SetActorRelativeTransform(bCombat ? Slot.CombatTransform : Slot.SpineTransform);

	
	const bool bBoneExists = OwnerCharacter->GetMesh()->GetBoneIndex(UsedBone) != INDEX_NONE;
	const FString Msg = FString::Printf(TEXT("[Equip] [%d] %s → %s (%s) 본존재=%s Loc=%s"),
		Index,
		*Slot.SlotTag.ToString(),
		*UsedBone.ToString(),
		bCombat ? TEXT("전투") : TEXT("등"),
		bBoneExists ? TEXT("O") : TEXT("X"),
		*Weapon->GetActorLocation().ToCompactString());

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, bBoneExists ? FColor::Cyan : FColor::Red, Msg);
	}
}

void ULB_EquipComponent::SetCombatMode(bool bCombat)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bCombatMode = bCombat;
	for (int32 i = 0; i < SpawnedWeapons.Num(); ++i)
	{
		AttachWeapon(i, bCombatMode);
	}
}

const FLB_DT_WeaponInformation* ULB_EquipComponent::FindWeaponRow(const FGameplayTag& WeaponTag) const
{
	if (!DT_WeaponInfo)
	{
		return nullptr;
	}
 
	// Weapon.Dual.Dagger -> Weapon.Dual -> Weapon 순서로 탐색
	FGameplayTag Current = WeaponTag;
	while (Current.IsValid())
	{
		if (const FLB_DT_WeaponInformation* Row =
			DT_WeaponInfo->FindRow<FLB_DT_WeaponInformation>(Current.GetTagName(), TEXT("EquipWeaponLookup"), false))
		{
			return Row;
		}
		Current = Current.RequestDirectParent();
	}
 
	return nullptr;
}


void ULB_EquipComponent::UnequipWeaponSet()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}
 
	for (AActor* Weapon : SpawnedWeapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
 
	SpawnedWeapons.Empty();
	CurrentWeaponTag = FGameplayTag();
}




