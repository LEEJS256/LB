// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/LB_NativeGameplayTag.h"


UE_DEFINE_GAMEPLAY_TAG(TAG_Team, "Team");
UE_DEFINE_GAMEPLAY_TAG(TAG_Team_Player, "Team.Player");
UE_DEFINE_GAMEPLAY_TAG(TAG_Team_Monster, "Team.Monster");

UE_DEFINE_GAMEPLAY_TAG(TAG_ATK_Normal, "ATK.Normal");
UE_DEFINE_GAMEPLAY_TAG(TAG_ATK_Strong, "ATK.Strong");
UE_DEFINE_GAMEPLAY_TAG(TAG_ATK_Ultimate, "ATK.Ultimate");

UE_DEFINE_GAMEPLAY_TAG(TAG_ATKType, "ATKType");
UE_DEFINE_GAMEPLAY_TAG(TAG_ATKType_Normal, "ATKType.Normal");
UE_DEFINE_GAMEPLAY_TAG(TAG_ATKType_Crit, "ATKType.Crit");


UE_DEFINE_GAMEPLAY_TAG(TAG_Input, "Input");
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_LeftClick, "Input.LeftClick");
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_RightClick, "Input.RightClick");

UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType, "DMGType");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Normal, "DMGType.Normal");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Fire, "DMGType.Fire");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Ice, "DMGType.Ice");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Lightning, "DMGType.Lightning");

UE_DEFINE_GAMEPLAY_TAG(TAG_Ability, "Ability");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Sprint, "Ability.Sprint");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Dodge, "Ability.Dodge");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Ultimate, "Ability.Ultimate");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_BuildTower, "Ability.BuildTower");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Tower_ATK, "Ability.Tower.ATK");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_InterAction, "Ability.InterAction");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Basic, "Ability.Attack.Basic");

UE_DEFINE_GAMEPLAY_TAG(TAG_State, "State");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Idle, "State.Idle");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_ATK, "State.ATK");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Option, "State.Option");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Run, "State.Run");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Walk, "State.Walk");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Wound, "State.Wound");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Death, "State.Death");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Build, "State.Build");

UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action_Attacking, "State.Action.Attacking");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_FacingLocked, "State.Movement.FacingLocked");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Blocked, "State.Movement.Blocked");