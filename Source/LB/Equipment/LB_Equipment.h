// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "LB_Equipment.generated.h"

 
class UGameplayEffect;
class UAbilitySystemComponent;
class ULB_DA_Equipment;
/**
 * 장비 공통 베이스 액터.
 * - 스폰되어 Owner가 지정되면(서버) Granted_GE를 Owner의 ASC에 적용하고,
 *   액터가 사라질 때(EndPlay) 적용했던 GE를 제거한다.
 * - GE 적용/제거가 액터 수명과 묶여 있어서 리스폰이나 교체 때 중복/누수가 생기지 않는다.
 */
UCLASS()
class LB_API ALB_Equipment : public AActor
{
	GENERATED_BODY()
	
public:	
	ALB_Equipment();

	UFUNCTION(BlueprintPure, Category = "Equipment|Mesh")
	USkeletalMeshComponent* GetSkeletalMesh() const { return WeaponSkeletalMesh; }

	/** 스태틱 메시 Getter */
	UFUNCTION(BlueprintPure, Category = "Equipment|Mesh")
	UStaticMeshComponent* GetStaticMesh() const { return WeaponStaticMesh; }

	// void InitializeEquipment(ULB_DA_Equipment* InItemData);
	ULB_DA_Equipment* GetItemData() const { return ItemData; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USceneComponent> Root;
 
	// // 장착 중에만 유지되는 GE 목록. 반드시 Duration Policy = Infinite 로 만들 것
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|GAS")
	// TArray<TSubclassOf<UGameplayEffect>> Granted_GE;

	// 1. 활, 변형 무기 등 애니메이션이 포함된 무기용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Mesh")
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMesh;

	// 2. 검, 도끼 등 일반 정적 무기용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Mesh")
	TObjectPtr<UStaticMeshComponent> WeaponStaticMesh;
	
	void ApplyGrantedEffects();
	void RemoveGrantedEffects();
 
	// Owner(캐릭터)에서 ASC를 찾는다. 캐릭터가 인터페이스를 안 가졌으면 PlayerState에서 찾는다
	UAbilitySystemComponent* GetOwnerASC() const;
 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<ULB_DA_Equipment> ItemData;
private:
	// 서버 전용 기록 (리플리케이트 불필요)
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	
};