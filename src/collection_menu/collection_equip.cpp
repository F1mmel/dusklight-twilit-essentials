#include "collection_equip.hpp"
#include "collection_layout.hpp"

static bool s_inAlinkCreate = false;

HookAction on_wait_proc_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D || !collect2D->mpScreen) return HOOK_CONTINUE;

    apply_collect_shifts(collect2D);

    if (dMw_A_TRIGGER()) {
        u8 curX = collect2D->mCursorX;
        u8 curY = collect2D->mCursorY;

        if (curY == 0 && curX >= 3 && curX <= 5) {
            if (daPy_getPlayerActorClass()->getSwordChangeWaitTimer() == 0) {
                collect2D->changeSword();
                return HOOK_SKIP_ORIGINAL;
            }
        } else if (curY == 1 && curX >= 3 && curX <= 5) {
            if (daPy_getPlayerActorClass()->getShieldChangeWaitTimer() == 0) {
                collect2D->changeShield();
                return HOOK_SKIP_ORIGINAL;
            }
        } else if (curY == 2 && curX >= 3 && curX <= 6) {
            if (daPy_getPlayerActorClass()->getClothesChangeWaitTimer() == 0) {
                collect2D->changeClothe();
                return HOOK_SKIP_ORIGINAL;
            }
        }
    }
    return HOOK_CONTINUE;
}



void on_wait_proc_post(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    if (curX >= 3 && curX <= 6 && curY <= 2) {
        if (collect2D->mIsWolf || !is_collect_item_unlocked(curX, curY)) {
            collect2D->setAButtonString(0);
        } else if (is_collect_item_equipped(curX, curY)) {
            if (g_configCollectionUnequip && (curY == 0 || curY == 1)) {
                collect2D->setAButtonString(0x437); // "Unequip"
            } else {
                collect2D->setAButtonString(0);
            }
        } else {
            collect2D->setAButtonString(0x436); // "Equip"
        }
    }
}

HookAction on_pointer_activate_current_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return HOOK_CONTINUE;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    if (curY == 0 && curX >= 3 && curX <= 5) {
        if (daPy_getPlayerActorClass()->getSwordChangeWaitTimer() == 0) {
            collect2D->changeSword();
            return HOOK_SKIP_ORIGINAL;
        }
    } else if (curY == 1 && curX >= 3 && curX <= 5) {
        if (daPy_getPlayerActorClass()->getShieldChangeWaitTimer() == 0) {
            collect2D->changeShield();
            return HOOK_SKIP_ORIGINAL;
        }
    } else if (curY == 2 && curX >= 3 && curX <= 6) {
        if (daPy_getPlayerActorClass()->getClothesChangeWaitTimer() == 0) {
            collect2D->changeClothe();
            return HOOK_SKIP_ORIGINAL;
        }
    }
    return HOOK_CONTINUE;
}

