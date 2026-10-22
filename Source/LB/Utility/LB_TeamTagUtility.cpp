// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/LB_TeamTagUtility.h"

#include "AbilitySystemComponent.h"
#include "Utility/LB_NativeGameplayTag.h"

FGameplayTag LBTeamTagUtility::FindTeamTag(const UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return FGameplayTag();
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	for (const FGameplayTag& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(TAG_Team) && Tag != TAG_Team)
		{
			return Tag;
		}
	}
	return FGameplayTag();
}
