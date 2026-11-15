// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/LB_AnimNotifyState_MeleeHitWindow.h"

#include "Components/SkeletalMeshComponent.h"

void ULB_AnimNotifyState_MeleeHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
#if WITH_EDITOR
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
	if (!bDrawPreview || !World || World->WorldType != EWorldType::EditorPreview)
	{
		return;
	}

	const FLB_MeleeHitboxSettings& S = HitboxSettings;
	FTransform BoxTM;

	if (S.SocketName != NAME_None && MeshComp->DoesSocketExist(S.SocketName))
	{
		// 컴포넌트의 소켓 부착 + Relative 설정과 동일
		BoxTM = FTransform(S.RelativeRotation, S.RelativeLocation) * MeshComp->GetSocketTransform(S.SocketName);
	}
	else
	{
		// ApplyCharacterSpaceSettings와 동일한 반전 규칙
		FVector Offset = S.RelativeLocation;
		FRotator Rot = S.RelativeRotation;
		if (bPreviewMirrorX)
		{
			Offset.X = -Offset.X;
			Rot.Pitch = -Rot.Pitch;
			Rot.Yaw = -Rot.Yaw;
		}
		// 프리뷰엔 캡슐이 없으니 Mesh 트랜스폼을 역산해서 가상의 캐릭터 위치를 구함
		const FTransform CharTM = PreviewMeshRelativeTransform.Inverse() * MeshComp->GetComponentTransform();
		BoxTM = FTransform(Rot, CharTM.GetLocation() + Offset);
	}

	DrawDebugBox(World, BoxTM.GetLocation(), S.BoxExtent, BoxTM.GetRotation(),
		FColor::Red, false, -1.f, 0, 1.5f);
#endif
}

void ULB_AnimNotifyState_MeleeHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                     float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 에디터 미리보기 Actor처럼 판정 컴포넌트가 없으면 아무것도 하지 않음
	if (ULB_MeleeHitboxComponent* Hitbox = FindHitbox(MeshComp))
	{
		Hitbox->BeginHitWindow(HitboxSettings);
	}
}

void ULB_AnimNotifyState_MeleeHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (ULB_MeleeHitboxComponent* Hitbox = FindHitbox(MeshComp))
	{
		Hitbox->EndHitWindow();
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString ULB_AnimNotifyState_MeleeHitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Melee Hit Window");
}

ULB_MeleeHitboxComponent* ULB_AnimNotifyState_MeleeHitWindow::FindHitbox(const USkeletalMeshComponent* MeshComp)
{
	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<ULB_MeleeHitboxComponent>() : nullptr;
}
