// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Component/LB_GasComponent.h"
#include "Component/LB_MeleeHitboxComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/LB_PlayerState.h"
#include "Utility/LB_NativeGameplayTag.h"

// Sets default values
ALB_PlayerCharacter::ALB_PlayerCharacter()
{
	// 이동/입력은 Enhanced Input 이벤트로 처리하므로 Tick 불필요 (ALB_CharacterBase 기본값인 false 유지)

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	GasComp = CreateDefaultSubobject<ULB_GasComponent>(TEXT("GasComp"));

	MeleeHitbox = CreateDefaultSubobject<ULB_MeleeHitboxComponent>(TEXT("MeleeHitbox"));
	MeleeHitbox->SetupAttachment(GetMesh());
	
}

UAbilitySystemComponent* ALB_PlayerCharacter::GetAbilitySystemComponent() const
{
	return GasComp ? GasComp->GetASC() : nullptr;
}

void ALB_PlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALB_PlayerCharacter, CurrentFacing);
}

// Called when the game starts or when spawned
void ALB_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Mesh의 현재(Right 기준) 상대 회전을 캐싱한 뒤, 초기 방향을 다시 적용해 서버/클라 모두 동일한 기준에서 시작
	BaseMeshRelativeRotation = GetMesh()->GetRelativeRotation();
	ApplyFacingToMesh(CurrentFacing);
}

void ALB_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitGAS();
}

void ALB_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitGAS();
}

void ALB_PlayerCharacter::InitGAS()
{
	if (ALB_PlayerState* PS = GetPlayerState<ALB_PlayerState>())
	{
		GasComp->InitFromPlayerState(PS);
	}
}

void ALB_PlayerCharacter::AddMoveInput(const FVector2D& Axis2D)
{
	// 방향키 대각선 입력이 축 단독 입력보다 빨라지지 않도록 최대 크기 1로 clamp
	const FVector2D ClampedAxis = Axis2D.GetClampedToMaxSize(1.f);

	if (ClampedAxis.IsNearlyZero())
	{
		return;
	}

	// 방향 잠금과 이동 차단은 Ability별로 조합할 수 있도록 서로 독립적으로 판단
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bFacingLocked = ASC && ASC->HasMatchingGameplayTag(TAG_State_Movement_FacingLocked);
	const bool bMovementBlocked = ASC && ASC->HasMatchingGameplayTag(TAG_State_Movement_Blocked);

	if (!bFacingLocked)
	{
		UpdateFacingFromInput(ClampedAxis.X);
	}

	if (bMovementBlocked)
	{
		return;
	}

	// 월드 X: 화면 좌우(Right:+X / Left:-X), 월드 Y: 화면 깊이(Up:+Y / Down:-Y)
	AddMovementInput(FVector::ForwardVector, ClampedAxis.X);
	AddMovementInput(FVector::RightVector, ClampedAxis.Y);
}

void ALB_PlayerCharacter::RequestBasicAttack()
{
	// GAS 초기화(InitAbilityActorInfo) 전이거나 Avatar가 이 캐릭터가 아니면 요청하지 않음
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || ASC->GetAvatarActor() != this)
	{
		return;
	}

	// 클라이언트에서 호출해도 ASC가 Ability의 Net Execution Policy에 따라 서버로 전달하며, 실행 가능 여부는 서버가 확정
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Basic));
}

void ALB_PlayerCharacter::UpdateFacingFromInput(float HorizontalInput)
{
	// X 입력이 임계값 이하면(Y 입력만 있거나 작은 노이즈) 기존 방향을 유지
	if (FMath::Abs(HorizontalInput) < FacingInputThreshold)
	{
		return;
	}

	SetFacing(HorizontalInput > 0.f ? ELB_FacingDirection::Right : ELB_FacingDirection::Left);
}

void ALB_PlayerCharacter::SetFacing(ELB_FacingDirection NewFacing)
{
	// 실제로 방향이 바뀔 때만 처리 (매 입력 프레임 RPC 전송 방지)
	if (CurrentFacing == NewFacing)
	{
		return;
	}

	// 소유 클라이언트/서버 모두 즉시 로컬 반영 (반응성 우선, 서버가 최종 권한)
	CurrentFacing = NewFacing;
	ApplyFacingToMesh(CurrentFacing);

	if (!HasAuthority())
	{
		ServerSetFacing(CurrentFacing);
	}
}

void ALB_PlayerCharacter::ApplyFacingToMesh(ELB_FacingDirection Facing)
{
	FRotator TargetRotation = BaseMeshRelativeRotation;
	if (Facing == ELB_FacingDirection::Left)
	{
		TargetRotation.Yaw += 180.f;
	}

	GetMesh()->SetRelativeRotation(TargetRotation);

	// CharacterMovementComponent의 네트워크 스무딩이 비로컬 인스턴스에서 매 틱
	// ACharacter::BaseRotationOffset(=PostInitializeComponents 시점의 Mesh 초기 회전)으로
	// Mesh Relative Rotation을 되돌리는 문제 수정. 현재 방향을 새 기준으로 갱신해 되돌림을 방지
	CacheInitialMeshOffset(GetMesh()->GetRelativeLocation(), TargetRotation);
}

void ALB_PlayerCharacter::ServerSetFacing_Implementation(ELB_FacingDirection NewFacing)
{
	SetFacing(NewFacing);
}

void ALB_PlayerCharacter::OnRep_Facing()
{
	ApplyFacingToMesh(CurrentFacing);
}

// Called to bind functionality to input
void ALB_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

