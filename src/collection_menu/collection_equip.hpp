#pragma once

#include "collection_common.hpp"

// Equipment hooks
DEFINE_HOOK(&dMenu_Collect2D_c::wait_proc, WaitProcHook);
HookAction on_wait_proc_pre(ModContext*, void* args, void*, void*);
void on_wait_proc_post(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::pointerActivateCurrent, PointerActivateCurrentHook);
HookAction on_pointer_activate_current_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::changeSword, ChangeSwordHook);
HookAction on_change_sword_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::changeShield, ChangeShieldHook);
HookAction on_change_shield_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::changeClothe, ChangeClotheHook);
HookAction on_change_clothes_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::setEquipItemFrameColorSword, SetEquipFrameColorSwordHook);
HookAction on_set_equip_frame_sword_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::setEquipItemFrameColorShield, SetEquipFrameColorShieldHook);
HookAction on_set_equip_frame_shield_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::setEquipItemFrameColorClothes, SetEquipFrameColorClothesHook);
HookAction on_set_equip_frame_clothes_pre(ModContext*, void* args, void*, void*);

// Area transition & spawn clothes preservation hooks
DEFINE_HOOK(&daAlink_c::create, DaAlinkCreateHook);
HookAction on_da_alink_create_pre(ModContext*, void*, void*, void*);
void on_da_alink_create_post(ModContext*, void*, void*, void*);

DEFINE_HOOK(&dComIfGs_setSelectEquipClothes, SetSelectEquipClothesHook);
HookAction on_set_select_equip_clothes_pre(ModContext*, void* args, void*, void*);

// Shield preservation hook for keep Ordon Shield
DEFINE_HOOK(&dMeter2Info_setShield, Meter2InfoSetShieldHook);
HookAction on_meter2_info_set_shield_pre(ModContext*, void* args, void*, void*);
