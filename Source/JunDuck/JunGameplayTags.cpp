


#include "JunGameplayTags.h"

namespace JunGameplayTags
{
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Move, "Input.Action.Move");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Turn, "Input.Action.Turn");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Jump, "Input.Action.Jump");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_BasicAttack, "Input.Action.BasicAttack");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_HeavyAttack, "Input.Action.HeavyAttack");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Dodge, "Input.Action.Dodge");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_LockOn, "Input.Action.LockOn");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Run, "Input.Action.Run");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_WalkToggle, "Input.Action.WalkToggle");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Defense, "Input.Action.Defense");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Drink, "Input.Action.Drink");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_ControlsToggle, "Input.Action.ControlsToggle");
UE_DEFINE_GAMEPLAY_TAG(Input_Action_Interact, "Input.Action.Interact");

UE_DEFINE_GAMEPLAY_TAG(Event_Notify_BasicAttack_ComboWindow, "Event.Notify.BasicAttack.ComboWindow");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_BasicAttack_ComboAdvance, "Event.Notify.BasicAttack.ComboAdvance");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_HeavyAttack_ComboWindow, "Event.Notify.HeavyAttack.ComboWindow");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Jigen_ComboWindow, "Event.Notify.Jigen.ComboWindow");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_BasicAttack_DefenseCancelOpen, "Event.Notify.BasicAttack.DefenseCancelOpen");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_BasicAttack_RecoveryOpen, "Event.Notify.BasicAttack.RecoveryOpen");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Dodge_Start, "Event.Notify.Dodge.Start");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Defense_ParryWindowBegin, "Event.Notify.Defense.ParryWindowBegin");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Defense_ParryWindowEnd, "Event.Notify.Defense.ParryWindowEnd");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Defense_GuardReady, "Event.Notify.Defense.GuardReady");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Defense_EndBaseRelease, "Event.Notify.Defense.EndBaseRelease");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Defense_GuardHoldBegin, "Event.Notify.Defense.GuardHoldBegin");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Execution_Apply, "Event.Notify.Execution.Apply");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Execution_FinishCameraPullIn, "Event.Notify.Execution.FinishCameraPullIn");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Execution_CameraRestore, "Event.Notify.Execution.CameraRestore");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Death_Present, "Event.Notify.Death.Present");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Boss_Clear, "Event.Notify.Boss.Clear");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Boss_StopCombatBGM, "Event.Notify.Boss.StopCombatBGM");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Boss_DangerAttack, "Event.Notify.Boss.DangerAttack");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Wukong_ParryCounter, "Event.Notify.Wukong.ParryCounter");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Wukong_ParryCounterFollowUpDecision, "Event.Notify.Wukong.ParryCounterFollowUpDecision");
UE_DEFINE_GAMEPLAY_TAG(Event_Notify_Wukong_BowAttackRestoreWeapon, "Event.Notify.Wukong.BowAttackRestoreWeapon");

UE_DEFINE_GAMEPLAY_TAG(State_Action_Attack, "State.Action.Attack");
UE_DEFINE_GAMEPLAY_TAG(State_Action_Dodge, "State.Action.Dodge");
  
UE_DEFINE_GAMEPLAY_TAG(State_Condition_Dead, "State.Condition.Dead");
UE_DEFINE_GAMEPLAY_TAG(State_Condition_Invincible, "State.Condition.Invincible");
UE_DEFINE_GAMEPLAY_TAG(State_Condition_HitReact, "State.Condition.HitReact");
UE_DEFINE_GAMEPLAY_TAG(State_Condition_ControlLocked, "State.Condition.ControlLocked");
UE_DEFINE_GAMEPLAY_TAG(State_Condition_SuperArmor, "State.Condition.SuperArmor");
UE_DEFINE_GAMEPLAY_TAG(State_Condition_PostHitArmor, "State.Condition.PostHitArmor");
 
UE_DEFINE_GAMEPLAY_TAG(State_Block_Move, "State.Block.Move");
UE_DEFINE_GAMEPLAY_TAG(State_Block_Jump, "State.Block.Jump");
UE_DEFINE_GAMEPLAY_TAG(State_Block_Attack, "State.Block.Attack");
UE_DEFINE_GAMEPLAY_TAG(State_Block_Dodge, "State.Block.Dodge");
UE_DEFINE_GAMEPLAY_TAG(State_Block_Input, "State.Block.Input");

UE_DEFINE_GAMEPLAY_TAG(Team_Player, "Team.Player");
UE_DEFINE_GAMEPLAY_TAG(Team_Enemy, "Team.Enemy");
UE_DEFINE_GAMEPLAY_TAG(Team_Neutral, "Team.Neutral");


}
