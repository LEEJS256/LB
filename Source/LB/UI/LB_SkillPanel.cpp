// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LB_SkillPanel.h"

#include "Component/LB_SkillComponent.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Data/LB_DA_SkillInfo.h"
#include "PlayerState/LB_PlayerState.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "TimerManager.h"
#include "UI/LB_SkillSlotEntry.h"
#include "UI/SlateWidget/LB_Button.h"

// ============================================================
// 생명주기
// ============================================================

void ULB_SkillPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Tab_Active)
		Tab_Active->OnClicked.AddUniqueDynamic(this, &ULB_SkillPanel::HandleTabActiveClicked);
	if (Tab_Passive)
		Tab_Passive->OnClicked.AddUniqueDynamic(this, &ULB_SkillPanel::HandleTabPassiveClicked);
}

void ULB_SkillPanel::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateTabVisuals();
	TryBindSkillComponent();
}

void ULB_SkillPanel::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}
	if (ULB_SkillComponent* Comp = SkillComp.Get())
	{
		Comp->OnSkillUpdated.RemoveDynamic(this, &ULB_SkillPanel::HandleSkillUpdated);
	}
	SkillComp.Reset();
	Super::NativeDestruct();
}

// ============================================================
// 바인딩
// ============================================================

void ULB_SkillPanel::TryBindSkillComponent()
{
	if (SkillComp.IsValid())
		return;

	if (const ALB_PlayerState* PS = GetOwningPlayerState<ALB_PlayerState>())
	{
		if (ULB_SkillComponent* Comp = PS->Get_SkillComponent())
		{
			SkillComp = Comp;
			Comp->OnSkillUpdated.AddUniqueDynamic(this, &ULB_SkillPanel::HandleSkillUpdated);
			Rebuild();
			return;
		}
	}

	// 클라에서 PS가 아직 복제 전이면 잠깐 뒤 재시도
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryHandle, this, &ULB_SkillPanel::TryBindSkillComponent, 0.2f, false);
	}
}

void ULB_SkillPanel::HandleSkillUpdated()
{
	Rebuild();
}

// ============================================================
// 탭
// ============================================================

void ULB_SkillPanel::SetShowPassive(bool bInShowPassive)
{
	if (bShowPassive == bInShowPassive)
		return;

	bShowPassive = bInShowPassive;
	UpdateTabVisuals();
	Rebuild();
}

void ULB_SkillPanel::HandleTabActiveClicked()  { SetShowPassive(false); }
void ULB_SkillPanel::HandleTabPassiveClicked() { SetShowPassive(true); }

void ULB_SkillPanel::UpdateTabVisuals()
{
	if (Tab_Active)
		Tab_Active->SetBorderColor(!bShowPassive ? TabActiveBorderColor : TabInactiveBorderColor);
	if (Tab_Passive)
		Tab_Passive->SetBorderColor(bShowPassive ? TabActiveBorderColor : TabInactiveBorderColor);
}

// ============================================================
// 목록
// ============================================================

void ULB_SkillPanel::Rebuild()
{
	const ULB_SkillComponent* Comp = SkillComp.Get();

	if (SkillPointText)
	{
		const int32 SP = Comp ? Comp->GetSkillPoints() : 0;
		SkillPointText->SetText(FText::Format(NSLOCTEXT("LB", "SkillPoint", "스킬 포인트 : {0}"), SP));
	}

	if (!SkillGrid || !EntryClass || !Comp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillPanel] Rebuild 중단: Grid=%d EntryClass=%d Comp=%d"),
			SkillGrid != nullptr, EntryClass != nullptr, Comp != nullptr);
		return;
	}

	SkillGrid->ClearChildren();
	EntryWidgets.Reset();

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	const TArray<FName>& Skills = Comp->GetAvailableSkills();
	const int32 Cols = FMath::Max(1, Columns);

	int32 Visible = 0;
	for (int32 i = 0; i < Skills.Num(); ++i)
	{
		const FName Code = Skills[i];
		const ULB_DA_SkillInfo* Info = Data ? Data->GetSkillInfo(Code) : nullptr;
		if (!Info)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SkillPanel] %s: 마스터 스킬 DT에 없음"), *Code.ToString());
			continue;
		}
		if (Info->bIsPassive != bShowPassive)
			continue;

		ULB_SkillSlotEntry* Entry = CreateWidget<ULB_SkillSlotEntry>(this, EntryClass);
		if (!Entry)
			continue;

		// SlotIndex는 AvailableSkills 인덱스 (퀵슬롯 드롭 때 이걸로 스킬 코드를 찾는다)
		Entry->SetSkill(i, Code, Info, Comp->GetSkillLevel(Code));
		Entry->SetSelected(Code == SelectedSkill);
		Entry->OnClicked.AddUObject(this, &ULB_SkillPanel::HandleEntryClicked);

		SkillGrid->AddChildToUniformGrid(Entry, Visible / Cols, Visible % Cols);
		EntryWidgets.Add(Entry);
		++Visible;
	}
}

void ULB_SkillPanel::HandleEntryClicked(FName SkillCode)
{
	SelectedSkill = SkillCode;

	for (ULB_SkillSlotEntry* Entry : EntryWidgets)
	{
		if (Entry)
		{
			Entry->SetSelected(Entry->GetSkillCode() == SkillCode);
		}
	}

	OnSkillSelected.Broadcast(SkillCode);
}
