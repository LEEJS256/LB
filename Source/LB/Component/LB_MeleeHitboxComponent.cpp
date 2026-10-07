// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_MeleeHitboxComponent.h"

#include "LB.h"

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

	if (DefaultAttachParent)
	{
		FName AttachSocket = Settings.SocketName;
		if (AttachSocket != NAME_None && !DefaultAttachParent->DoesSocketExist(AttachSocket))
		{
			UE_LOG(LogLB, Warning, TEXT("%s: Socket '%s' not found on %s. Using parent origin."),
				*GetName(), *AttachSocket.ToString(), *GetNameSafe(DefaultAttachParent));
			AttachSocket = NAME_None;
		}

		if (GetAttachParent() != DefaultAttachParent || GetAttachSocketName() != AttachSocket)
		{
			AttachToComponent(DefaultAttachParent, FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);
		}
	}

	SetBoxExtent(Settings.BoxExtent, false);
	SetRelativeLocationAndRotation(Settings.RelativeLocation, Settings.RelativeRotation);

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
