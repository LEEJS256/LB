// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include  "GameplayTagContainer.h"
#include "LB_DropItem.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class LB_API ALB_DropItem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALB_DropItem();

	// 서버에서 스폰 직후 호출 (SpawnActorDeferred 권장)
	void InitDrop(FName InItemCode, int32 InCount);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_ItemCode();

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                          const FHitResult& SweepResult);

	// 클라/서버 공통 외형 갱신 (DA 조회 → 등급별 나이아가라)
	void RefreshVisual();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> GradeEffect;

	UPROPERTY(ReplicatedUsing = OnRep_ItemCode)
	FName ItemCode = "1000";

	UPROPERTY(Replicated)
	int32 Count = 1;

	// 등급 태그 → 나이아가라 매핑 (BP 디폴트에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TMap<FGameplayTag, TSoftObjectPtr<UNiagaraSystem>> GradeEffectMap;

	bool bPickedUp = false;
};
