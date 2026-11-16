// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LB_DungeonRoomMarkers.generated.h"

class USceneComponent;
class UBoxComponent;
class UPrimitiveComponent;
class APawn;
class ALB_DungeonExit;

// Exit의 서버 Pawn 목록이 실제로 바뀌었을 때 Exit 자신을 전달한다(서버 전용, 네이티브).
DECLARE_MULTICAST_DELEGATE_OneParam(FLB_OnExitPresenceChanged, ALB_DungeonExit* /*Exit*/);

/**
 * 방 맵에 하나 배치하는 진입 지점 Marker.
 * 서버가 Pawn을 이동시킬 Transform만 제공하며 Tick·복제·자동 로직은 없다.
 */
UCLASS()
class LB_API ALB_DungeonRoomEntry : public AActor
{
	GENERATED_BODY()

public:
	ALB_DungeonRoomEntry();

	// 진입 위치·회전(Scale 제외) World Transform.
	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomEntry")
	FTransform GetEntryTransform() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomEntry")
	FVector GetEntryLocation() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomEntry")
	FRotator GetEntryRotation() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Dungeon|RoomEntry")
	TObjectPtr<USceneComponent> Root;
};

/**
 * 방 맵에 배치하는 출구 Marker.
 * 서버에서만 ExitTrigger 안의 Pawn을 추적하고 목록 변화 시 OnExitPresenceChanged를 알린다.
 * 과반수·파티·카운트다운·목적지 판단은 현재 방을 소유한 GameMode가 담당한다.
 */
UCLASS()
class LB_API ALB_DungeonExit : public AActor
{
	GENERATED_BODY()

public:
	ALB_DungeonExit();

	UFUNCTION(BlueprintPure, Category = "Dungeon|Exit")
	FName GetExitId() const { return ExitId; }

	// ExitId가 None이 아니고 영문숫자/underscore로만 이루어졌는지.
	UFUNCTION(BlueprintPure, Category = "Dungeon|Exit")
	bool HasValidExitId() const { return IsValidExitId(ExitId); }

	static bool IsValidExitId(FName InExitId);

	// 현재 Trigger 안의 유효 Pawn 목록(무효 항목 제외). 서버에서만 의미가 있다.
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Exit")
	TArray<APawn*> GetPawnsInExit() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon|Exit")
	int32 GetPawnCountInExit() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon|Exit")
	bool IsPawnInExit(const APawn* Pawn) const;

	UBoxComponent* GetExitTrigger() const { return ExitTrigger; }

	// 서버 전용. 실제 목록 변화(추가/제거/무효 정리) 때만 발행.
	FLB_OnExitPresenceChanged OnExitPresenceChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 방 안에서 고유한 출구 ID. Route Graph의 ExitId와 일치해야 한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dungeon|Exit")
	FName ExitId = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Dungeon|Exit")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Exit")
	TObjectPtr<UBoxComponent> ExitTrigger;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void HandlePawnDestroyed(AActor* DestroyedActor);

	// 추가/제거 결과 true면 목록이 바뀐 것.
	bool AddPawn(APawn* Pawn);
	bool RemovePawn(APawn* Pawn);
	// 무효(Destroy/GC) 항목 제거. 제거가 있으면 true.
	bool PruneInvalidPawns();

	void NotifyPresenceChanged();
	void ClearTrackedPawns();

	TSet<TWeakObjectPtr<APawn>> PawnsInExit;
	bool bTrackingBound = false;
};
