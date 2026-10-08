// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_MeleeHitboxComponent.h"

#include "LB.h"
#include "Character/LB_PlayerCharacter.h"

ULB_MeleeHitboxComponent::ULB_MeleeHitboxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);

	// 기본 비활성: 판정 구간에서만 QueryOnly + Overlap Event 활성화
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CanCharacterStepUpOn = ECB_No;
	SetHiddenInGame(true);
}

void ULB_MeleeHitboxComponent::BeginPlay()
{
	Super::BeginPlay();

	DefaultAttachParent = GetAttachParent();
	OnComponentBeginOverlap.AddDynamic(this, &ULB_MeleeHitboxComponent::HandleBeginOverlap);
}

void ULB_MeleeHitboxComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateHitWindow();
	OnComponentBeginOverlap.RemoveDynamic(this, &ULB_MeleeHitboxComponent::HandleBeginOverlap);

	Super::EndPlay(EndPlayReason);
}

void ULB_MeleeHitboxComponent::BeginHitWindow(const FLB_MeleeHitboxSettings& Settings)
{
	// 이전 구간이 정리되지 않았더라도 새 구간은 깨끗한 상태에서 시작
	DeactivateHitWindow();

	bool bUseSocket = false;
	if (DefaultAttachParent && Settings.SocketName != NAME_None)
	{
		bUseSocket = DefaultAttachParent->DoesSocketExist(Settings.SocketName);
		if (!bUseSocket)
		{
			UE_LOG(LogLB, Warning, TEXT("%s: Socket '%s' not found on %s. Using character axes."),
				*GetName(), *Settings.SocketName.ToString(), *GetNameSafe(DefaultAttachParent));
		}
	}

	if (bUseSocket)
	{
		// Socket 기준: Mesh Socket에 부착해 Mesh의 좌우 회전을 그대로 따름
		if (GetAttachParent() != DefaultAttachParent || GetAttachSocketName() != Settings.SocketName)
		{
			AttachToComponent(DefaultAttachParent, FAttachmentTransformRules::KeepRelativeTransform, Settings.SocketName);
		}

		SetBoxExtent(Settings.BoxExtent, false);
		SetRelativeLocationAndRotation(Settings.RelativeLocation, Settings.RelativeRotation);
	}
	else
	{
		ApplyCharacterSpaceSettings(Settings);
	}

	HitActors.Reset();
	bHitWindowActive = true;

	SetGenerateOverlapEvents(true);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 활성화 직후 이미 Box 안에 있던 대상도 누락하지 않도록 현재 Overlap을 갱신해 확인
	UpdateOverlaps();

	TArray<UPrimitiveComponent*> CurrentOverlaps;
	GetOverlappingComponents(CurrentOverlaps);
	for (UPrimitiveComponent* OtherComp : CurrentOverlaps)
	{
		if (!bHitWindowActive)
		{
			// Delegate 처리 중 판정이 종료될 수 있음
			break;
		}
		TryRegisterHit(OtherComp ? OtherComp->GetOwner() : nullptr, OtherComp, nullptr);
	}
}

void ULB_MeleeHitboxComponent::ApplyCharacterSpaceSettings(const FLB_MeleeHitboxSettings& Settings)
{
	AActor* OwnerActor = GetOwner();
	USceneComponent* CharacterRoot = OwnerActor ? OwnerActor->GetRootComponent() : nullptr;
	if (!CharacterRoot || CharacterRoot == this)
	{
		SetBoxExtent(Settings.BoxExtent, false);
		SetRelativeLocationAndRotation(Settings.RelativeLocation, Settings.RelativeRotation);
		return;
	}

	// Mesh는 기본 Yaw 오프셋과 좌우 회전을 가지므로 Mesh 로컬 축 대신 Character(Capsule)에 부착
	if (GetAttachParent() != CharacterRoot || GetAttachSocketName() != NAME_None)
	{
		AttachToComponent(CharacterRoot, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// Actor 회전은 Controller를 따를 수 있으므로 이동/방향과 같은 월드 X(공격)/Y(깊이)/Z(높이) 축을 사용
	const ALB_PlayerCharacter* PlayerCharacter = Cast<ALB_PlayerCharacter>(OwnerActor);
	const bool bFacingLeft = PlayerCharacter && PlayerCharacter->GetFacing() == ELB_FacingDirection::Left;

	FVector Offset = Settings.RelativeLocation;
	FRotator Rotation = Settings.RelativeRotation;
	if (bFacingLeft)
	{
		// X축 거울 반전: 위치 X 반전, 회전은 Pitch/Yaw 반전(Roll 유지)
		Offset.X = -Offset.X;
		Rotation.Pitch = -Rotation.Pitch;
		Rotation.Yaw = -Rotation.Yaw;
	}

	SetBoxExtent(Settings.BoxExtent, false);
	SetWorldLocationAndRotation(OwnerActor->GetActorLocation() + Offset, Rotation);
}

void ULB_MeleeHitboxComponent::EndHitWindow()
{
	DeactivateHitWindow();
}

void ULB_MeleeHitboxComponent::ForceEndHitWindow()
{
	DeactivateHitWindow();
}

void ULB_MeleeHitboxComponent::DeactivateHitWindow()
{
	bHitWindowActive = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	HitActors.Reset();
}

void ULB_MeleeHitboxComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryRegisterHit(OtherActor, OtherComp, bFromSweep ? &SweepResult : nullptr);
}

void ULB_MeleeHitboxComponent::TryRegisterHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult* SweepResult)
{
	if (!bHitWindowActive || !IsValid(OtherActor))
	{
		return;
	}

	// 게임플레이 명중은 서버에서만 확정. 클라이언트의 Box는 표현/디버그 용도
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (OtherActor == OwnerActor)
	{
		return;
	}

	// 같은 판정 구간에서 대상당 1회만 명중
	bool bAlreadyHit = false;
	HitActors.Add(OtherActor, &bAlreadyHit);
	if (bAlreadyHit)
	{
		return;
	}

	FHitResult HitResult;
	if (SweepResult)
	{
		HitResult = *SweepResult;
	}
	else
	{
		const FVector TargetLocation = OtherComp ? OtherComp->GetComponentLocation() : OtherActor->GetActorLocation();
		const FVector HitNormal = (GetComponentLocation() - TargetLocation).GetSafeNormal();
		HitResult = FHitResult(OtherActor, OtherComp, TargetLocation, HitNormal);
	}

	OnMeleeHit.Broadcast(OtherActor, HitResult);
}
