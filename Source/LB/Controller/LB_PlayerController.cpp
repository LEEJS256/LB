// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_PlayerController.h"
#include "PlayerState/LB_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "Character/LB_PlayerCharacter.h"
#include "Component/LB_BattleComponent.h"
#include "Controller/LB_MonsterAIController.h"
#include "Subsystem/LB_DMGFontManager.h"
#include "UI/LB_PlayerStatus.h"
#include "UI/LB_HUD.h"
#include "GameMode/LB_GameState.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"


ALB_PlayerController::ALB_PlayerController()
{
	DamageNumberManager = CreateDefaultSubobject<ULB_DMGFontManager>(TEXT("DMGFontManager"));
}

void ALB_PlayerController::CloseInventory()
{
	if (!HUDWidget || !HUDWidget->IsInventoryVisible()) return;

	HUDWidget->SetInventoryVisible(false);
	SetInventoryUIMode(false);
}

class ULB_DMGFontManager* ALB_PlayerController::Get_DMGFontMgr() const
{
	return DamageNumberManager;
}



void ALB_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>())
		{
			if (MoveMappingContext)
			{
				Subsystem->AddMappingContext(MoveMappingContext, 0);
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,
			                                   &ALB_PlayerController::OnMoveTriggered);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this,
			                                   &ALB_PlayerController::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
			                                   &ALB_PlayerController::StopJumping);
		}
		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this,
			                                   &ALB_PlayerController::StartSprint);
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Completed, this,
			                                   &ALB_PlayerController::StopSprint);
		}
		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this,
			                                   &ALB_PlayerController::Dodge);
		}
		if (InvenAction)
		{
			EnhancedInputComponent->BindAction(InvenAction, ETriggerEvent::Started, this,
											   &ALB_PlayerController::TurnOn_Inventory);
		}
		if (NormalAttackAction)
		{
			EnhancedInputComponent->BindAction(NormalAttackAction, ETriggerEvent::Started, this,
			                                   &ALB_PlayerController::OnNormalAttackStarted);
		}
		if (StrongAttackAction)
		{
			EnhancedInputComponent->BindAction(StrongAttackAction, ETriggerEvent::Completed, this,
			                                   &ALB_PlayerController::StrongATK);
		}

	
		for (int32 i = 0; i < SkillSlotActions.Num(); ++i)
		{
			EnhancedInputComponent->BindAction(SkillSlotActions[i], ETriggerEvent::Started, this, &ThisClass::OnSkillSlot, i);
		}
		for (int32 i = 0; i < ItemSlotActions.Num(); ++i)
		{
			EnhancedInputComponent->BindAction(ItemSlotActions[i], ETriggerEvent::Started, this, &ThisClass::OnItemSlot, i);
		}
	}
}

void ALB_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	Set_IATag();
	InitUI();
	BindHUD();
	TryBindDungeonGameState();
}

void ALB_PlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitUI();
	BindHUD();
	TryBindDungeonGameState();
}

void ALB_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindDungeonGameState();

	if (GameStateSetHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GameStateSetEvent.Remove(GameStateSetHandle);
		}
		GameStateSetHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

#pragma region Dungeon

void ALB_PlayerController::TryBindDungeonGameState()
{
	// 표현/보고는 로컬 Controller 몫. 서버의 원격 Controller는 바인딩하지 않는다.
	// BeginPlay 이전(OnRep_PlayerState 선도착)에는 BeginPlay에서 다시 시도한다.
	if (!IsLocalController() || (!HasActorBegunPlay() && !IsActorBeginningPlay()))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ALB_GameState* GS = World->GetGameState<ALB_GameState>();
	if (!GS)
	{
		// GameState 지연 도착: 한 번만 대기 등록한다.
		if (!GameStateSetHandle.IsValid())
		{
			GameStateSetHandle = World->GameStateSetEvent.AddUObject(this, &ThisClass::HandleGameStateSet);
		}
		return;
	}

	if (GameStateSetHandle.IsValid())
	{
		World->GameStateSetEvent.Remove(GameStateSetHandle);
		GameStateSetHandle.Reset();
	}

	if (BoundDungeonGameState.Get() == GS)
	{
		return; // 이미 바인딩됨
	}

	UnbindDungeonGameState();

	BoundDungeonGameState = GS;
	LocalPendingRoomReadyHandle = GS->OnLocalPendingRoomReady.AddUObject(this, &ThisClass::HandleLocalPendingRoomReady);
	GS->OnDungeonTransitionChanged.AddUniqueDynamic(this, &ThisClass::HandleDungeonTransitionChanged);

	// 바인딩 이전에 이미 일어난 상태를 따라잡는다.
	if (GS->HasDungeonRoute())
	{
		const FLB_DungeonTransitionSnapshot& Snapshot = GS->GetDungeonTransition();
		HandleDungeonTransitionChanged(Snapshot);

		// Client World에서 Pending 방이 이미 표시됐다면 놓친 준비 보고를 보낸다.
		if (!HasAuthority() && !Snapshot.PendingRoomId.IsNone() && GS->GetPendingRoomLevel() != nullptr)
		{
			ReportRoomReadyOnce(GS->GetDungeonRoute().InstanceKey, Snapshot.PendingRoomId, Snapshot.TransitionSerial);
		}
	}
}

