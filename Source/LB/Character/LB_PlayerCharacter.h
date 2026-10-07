// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LB_CharacterBase.h"
#include "GameFramework/Character.h"
#include "LB_PlayerCharacter.generated.h"

class ULB_GasComponent;
class USpringArmComponent;
class UCameraComponent;
class UAbilitySystemComponent;

// 벨트스크롤 캐릭터는 좌/우 두 방향만 가진다
UENUM(BlueprintType)
enum class ELB_FacingDirection : uint8
{
	Right,
	Left
};

UCLASS()
class LB_API ALB_PlayerCharacter : public ALB_CharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALB_PlayerCharacter();

	//ASC = GAS본체
	UAbilitySystemComponent* GetAbilitySystemComponent() const ;

	// ALB_PlayerController가 IA_Move의 2D Axis 값을 전달할 때 호출 (Right:+X, Up:+Y 기준)
	void AddMoveInput(const FVector2D& Axis2D);

	// ALB_PlayerController가 공격 입력 시 호출. Ability.Attack.Basic 태그 Ability의 활성화를 ASC에 요청
	void RequestBasicAttack();

	ELB_FacingDirection GetFacing() const { return CurrentFacing; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override; // 서버
	virtual void OnRep_PlayerState() override;

	void InitGAS();

#pragma region FACING_AREA
	// X 입력 부호로 좌/우 방향을 결정 (Y 입력만 있으면 기존 방향 유지)
	void UpdateFacingFromInput(float HorizontalInput);

	// 소유 클라이언트는 즉시 반영, 변경이 있을 때만 서버로 전파
	void SetFacing(ELB_FacingDirection NewFacing);

	// BaseMeshRelativeRotation 기준으로 Mesh 상대 회전만 변경 (Actor 회전은 건드리지 않음)
	// CacheInitialMeshOffset()으로 ACharacter::BaseRotationOffset도 갱신해야 해서 const가 아님
	void ApplyFacingToMesh(ELB_FacingDirection Facing);

	UFUNCTION(Server, Reliable)
	void ServerSetFacing(ELB_FacingDirection NewFacing);

	UFUNCTION()
	void OnRep_Facing();

	// 서버 권한, OnRep으로 다른 클라이언트에 복제. 초기값은 Right
	UPROPERTY(ReplicatedUsing = OnRep_Facing, VisibleAnywhere, BlueprintReadOnly, Category = "LB|Facing")
	ELB_FacingDirection CurrentFacing = ELB_FacingDirection::Right;

	// 작은 아날로그 노이즈로 방향이 바뀌지 않도록 하는 X축 임계값
	static constexpr float FacingInputThreshold = 0.1f;

	// BeginPlay에서 캐싱하는 Mesh의 기본 상대 회전(Right 기준)
	FRotator BaseMeshRelativeRotation = FRotator::ZeroRotator;
#pragma endregion

#pragma region COMPONENT_AREA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<ULB_GasComponent> GasComp;

#pragma endregion
public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
