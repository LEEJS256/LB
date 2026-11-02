// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LB_DestructibleProp.generated.h"

class USceneComponent;
class UBoxComponent;

// 파괴 상태를 하나의 구조체로 원자적으로 복제한다.
USTRUCT(BlueprintType)
struct FLB_PropBreakState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Destructible")
	bool bBroken = false;

	UPROPERTY(BlueprintReadOnly, Category = "Destructible")
	FVector_NetQuantize ImpactPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Destructible")
	FVector_NetQuantizeNormal ImpactDirection = FVector::ZeroVector;
};

UCLASS()
class LB_API ALB_DestructibleProp : public AActor
{
	GENERATED_BODY()

public:
	ALB_DestructibleProp();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버 전용. 유효한 타 액터가 미파괴 상태의 Prop을 처음 파괴할 때만 true.
	UFUNCTION(BlueprintCallable, Category = "Destructible")
	bool TryBreak(AActor* DamageCauser, const FVector& ImpactPoint, const FVector& ImpactDirection);

	UFUNCTION(BlueprintPure, Category = "Destructible")
	bool IsBroken() const { return BreakState.bBroken; }

protected:
	UFUNCTION()
	void OnRep_BreakState();

	// 서버 직접 호출과 OnRep 공통 적용 (멱등)
	void ApplyBreakState();

	// Chaos 등 파괴 표현은 BP에서 연결. Dedicated Server에서는 호출되지 않는다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Destructible")
	void OnBreakPresentation(FVector ImpactPoint, FVector ImpactDirection);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destructible")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destructible")
	TObjectPtr<UBoxComponent> HitReceiver;

	UPROPERTY(ReplicatedUsing = OnRep_BreakState, BlueprintReadOnly, Category = "Destructible")
	FLB_PropBreakState BreakState;

	// 파괴 후 서버에서 액터를 제거하기까지의 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Destructible", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BreakLifetime = 5.f;

private:
	// 로컬 중복 표현 차단용 (복제하지 않음)
	bool bBreakApplied = false;
};
