#include "StudyGameplayTags.h"

namespace StudyTags
{
    UE_DEFINE_GAMEPLAY_TAG(State_Idle,        "State.Idle");
    UE_DEFINE_GAMEPLAY_TAG(State_Moving,      "State.Moving");
    UE_DEFINE_GAMEPLAY_TAG(State_Attacking,   "State.Attacking");
    UE_DEFINE_GAMEPLAY_TAG(State_Dodging,     "State.Dodging");
    UE_DEFINE_GAMEPLAY_TAG(State_HitReact,    "State.HitReact");
    UE_DEFINE_GAMEPLAY_TAG(State_AirBorne,    "State.AirBorne");
    UE_DEFINE_GAMEPLAY_TAG(State_Finisher,    "State.Finisher");
    UE_DEFINE_GAMEPLAY_TAG(State_Dead,        "State.Dead");

    UE_DEFINE_GAMEPLAY_TAG(Ability_Combo,       "Ability.Combo");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge,       "Ability.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(Ability_JustCounter, "Ability.JustCounter");
    UE_DEFINE_GAMEPLAY_TAG(Ability_AirLauncher, "Ability.AirLauncher");
    UE_DEFINE_GAMEPLAY_TAG(Ability_AirCombo,    "Ability.AirCombo");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Finisher,    "Ability.Finisher");
    UE_DEFINE_GAMEPLAY_TAG(Ability_LockOn,      "Ability.LockOn");

    UE_DEFINE_GAMEPLAY_TAG(Effect_HitStop,   "Effect.HitStop");
    UE_DEFINE_GAMEPLAY_TAG(Effect_Knockback, "Effect.Knockback");

    UE_DEFINE_GAMEPLAY_TAG(Status_Invincible, "Status.Invincible");
    UE_DEFINE_GAMEPLAY_TAG(Status_SuperArmor, "Status.SuperArmor");
    UE_DEFINE_GAMEPLAY_TAG(Status_CanFinish,  "Status.CanFinish");

    UE_DEFINE_GAMEPLAY_TAG(Event_HitReact,        "Event.HitReact");
    UE_DEFINE_GAMEPLAY_TAG(Event_Death,           "Event.Death");
    UE_DEFINE_GAMEPLAY_TAG(Event_ComboWindowOpen, "Event.ComboWindowOpen");
    UE_DEFINE_GAMEPLAY_TAG(Event_Melee_Hit,       "Event.Melee.Hit");
    UE_DEFINE_GAMEPLAY_TAG(Event_Melee_HitStart,  "Event.Melee.HitStart");
    UE_DEFINE_GAMEPLAY_TAG(Event_Melee_HitEnd,    "Event.Melee.HitEnd");
    UE_DEFINE_GAMEPLAY_TAG(Event_Launched,        "Event.Launched");
    UE_DEFINE_GAMEPLAY_TAG(Event_Slammed,         "Event.Slammed");
    UE_DEFINE_GAMEPLAY_TAG(Event_Executed,        "Event.Executed");

    UE_DEFINE_GAMEPLAY_TAG(Input_Attack,    "Input.Attack");
    UE_DEFINE_GAMEPLAY_TAG(Input_AirAttack, "Input.AirAttack");
    UE_DEFINE_GAMEPLAY_TAG(Input_Launcher,  "Input.Launcher");
    UE_DEFINE_GAMEPLAY_TAG(Input_Finisher,  "Input.Finisher");
    UE_DEFINE_GAMEPLAY_TAG(Input_Dodge,     "Input.Dodge");

    UE_DEFINE_GAMEPLAY_TAG(Data_Damage,  "Data.Damage");
    UE_DEFINE_GAMEPLAY_TAG(Data_LaunchZ, "Data.LaunchZ");
}
