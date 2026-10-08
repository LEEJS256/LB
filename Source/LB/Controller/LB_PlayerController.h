// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LB_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 *
 */
UCLASS()
class LB_API ALB_PlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	// Enhanced Input Mapping Context 등록과 Action 바인딩 (로컬 컨트롤러에서만 호출됨)
	virtual void SetupInputComponent() override;

	void OnMoveTriggered(const FInputActionValue& Value);

#pragma region INPUT_AREA
	// 에디터에서 생성한 /Game/LB/Input/IMC_Player를 BP_PlayerController Class Defaults에서 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputMappingContext> MoveMappingContext;

	// 에디터에서 생성한 /Game/LB/Input/IA_Move를 BP_PlayerController Class Defaults에서 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LB|Input")
	TObjectPtr<UInputAction> DashAction;
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
#pragma  region Action
	void Jump(const FInputActionValue& Value);
	void StopJumping();

	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void OnNormalAttackStarted(const FInputActionValue& Value);
	void StrongATK(const FInputActionValue& Value);

	
#pragma endregion
};
