// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/LB_GISubsystem_Data.h"
#include "Data/LB_DT_MasterItem.h"
#include "LB_GameInstance.h"

void ULB_GISubsystem_Data::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const ULB_GameInstance* GI = Cast<ULB_GameInstance>(GetGameInstance()))
	{
		MasterItemTable = GI->MasterItemTable.LoadSynchronous();
		WeaponInfoTable = GI->WeaponInfoTable.LoadSynchronous(); 
	}
	ensureMsgf(MasterItemTable, TEXT("BP_GameInstance에 MasterItemTable 지정 필요"));
}

const FLB_DT_MasterItem* ULB_GISubsystem_Data::FindItemRow(FName ItemCode) const
{
	if (!MasterItemTable || ItemCode.IsNone()) return nullptr;
	return MasterItemTable->FindRow<FLB_DT_MasterItem>(ItemCode, TEXT("FindItemRow"));
}

const FLB_DT_WeaponInformation* ULB_GISubsystem_Data::FindWeaponRow(const FGameplayTag& WeaponTag) const
{
	if (!WeaponInfoTable) return nullptr;

	// Weapon.Dual.Dagger -> Weapon.Dual -> Weapon 순서로 탐색
	FGameplayTag Current = WeaponTag;
	while (Current.IsValid())
	{
		if (const FLB_DT_WeaponInformation* Row =
			WeaponInfoTable->FindRow<FLB_DT_WeaponInformation>(Current.GetTagName(), TEXT("FindWeaponRow"), false))
		{
			return Row;
		}
		Current = Current.RequestDirectParent();
	}
	return nullptr;
}

ULB_GISubsystem_Data* ULB_GISubsystem_Data::Get_Subsystem_Data(const UObject* WorldContext)
{
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(
		WorldContext,
		EGetWorldErrorMode::LogAndReturnNull
	);

	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;

	return GI ? GI->GetSubsystem<ULB_GISubsystem_Data>() : nullptr;
}

ULB_DA_ItemInfo* ULB_GISubsystem_Data::GetItemInfo(FName ItemCode) const
{
	const FLB_DT_MasterItem* Row = FindItemRow(ItemCode);
	return Row ? Row->ItemInfo.LoadSynchronous() : nullptr;
}
