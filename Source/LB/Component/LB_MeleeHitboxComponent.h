// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "LB_MeleeHitboxComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLB_OnMeleeHit, AActor*, HitActor, const FHitResult&, HitResult);

// 판정 구간 하나의 Box 설정.
// SocketName=None: Character 위치 기준, X=공격 방향(CurrentFacing이 Left면 반전), Y=깊이, Z=높이
// SocketName 지정: Mesh Socket 기준 상대값
USTRUCT(BlueprintType)
struct FLB_MeleeHitboxSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Hitbox")
	FVector BoxExtent = FVector(50.f, 50.f, 50.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Hitbox")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Hitbox")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	// None이면 Character 축 기준. 지정한 Socket이 Mesh에 없으면 Character 축 기준으로 대체
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LB|Hitbox")
	FName SocketName = NAME_None;
};

/**
 * 근접 공격 판정 Box. 기본은 비활성(NoCollision)이며 판정 구간 동안만 Query Overlap을 켠다.
 * Socket 지정 시 Mesh Socket을, 미지정 시 Character 축과 CurrentFacing을 기준으로 배치하며,
 * 명중은 서버에서만 확정해 OnMeleeHit으로 전달한다.
 * Tick을 사용하지 않는다.
 */
UCLASS(ClassGroup = (LB), meta = (BlueprintSpawnableComponent))
class LB_API ULB_MeleeHitboxComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	ULB_MeleeHitboxComponent();

	// 판정 구간 시작. 명중 목록을 초기화하고 이미 겹쳐 있는 대상도 확인
	UFUNCTION(BlueprintCallable, Category = "LB|Hitbox")
	void BeginHitWindow(const FLB_MeleeHitboxSettings& Settings);

	// 정상 판정 구간 종료 (NotifyEnd 등)
	UFUNCTION(BlueprintCallable, Category = "LB|Hitbox")
	void EndHitWindow();

	// Montage 중단, Ability 취소 등에서 호출하는 강제 종료. EndHitWindow와 같은 정리 경로 사용
	UFUNCTION(BlueprintCallable, Category = "LB|Hitbox")
	void ForceEndHitWindow();

	UFUNCTION(BlueprintPure, Category = "LB|Hitbox")
	bool IsHitWindowActive() const { return bHitWindowActive; }

	// 서버에서만 브로드캐스트. 한 판정 구간에서 대상당 1회
	UPROPERTY(BlueprintAssignable, Category = "LB|Hitbox")
	FLB_OnMeleeHit OnMeleeHit;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TryRegisterHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult* SweepResult);

	void DeactivateHitWindow();

	// SocketName=None 설정을 Character 위치 + 월드 X/Y/Z 축 기준으로 적용. Left 방향이면 X축 거울 반전
	void ApplyCharacterSpaceSettings(const FLB_MeleeHitboxSettings& Settings);

	// BeginPlay 시점의 부착 대상(Mesh). Socket 변경 시 이 컴포넌트 기준으로 다시 부착
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DefaultAttachParent;

	// 현재 판정 구간에서 이미 명중 처리한 Actor
	TSet<TWeakObjectPtr<AActor>> HitActors;

	bool bHitWindowActive = false;
};