void ALB_PlayerController::UnbindDungeonGameState()
{
	if (ALB_GameState* GS = BoundDungeonGameState.Get())
	{
		GS->OnLocalPendingRoomReady.Remove(LocalPendingRoomReadyHandle);
		GS->OnDungeonTransitionChanged.RemoveDynamic(this, &ThisClass::HandleDungeonTransitionChanged);
	}
	LocalPendingRoomReadyHandle.Reset();
	BoundDungeonGameState.Reset();
}

void ALB_PlayerController::HandleGameStateSet(AGameStateBase* NewGameState)
{
	TryBindDungeonGameState();
}

void ALB_PlayerController::HandleLocalPendingRoomReady(FName InstanceKey, FName RoomId, int32 TransitionSerial)
{
	ReportRoomReadyOnce(InstanceKey, RoomId, TransitionSerial);
}

void ALB_PlayerController::ReportRoomReadyOnce(FName InstanceKey, FName RoomId, int32 TransitionSerial)
{
	if (!IsLocalController() || HasAuthority() || InstanceKey.IsNone() || RoomId.IsNone())
	{
		return;
	}

	if (LastReportedInstanceKey == InstanceKey && LastReportedRoomId == RoomId && LastReportedSerial == TransitionSerial)
	{
		return;
	}

	LastReportedInstanceKey = InstanceKey;
	LastReportedRoomId = RoomId;
	LastReportedSerial = TransitionSerial;
	ServerReportDungeonRoomReady(InstanceKey, RoomId, TransitionSerial);
}

void ALB_PlayerController::ServerReportDungeonRoomReady_Implementation(FName InstanceKey, FName RoomId, int32 TransitionSerial)
{
	// 검증과 판정은 GameState/GameMode 몫. 여기서는 보고자와 입력만 전달한다.
	if (ALB_GameState* GS = GetWorld() ? GetWorld()->GetGameState<ALB_GameState>() : nullptr)
	{
		GS->ReceiveClientRoomReady(this, InstanceKey, RoomId, TransitionSerial);
	}
}

void ALB_PlayerController::HandleDungeonTransitionChanged(const FLB_DungeonTransitionSnapshot& Snapshot)
{
	if (!IsLocalController())
	{
		return;
	}

	if (Snapshot.Phase != ELB_DungeonTransitionPhase::CountingDown)
	{
		if (bDungeonCountdownVisible)
		{
			bDungeonCountdownVisible = false;
			LastCountdownNearby = INDEX_NONE;
			LastCountdownRequired = INDEX_NONE;
			LastCountdownTotal = INDEX_NONE;
			LastCountdownEndTime = -1.0;
			OnDungeonCountdownHidden();
		}
		return;
	}

	const bool bChanged = !bDungeonCountdownVisible
		|| LastCountdownNearby != Snapshot.NearbyPlayers
		|| LastCountdownRequired != Snapshot.RequiredPlayers
		|| LastCountdownTotal != Snapshot.TotalPlayers
		|| LastCountdownEndTime != Snapshot.CountdownEndServerTime;
	if (!bChanged)
	{
		return;
	}

	bDungeonCountdownVisible = true;
	LastCountdownNearby = Snapshot.NearbyPlayers;
	LastCountdownRequired = Snapshot.RequiredPlayers;
	LastCountdownTotal = Snapshot.TotalPlayers;
	LastCountdownEndTime = Snapshot.CountdownEndServerTime;
	OnDungeonCountdownUpdated(Snapshot.NearbyPlayers, Snapshot.RequiredPlayers, Snapshot.TotalPlayers, Snapshot.CountdownEndServerTime);
}

void ALB_PlayerController::ClientSetDungeonFade_Implementation(bool bFadeOut, float Duration)
{
	// 로컬 카메라 표현만. Pawn/Transform은 서버가 소유한다.
	if (!IsLocalController() || !PlayerCameraManager)
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.f, Duration);
	constexpr float MinFadeDuration = KINDA_SMALL_NUMBER;

	if (bFadeOut)
	{
		// 중복 Fade Out도 현재 알파에서 이어가므로 깜빡임 없이 갱신된다.
		if (SafeDuration <= MinFadeDuration)
		{
			PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
		}
		else
		{
			const float FromAlpha = FMath::Clamp(PlayerCameraManager->FadeAmount, 0.f, 1.f);
			PlayerCameraManager->StartCameraFade(FromAlpha, 1.f, SafeDuration, FLinearColor::Black, false, true);
		}
	}
	else
	{
		// 이미 Fade In 중/완료라면 1로 되돌리지 않는다.
		if (bHasDungeonFadeRequest && !bLastDungeonFadeOut)
		{
			return;
		}

		if (SafeDuration <= MinFadeDuration)
		{
			PlayerCameraManager->StopCameraFade();
		}
		else
		{
			PlayerCameraManager->StartCameraFade(1.f, 0.f, SafeDuration, FLinearColor::Black, false, false);
		}
	}

	bHasDungeonFadeRequest = true;
	bLastDungeonFadeOut = bFadeOut;
}

