// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"

class UAbilitySystemComponent;

namespace LBTeamTagUtility
{
	// ASC가 보유한 Team 하위 태그(Team.Player, Team.Monster 등). ASC가 null이거나 하위 태그가 없으면 빈 태그
	// Team 루트 태그 자체는 팀 값으로 인정하지 않음
	FGameplayTag FindTeamTag(const UAbilitySystemComponent* ASC);
}
