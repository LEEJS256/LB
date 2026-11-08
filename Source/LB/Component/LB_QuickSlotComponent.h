#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LB_QuickSlotComponent.generated.h"

class UAbilitySystemComponent;
class ULB_DA_ItemInfo;
class ULB_InventoryComponent;
UENUM(BlueprintType)
enum class ELB_QuickSlotType : uint8
{
	Skill,
	Item
};


// 드래그앤드롭 그룹 이름 (FLB_SlotDragDropOp::SourceGroup)
namespace LBSlotGroup
{
	inline FName Inventory() { return FName(TEXT("Inventory")); }
	inline FName QuickSkill() { return FName(TEXT("QuickSkill")); }
	inline FName QuickItem()  { return FName(TEXT("QuickItem")); }
	inline FName FromQuickSlotType(ELB_QuickSlotType Type)
	{
		return Type == ELB_QuickSlotType::Skill ? QuickSkill() : QuickItem();
	}
}

USTRUCT(BlueprintType)
struct LB_API FLB_QuickSlotEntry
{
	GENERATED_BODY()

	// 스킬 슬롯용 (Ability.Skill.xxx)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AbilityTag;

	// 아이템 슬롯용 (마스터테이블 로우 이름 = 아이템 코드)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SkillCode;
	
	bool IsEmpty() const { return !AbilityTag.IsValid() && ItemCode.IsNone(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLB_OnQuickSlotChanged, ELB_QuickSlotType, SlotType, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLB_OnQuickSlotUpdated, ELB_QuickSlotType, SlotType, int32, SlotIndex, FName, NewItemCode);
/**
 * 퀵슬롯 컴포넌트 (PlayerState 소유)
 * - 스킬 4칸(A/S/D/F) + 아이템 4칸(Num1~4)
 * - 슬롯 배열은 서버가 들고 복제, 등록/아이템 사용은 Server RPC
 * - 스킬 사용은 GAS(LocalPredicted GA)라서 클라에서 바로 TryActivate
 */
UCLASS(ClassGroup=(LB), meta=(BlueprintSpawnableComponent))
class LB_API ULB_QuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 SkillSlotCount = 4;
	static constexpr int32 ItemSlotCount = 4;

	ULB_QuickSlotComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---- 등록 / 해제 (클라에서 호출해도 서버에서 처리) ----
	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void RegisterSkill(int32 Index, FGameplayTag AbilityTag);

	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void RegisterItem(int32 Index, FName ItemCode);

	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void ClearSlot(ELB_QuickSlotType Type, int32 Index);

	// 같은 종류 슬롯끼리 서로 교환 (퀵슬롯 안에서 드래그)
	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void SwapSlots(ELB_QuickSlotType Type, int32 IndexA, int32 IndexB);

	// ---- 사용 (입력에서 호출) ----
	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void UseSkillSlot(int32 Index);

	UFUNCTION(BlueprintCallable, Category="LB|QuickSlot")
	void UseItemSlot(int32 Index);
	// 스킬 칸의 쿨타임 (GA의 Cooldown GE 기준). 쿨타임 중이면 true
	// UFUNCTION(BlueprintPure, Category="LB|QuickSlot")
	// bool GetSkillCooldown(int32 Index, float& OutRemaining, float& OutDuration) const;
 //
	// // 스킬 칸에 등록된 GA (아이콘/이름/설명 조회용). ASC에 아직 부여 전이면 nullptr
	// const ULB_GA_Base* GetSkillAbility(int32 Index) const;
	//
	

	UFUNCTION(BlueprintPure, Category="LB|QuickSlot")
	FLB_QuickSlotEntry GetEntry(ELB_QuickSlotType Type, int32 Index) const;

	// 인벤토리 수량이 바뀌면 호출해서 아이템 슬롯 UI 갱신 (인벤토리 OnRep/변경 델리게이트에서)
	void RefreshItemSlots();

	UPROPERTY(BlueprintAssignable, Category="LB|QuickSlot")
	FLB_OnQuickSlotChanged OnSlotChanged;

	UPROPERTY(BlueprintAssignable, Category="LB|QuickSlot")
	FLB_OnQuickSlotUpdated OnQuickSlotUpdated;
	
protected:
	virtual void BeginPlay() override;
 
	// 인벤토리 수량/구성이 바뀌면 아이템 칸 UI(수량, 흐림)를 갱신
	UFUNCTION()
	void HandleInventoryUpdated();

	
	UFUNCTION(Server, Reliable)
	void Server_SetSlot(ELB_QuickSlotType Type, int32 Index, FLB_QuickSlotEntry Entry);

	UFUNCTION(Server, Reliable)
	void Server_UseItemSlot(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_SwapSlots(ELB_QuickSlotType Type, int32 IndexA, int32 IndexB);

	UFUNCTION()
	void OnRep_SkillSlots();

	UFUNCTION()
	void OnRep_ItemSlots();

private:
	UPROPERTY(ReplicatedUsing=OnRep_SkillSlots)
	TArray<FLB_QuickSlotEntry> SkillSlots;

	UPROPERTY(ReplicatedUsing=OnRep_ItemSlots)
	TArray<FLB_QuickSlotEntry> ItemSlots;

	const TArray<FLB_QuickSlotEntry>& GetSlots(ELB_QuickSlotType Type) const
	{
		return Type == ELB_QuickSlotType::Skill ? SkillSlots : ItemSlots;
	}
 
	ULB_InventoryComponent* GetInventory() const; // 같은 PlayerState의 인벤토리
	const ULB_DA_ItemInfo* GetItemInfo(FName ItemCode) const;
};