#pragma endregion

void ALB_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	InitUI();
	BindHUD();

	// 타깃이 없어 대기 중인 몬스터가 새 플레이어를 다시 탐색하도록 알림
	ALB_MonsterAIController::NotifyPlayerAvailable(GetWorld());
}

void ALB_PlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	if (ALB_PlayerCharacter* LBCharacter = Cast<ALB_PlayerCharacter>(GetPawn()))
	{
		LBCharacter->AddMoveInput(Value.Get<FVector2D>());
	}
}

void ALB_PlayerController::InitUI()
{
	if (HUDWidget || !IsLocalController() || !HUDWidgetClass) return;

	HUDWidget = CreateWidget<ULB_HUD>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void ALB_PlayerController::BindHUD()
{
	if (!HUDWidget)
		return;

	ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>();
	if (!PS)
		return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;


	if (ULB_PlayerStatus* Status = HUDWidget->GetLocalPlayerStatus())
	{
		Status->BindToASC(ASC);
	}
}

void ALB_PlayerController::Jump(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
		return;

	ALB_PlayerCharacter* PlayerCharacter = Cast<ALB_PlayerCharacter>(ControlledPawn);
	if (!IsValid(PlayerCharacter))
		return;


	PlayerCharacter->Jump();
}

void ALB_PlayerController::StopJumping()
{
	if (ACharacter* pCharacter = Cast<ACharacter>(GetPawn()))
	{
		pCharacter->StopJumping();
	}
}

void ALB_PlayerController::StartSprint(const FInputActionValue& Value)
{
	ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>();
	if (!PS)
	{
		return;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;
	if (!ASC || ASC->GetAvatarActor() != GetPawn())
		return;

	ASC->TryActivateAbilitiesByTag(SprintTagContainer);


}

void ALB_PlayerController::StopSprint(const FInputActionValue& Value)
{
	ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>();
	if (!PS)
	{
		return;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;


	ASC->CancelAbilities(&SprintTagContainer);
}

void ALB_PlayerController::TurnOn_Inventory(const FInputActionValue& Value)
{
	if (!IsLocalController() || !HUDWidget) return;

	if (HUDWidget->IsInventoryVisible()) { CloseInventory(); return; }

	HUDWidget->SetInventoryVisible(true);
	SetInventoryUIMode(true);
}

void ALB_PlayerController::Dodge(const FInputActionValue& Value)
{
	ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>();
	if (!PS)
	{
		return;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;
	if (!ASC || ASC->GetAvatarActor() != GetPawn())
		return;

	ASC->TryActivateAbilitiesByTag(DodgeTagContainer);
}

void ALB_PlayerController::OnNormalAttackStarted(const FInputActionValue& Value)
{
	// 공격 입력 1회당 BattleComponent에 기본 공격 요청을 1회 전달 (실행 가능 여부는 GAS/서버가 확정)
	ALB_PlayerCharacter* LBCharacter = Cast<ALB_PlayerCharacter>(GetPawn());
	if (!LBCharacter)
		return;

	ULB_BattleComponent* BattleComp = LBCharacter->GetBattleComponent();
	if (!BattleComp)
		return;

	BattleComp->RequestComboInput(TAG_Input_Normal);
}

void ALB_PlayerController::StrongATK(const FInputActionValue& Value)
{
	// 강공격 입력 1회당 BattleComponent에 강공격 요청을 1회 전달 (실행 가능 여부는 GAS/서버가 확정)
	ALB_PlayerCharacter* LBCharacter = Cast<ALB_PlayerCharacter>(GetPawn());
	if (!LBCharacter)
		return;

	ULB_BattleComponent* BattleComp = LBCharacter->GetBattleComponent();
	if (!BattleComp)
		return;

	BattleComp->RequestComboInput(TAG_Input_Strong);
}

void ALB_PlayerController::OnSkillSlot(int32 Index)
{
	// if (ULB_QuickSlotComponent* QS = GetPlayerState<ALB_PlayerState>()->GetQuickSlotComponent())
	// 	QS->UseSkillSlot(Index);
}

void ALB_PlayerController::OnItemSlot(int32 Index)
{
	// if (ULB_QuickSlotComponent* QS = GetPlayerState<ALB_PlayerState>()->GetQuickSlotComponent())
	// 	QS->UseSkillSlot(Index);
}

void ALB_PlayerController::Set_IATag()
{
	SprintTagContainer.AddTag(TAG_Ability_Sprint);
	DodgeTagContainer.AddTag(TAG_Ability_Dodge);
}

void ALB_PlayerController::SetInventoryUIMode(bool bOpen)
{
	if (bOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		bShowMouseCursor = true;
		SetIgnoreLookInput(true);    // 열려 있는 동안 카메라가 마우스를 따라 돌지 않게
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		SetIgnoreLookInput(false);
	}
}
