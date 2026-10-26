// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Item/LB_DropItem.h"

#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ALB_DropItem::ALB_DropItem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;


	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(Sphere);
	Sphere->SetSphereRadius(60.f);
	Sphere->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));

	GradeEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GradeEffect"));
	GradeEffect->SetupAttachment(Sphere);
	GradeEffect->bAutoActivate = false;
}

void ALB_DropItem::InitDrop(FName InItemCode, int32 InCount)
{
	ItemCode = InItemCode;
	Count = InCount;
	RefreshVisual(); // 서버(호스트)는 OnRep이 안 불리니까 직접 호출
}

void ALB_DropItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALB_DropItem, ItemCode);
	DOREPLIFETIME(ALB_DropItem, Count);
}

// Called when the game starts or when spawned
void ALB_DropItem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &ALB_DropItem::OnSphereBeginOverlap);
	}
	RefreshVisual(); // 리슨서버/이미 값이 세팅된 경우 대비
}

void ALB_DropItem::OnRep_ItemCode()
{
	RefreshVisual();
}

void ALB_DropItem::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                        const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	const APawn* Pawn = Cast<APawn>(OtherActor);
	APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;
	if (!PS) return;

	// ULB_InventoryComponent* Inv = PS->FindComponentByClass<ULB_InventoryComponent>();
	// if (!Inv) return;
	// const int32 Remain = Inv->AddItem(ItemCode, Count); // 못 넣은 수량 반환
	// if (Remain <= 0) Destroy();
	// else Count = Remain;
}

void ALB_DropItem::RefreshVisual()
{
	if (ItemCode.IsNone())
		return;

	// const ULB_DA_ItemInfo* Info = GetGameInstance()->GetSubsystem<ULB_ItemSubsystem>()->GetItemInfo(ItemCode);
	// if (!Info) return;
	// if (TSoftObjectPtr<UNiagaraSystem>* Found = GradeEffectMap.Find(Info->ItemGradeTag))
	// {
	//     GradeEffect->SetAsset(Found->LoadSynchronous());
	//     GradeEffect->Activate(true);
	// }
}