HookAction on_change_sword_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return HOOK_CONTINUE;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    if (curX == 3) {
        // Wooden sword is always unlocked
        if (dComIfGs_getSelectEquipSword() == dItemNo_WOOD_STICK_e) {
            if (g_configCollectionUnequip) {
                dMeter2Info_setSword(dItemNo_NONE_e, false);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        } else {
            dMeter2Info_setSword(dItemNo_WOOD_STICK_e, false);
            dComIfGs_onItemFirstBit(dItemNo_WOOD_STICK_e);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
            update_frame_highlights(collect2D);
        }
    } else if (curX == 4) {
        if (dComIfGs_isItemFirstBit(dItemNo_SWORD_e)) {
            if (dComIfGs_getSelectEquipSword() == dItemNo_SWORD_e) {
                if (g_configCollectionUnequip) {
                    dMeter2Info_setSword(dItemNo_NONE_e, false);
                    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                    dMeter2Info_set2DVibration();
                    update_frame_highlights(collect2D);
                }
            } else {
                dMeter2Info_setSword(dItemNo_SWORD_e, false);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    } else if (curX == 5) {
        if (dComIfGs_isItemFirstBit(dItemNo_MASTER_SWORD_e) || dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e)) {
            u8 targetSword = dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e) ? dItemNo_LIGHT_SWORD_e : dItemNo_MASTER_SWORD_e;
            if (dComIfGs_getSelectEquipSword() == targetSword || dComIfGs_getSelectEquipSword() == dItemNo_MASTER_SWORD_e || dComIfGs_getSelectEquipSword() == dItemNo_LIGHT_SWORD_e) {
                if (g_configCollectionUnequip) {
                    dMeter2Info_setSword(dItemNo_NONE_e, false);
                    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                    dMeter2Info_set2DVibration();
                    update_frame_highlights(collect2D);
                }
            } else {
                dMeter2Info_setSword(targetSword, false);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_change_shield_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return HOOK_CONTINUE;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    if (curX == 3) {
        if (dComIfGs_isItemFirstBit(dItemNo_WOOD_SHIELD_e)) {
            if (dComIfGs_getSelectEquipShield() == dItemNo_WOOD_SHIELD_e) {
                if (g_configCollectionUnequip) {
                    dMeter2Info_setShield(dItemNo_NONE_e, false);
                    daAlink_getAlinkActorClass()->setShieldChange();
                    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                    dMeter2Info_set2DVibration();
                    update_frame_highlights(collect2D);
                }
            } else {
                dMeter2Info_setShield(dItemNo_WOOD_SHIELD_e, false);
                daAlink_getAlinkActorClass()->setShieldChange();
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    } else if (curX == 4) {
        if (dComIfGs_isItemFirstBit(dItemNo_SHIELD_e)) {
            if (dComIfGs_getSelectEquipShield() == dItemNo_SHIELD_e) {
                if (g_configCollectionUnequip) {
                    dMeter2Info_setShield(dItemNo_NONE_e, false);
                    daAlink_getAlinkActorClass()->setShieldChange();
                    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                    dMeter2Info_set2DVibration();
                    update_frame_highlights(collect2D);
                }
            } else {
                dMeter2Info_setShield(dItemNo_SHIELD_e, false);
                daAlink_getAlinkActorClass()->setShieldChange();
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    } else if (curX == 5) {
        if (dComIfGs_isItemFirstBit(dItemNo_HYLIA_SHIELD_e)) {
            if (dComIfGs_getSelectEquipShield() == dItemNo_HYLIA_SHIELD_e) {
                if (g_configCollectionUnequip) {
                    dMeter2Info_setShield(dItemNo_NONE_e, false);
                    daAlink_getAlinkActorClass()->setShieldChange();
                    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                    dMeter2Info_set2DVibration();
                    update_frame_highlights(collect2D);
                }
            } else {
                dMeter2Info_setShield(dItemNo_HYLIA_SHIELD_e, false);
                daAlink_getAlinkActorClass()->setShieldChange();
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_change_clothes_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return HOOK_CONTINUE;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    if (curX == 3) {
        // Ordon Clothes always unlocked
        if (dComIfGs_getSelectEquipClothes() != dItemNo_WEAR_CASUAL_e) {
            dMeter2Info_setCloth(dItemNo_WEAR_CASUAL_e, false);
            daPy_getPlayerActorClass()->setClothesChange(0);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
            update_frame_highlights(collect2D);
        }
    } else if (curX == 4) {
        if (dComIfGs_isItemFirstBit(dItemNo_WEAR_KOKIRI_e)) {
            if (dComIfGs_getSelectEquipClothes() != dItemNo_WEAR_KOKIRI_e) {
                dMeter2Info_setCloth(dItemNo_WEAR_KOKIRI_e, false);
                daPy_getPlayerActorClass()->setClothesChange(0);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    } else if (curX == 5) {
        if (dComIfGs_isItemFirstBit(dItemNo_WEAR_ZORA_e)) {
            if (dComIfGs_getSelectEquipClothes() != dItemNo_WEAR_ZORA_e) {
                dMeter2Info_setCloth(dItemNo_WEAR_ZORA_e, false);
                daPy_getPlayerActorClass()->setClothesChange(0);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    } else if (curX == 6) {
        if (dComIfGs_isItemFirstBit(dItemNo_ARMOR_e)) {
            if (dComIfGs_getSelectEquipClothes() != dItemNo_ARMOR_e) {
                dMeter2Info_setCloth(dItemNo_ARMOR_e, false);
                daPy_getPlayerActorClass()->setClothesChange(0);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                dMeter2Info_set2DVibration();
                update_frame_highlights(collect2D);
            }
        }
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_equip_frame_sword_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    update_frame_highlights(collect2D);
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_equip_frame_shield_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    update_frame_highlights(collect2D);
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_equip_frame_clothes_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    update_frame_highlights(collect2D);
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_da_alink_create_pre(ModContext*, void*, void*, void*) {
    s_inAlinkCreate = true;
    return HOOK_CONTINUE;
}

void on_da_alink_create_post(ModContext*, void*, void*, void*) {
    s_inAlinkCreate = false;
}

HookAction on_set_select_equip_clothes_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    u8 newCloth = mods::arg<u8>(args, 0);
    if (s_inAlinkCreate && newCloth == dItemNo_WEAR_KOKIRI_e && dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_CASUAL_e) {
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
