// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "LB_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ULB_HUD;
class ULB_DMGFontManager;
class ALB_GameState;
class AGameStateBase;
/**
 *
 */
UCLASS()
class LB_API ALB_PlayerController : public APlayerController
{
	GENERATED_BODY()
	ALB_PlayerController();

public:
	void CloseInventory();
	void CloseSkillStat();

	UFUNCTION(BlueprintCallable, Category = "LB|Component")
	class ULB_DMGFontManager* Get_DMGFontMgr() const;

	// ---- Dungeon 전환 ----

	// 소유 Client의 Pending 방 표시 완료 보고. 서버는 검증을 GameState에 위임만 한다.
	UFUNCTION(Server, Reliable)
	void ServerReportDungeonRoomReady(FName InstanceKey, FName RoomId, int32 TransitionSerial);

	// 소유 Client의 안정 상태 Current 방 표시 완료 보고(늦은 합류). 서버는 검증을 GameState에 위임만 한다.
	UFUNCTION(Server, Reliable)
	void ServerReportDungeonCurrentRoomReady(FName InstanceKey, FName RoomId, int32 TransitionSerial);

	// 로컬 카메라 Fade 표현만 담당한다. Pawn 위치는 건드리지 않는다.
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Dungeon|Transition")
	void ClientSetDungeonFade(bool bFadeOut, float Duration);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// CountingDown Phase에서 인원/종료 서버 시간이 바뀔 때만 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "LB|Dungeon")
	void OnDungeonCountdownUpdated(int32 NearbyPlayers, int32 RequiredPlayers, int32 TotalPlayers,
	                               double CountdownEndServerTime);

	// CountingDown이 아닌 Phase로 바뀌면 즉시 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "LB|Dungeon")
	void OnDungeonCountdownHidden();

	// Enhanced Input Mapping Context 등록과 Action 바인딩 (로컬 컨트롤러에서만 호출됨)
	virtual void SetupInputComponent() override;

	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override; // 클라: PlayerState 도착 시점
	virtual void OnPossess(APawn* InPawn) override; // 서버/호스트

	void OnMoveTriggered(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<ULB_DMGFontManager> DamageNumberManager;

#pragma region INPUT_AREA
	// 에디터에서 생성한 /Game/LB/Input/IMC_Player를 BP_PlayerController Class Defaults에서 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputMappingContext> MoveMappingContext;

	// 에디터에서 생성한 /Game/LB/Input/IA_Move를 BP_PlayerController Class Defaults에서 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TArray<TObjectPtr<UInputAction>> SkillSlotActions; // A S D F (4개)

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TArray<TObjectPtr<UInputAction>> ItemSlotActions; // Num1~4 (4개)

	//스페이스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> DodgeAction;

	//인벤토리 - I
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> InvenAction;

	//스킬&스탯창 - K
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> SkillInfoAction;


	// 점프- C
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> JumpAction;
	// 평타- X
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> NormalAttackAction;
	// 기본스킬- Z
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> StrongAttackAction;

#pragma endregion

private:
#pragma region UI

	// MyPlayerController.h
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULB_HUD> HUDWidgetClass;

	UPROPERTY()
	ULB_HUD* HUDWidget;

	void InitUI();
	void BindHUD();
#pragma endregion

#pragma  region Action
	void Jump(const FInputActionValue& Value);
	void StopJumping();

	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void TurnOn_Inventory(const FInputActionValue& Value);
	void TurnOn_SkillInfo(const FInputActionValue& Value);
	void Dodge(const FInputActionValue& Value);
	void OnNormalAttackStarted(const FInputActionValue& Value);
	void StrongATK(const FInputActionValue& Value);

	void OnSkillSlot(int32 Index);
	void OnItemSlot(int32 Index);
	void Set_IATag();

	FGameplayTagContainer SprintTagContainer;
	FGameplayTagContainer DodgeTagContainer;

#pragma endregion


	void RefreshUIInputMode();
#pragma region Dungeon
	// GameState 도착 여부와 무관하게 여러 지점에서 호출해도 안전하다(중복 바인딩 없음).
	void TryBindDungeonGameState();
	void UnbindDungeonGameState();
	void HandleGameStateSet(AGameStateBase* NewGameState);

	void HandleLocalPendingRoomReady(FName InstanceKey, FName RoomId, int32 TransitionSerial);
	void ReportRoomReadyOnce(FName InstanceKey, FName RoomId, int32 TransitionSerial);

	void HandleLocalCurrentRoomReady(FName InstanceKey, FName RoomId, int32 TransitionSerial);
	void ReportCurrentRoomReadyOnce(FName InstanceKey, FName RoomId, int32 TransitionSerial);
	void ResetDungeonReportCache();

	UFUNCTION()
	void HandleDungeonTransitionChanged(const FLB_DungeonTransitionSnapshot& Snapshot);

	TWeakObjectPtr<ALB_GameState> BoundDungeonGameState;
	FDelegateHandle LocalPendingRoomReadyHandle;
	FDelegateHandle LocalCurrentRoomReadyHandle;
	FDelegateHandle GameStateSetHandle;

	// 중복 보고 방지.
	FName LastReportedInstanceKey = NAME_None;
	FName LastReportedRoomId = NAME_None;
	int32 LastReportedSerial = INDEX_NONE;

	// 안정 상태 Current 방 중복 보고 방지.
	FName LastReportedCurrentInstanceKey = NAME_None;
	FName LastReportedCurrentRoomId = NAME_None;
	int32 LastReportedCurrentSerial = INDEX_NONE;

	// 카운트다운 표현 상태.
	bool bDungeonCountdownVisible = false;
	int32 LastCountdownNearby = INDEX_NONE;
	int32 LastCountdownRequired = INDEX_NONE;
	int32 LastCountdownTotal = INDEX_NONE;
	double LastCountdownEndTime = -1.0;

	// Fade 표현 상태.
	bool bHasDungeonFadeRequest = false;
	bool bLastDungeonFadeOut = false;
#pragma endregion
};
