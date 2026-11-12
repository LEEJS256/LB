// LB_MonsterHPBar.cpp
#include "LB_MonsterHPBar.h"
#include "UI/Slate/SlateWidget/LB_NeonProgressBar.h" 

void ULB_MonsterHPBar::SetHealth(float Current, float Max)
{
	const float Percent = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;
	if (HPBar)
	{
		HPBar->SetPercent(Percent); // TODO: 네온바의 퍼센트 세터 이름
	}
}