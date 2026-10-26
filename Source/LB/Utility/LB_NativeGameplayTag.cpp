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

UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Damage, "Data.Damage");


UE_DEFINE_GAMEPLAY_TAG(TAG_Input, "Input");
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Normal, "Input.Normal");
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Strong, "Input.Strong");

UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType, "DMGType");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Normal, "DMGType.Normal");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Fire, "DMGType.Fire");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Ice, "DMGType.Ice");
UE_DEFINE_GAMEPLAY_TAG(TAG_DMGType_Lightning, "DMGType.Lightning");

UE_DEFINE_GAMEPLAY_TAG(TAG_Ability, "Ability");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Combo, "Ability.Combo");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Sprint, "Ability.Sprint");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Dodge, "Ability.Dodge");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Ultimate, "Ability.Ultimate");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_BuildTower, "Ability.BuildTower");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Tower_ATK, "Ability.Tower.ATK");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_InterAction, "Ability.InterAction");

UE_DEFINE_GAMEPLAY_TAG(TAG_State, "State");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Idle, "State.Idle");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Invincible, "State.Invincible");
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


UE_DEFINE_GAMEPLAY_TAG(TAG_Event, "Event");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Start, "Event.Start");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_End,   "Event.End");

UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon			, "Weapon");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Dual		, "Weapon.Dual");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Dual_Left		, "Weapon.Dual.Left");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Dual_Right		, "Weapon.Dual.Right");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Twohanded , "Weapon.Twohanded");


UE_DEFINE_GAMEPLAY_TAG(TAG_Item					, "Item");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Equip			, "Item.Equip");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Consumable		, "Item.Consumable");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Currency		, "Item.Currency");

UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Grade			, "Item");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Grade_Tier1		, "Item.Grade.Tier1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Grade_Tier2		, "Item.Grade.Tier2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Grade_Tier3		, "Item.Grade.Tier3");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Grade_Tier4     , "Item.Grade.Tier4");

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Grade);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Grade_Common);    // 1단계 (일반)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Grade_Uncommon);  // 2단계 (고급)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Grade_Rare);      // 3단계 (희귀)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Grade_Epic);      // 4단계 (영웅)

UE_DEFINE_GAMEPLAY_TAG(TAG_Equip			, "Equip");
UE_DEFINE_GAMEPLAY_TAG(TAG_Equip_Weapon		, "Equip.Weapon");
UE_DEFINE_GAMEPLAY_TAG(TAG_Equip_Armor		, "Equip.Armor");


UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_None, "Combo.None");

UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L1, "Combo.Left.L1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L2, "Combo.Left.L2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L3, "Combo.Left.L3");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L4, "Combo.Left.L4");

UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L1R1, "Combo.Left.L1R1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L1R2, "Combo.Left.L1R2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L1R3, "Combo.Left.L1R3");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L1R4, "Combo.Left.L1R4");

UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L2R1, "Combo.Left.L2R1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L2R2, "Combo.Left.L2R2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Left_L3R1, "Combo.Left.L3R1");

UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Right_R1, "Combo.Right.R1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Right_R2, "Combo.Right.R2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Combo_Right_R3, "Combo.Right.R3");