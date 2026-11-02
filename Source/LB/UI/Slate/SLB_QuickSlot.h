// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class LB_API SLB_QuickSlot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLB_QuickSlot)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
};
