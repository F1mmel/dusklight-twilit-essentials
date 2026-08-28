#pragma once

#include "z_item_actions.hpp"
#include "z_mobile.hpp"
#include "../sheathed_spin/sheathed_spin.hpp"
#include "m_Do/m_Do_audio.h"

DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton, CheckItemChangeFromButtonHook);
DEFINE_HOOK(&daAlink_c::checkItemButtonChange, CheckItemButtonChangeHook);
DEFINE_HOOK(&daAlink_c::checkItemSetButton, CheckItemSetButtonHook);
DEFINE_HOOK(&daAlink_c::checkSetItemTrigger, CheckSetItemTriggerHook);
DEFINE_HOOK(&daAlink_c::setHeavyBoots, SetHeavyBootsHook);
DEFINE_HOOK(&daAlink_c::orderTalk, OrderTalkHook);

bool item_needs_z_valid_button(int itemNo) {
    return itemNo == dItemNo_HVY_BOOTS_e || itemNo == dItemNo_SPINNER_e;
}

HookAction on_set_heavy_boots_pre(ModContext*, void* args, void* ret, void*) {
    // Real logic is mobile-only (no-op / HOOK_CONTINUE on desktop).
    return z_mobile_guard_heavy_boots(args, ret);
}

HookAction on_check_item_change_from_button_pre(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!g_configCustomZButtonEnabled || link == nullptr) {
        return HOOK_CONTINUE;
    }

    sync_play_select_item(2);

    BOOL result = FALSE;
    if (link->checkModeFlg(4) &&
        !link->checkEquipAnime() &&
        !link->checkBoomerangThrowAnime() &&
        !link->checkCopyRodThrowAnime() &&
        !link->checkKandelaarSwingAnime())
    {
        if (
#if PLATFORM_GCN
            dComIfGs_getSelectEquipSword() != dItemNo_NONE_e &&
#endif
            !link->checkNotBattleStage() &&
            !link->checkCanoeRide() &&
            (!link->checkModeFlg(0x40000) || link->checkEquipHeavyBoots()) &&
            link->mEquipItem != 0x103 &&
            link->swordTrigger())
        {
            if (!link->checkEndResetFlg1(daPy_py_c::ERFLG1_SWORD_TRIGGER_NON)) {
                if (g_configSheathedSpinEnabled && link->checkCutTurnInput()) {
                    link->swordEquip(TRUE);
                    link->setSwordModel();
                    mDoAud_seStart(Z2SE_AL_SWORD_PULLOUT, NULL, 0, 0);
                    if (link->checkBoardRide()) {
                        result = link->procBoardCutTurnInit();
                    } else if (link->checkReinRide()) {
                        result = link->procHorseCutTurnInit();
                    } else {
                        result = link->procCutTurnInit(1, 2);
                    }
                    *static_cast<BOOL*>(retval) = result;
                    return HOOK_SKIP_ORIGINAL;
                } else {
                    link->swordEquip(TRUE);
                }
            }
        } else if (link->checkCanoeRide() &&
                   !link->checkStageName("F_SP103") &&
                   !link->checkCanoeSlider() &&
                   !link->checkFisingRodLure() &&
                   link->swordTrigger())
        {
            link->itemEquip(0x105);
        } else {
            for (u8 i = 0; i < 3; ++i) {
                if (!link->itemTriggerCheck(1 << i)) {
                    continue;
                }

                const int procType = link->checkNewItemChange(i);
                if (procType != 0 && link->itemTriggerCheck(1 << i)) {
                    if (i == 2 && link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(i))) {
                        if (z_mobile_hb_locked(link)) {
                            continue;
                        }
                        z_mobile_hb_lock(link, link->checkEquipHeavyBoots());  // no-op on desktop
                    }
                    result = link->changeItemTriggerKeepProc(i, procType);
                    *static_cast<BOOL*>(retval) = result;
                    return HOOK_SKIP_ORIGINAL;
                }
            }

            if (link->doTrigger() && dComIfGp_getDoStatus() == BUTTON_STATUS_PUT_AWAY) {
                if (link->mEquipItem != dItemNo_KANTERA_e && link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_1)) {
                    link->offKandelaarModel();
                } else if (link->mSwordFlourishTimer != 0 && link->mEquipItem == 0x103 &&
                           !link->checkWoodSwordEquip() && !link->checkModeFlg(0x402))
                {
                    result = link->procSwordUnequipSpInit();
                } else {
                    link->allUnequip(TRUE);
                }
            } else if (link->mEquipItem == dItemNo_NONE_e &&
                       link->mThrowBoomerangAcKeep.getActor() == nullptr &&
                       !link->checkCanoeRide() &&
                       link->checkNoUpperAnime() &&
                       link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_1))
            {
                for (u8 i = 0; i < 3; ++i) {
                    if (resolved_select_item(i) == dItemNo_KANTERA_e) {
                        link->mSelectItemId = i;
                    }
                }
                link->itemEquip(dItemNo_KANTERA_e);
                link->onNoResetFlg1(daPy_py_c::FLG1_UNK_40);
            }
        }
    }

    *static_cast<BOOL*>(retval) = result;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_check_item_button_change_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!g_configCustomZButtonEnabled || link == nullptr) {
        return HOOK_CONTINUE;
    }

    sync_play_select_item(2);

    if (!link->checkCanoeRide() &&
        link->mEquipItem != dItemNo_NONE_e &&
        !link->checkEquipAnime())
    {
        for (u8 i = 0; i < 3; ++i) {
            const u8 next = (i + 1) % 3;
            if (link->mEquipItem == resolved_select_item(i) &&
                (link->mEquipItem != resolved_select_item(next) ||
                    link->mSelectItemId != next))
            {
                link->mSelectItemId = i;
            }
        }
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_check_item_set_button_pre(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const int itemNo = mods::arg<int>(args, 1);
    if (!g_configCustomZButtonEnabled || link == nullptr) {
        return HOOK_CONTINUE;
    }

    if (link->checkGroupItem(itemNo, resolved_select_item(0))) {
        *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    if (link->checkGroupItem(itemNo, resolved_select_item(1))) {
        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    if (link->checkGroupItem(itemNo, resolved_select_item(2))) {
        *static_cast<int*>(retval) = 3;
        return HOOK_SKIP_ORIGINAL;
    }

    *static_cast<int*>(retval) = 2;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_check_set_item_trigger_pre(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const int itemNo = mods::arg<int>(args, 1);
    if (!g_configCustomZButtonEnabled || link == nullptr) {
        return HOOK_CONTINUE;
    }

    for (u8 i = 0; i < 3; ++i) {
        if (!link->checkGroupItem(itemNo, resolved_select_item(i)) ||
            !link->itemTriggerCheck(1 << i))
        {
            continue;
        }

        if (itemNo == dItemNo_HVY_BOOTS_e && i == 2) {
            if (link->checkEquipHeavyBoots()) {
                // mobile: same press still being handled -> ignore
                if (z_mobile_hb_locked(link)) {
                    *static_cast<int*>(retval) = 0;
                    return HOOK_SKIP_ORIGINAL;
                }
                if (link->checkNewItemChange(2) == 1) {
                    z_mobile_hb_lock(link, true);  // no-op on desktop
                    link->changeItemTriggerKeepProc(2, 1);
                }
                *static_cast<int*>(retval) = 0;
                return HOOK_SKIP_ORIGINAL;
            }
            z_mobile_hb_lock(link, false);  // no-op on desktop
        } else {
            link->mSelectItemId = i;
        }

        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void on_order_talk_post(ModContext*, void* args, void* ret, void*) {
    if (!g_configCustomZButtonEnabled || !args || !ret) {
        return;
    }

    int* result = reinterpret_cast<int*>(ret);
    if (*result != 0) {
        return;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr || alink->checkWolf()) {
        return;
    }

    u8 zItem = resolved_select_item(2);
    if (zItem == dItemNo_NONE_e || zItem == 0x00 || zItem == 0xFF) {
        return;
    }

    dAttention_c* att = dComIfGp_getAttention();
    dAttList_c* attList2 = (att != nullptr) ? att->getActionBtnXY() : nullptr;
    fopAc_ac_c* targetActor = (attList2 != nullptr) ? attList2->getActor() : nullptr;

    if (daPy_py_c::checkTradeItem(zItem) && alink->itemTriggerCheck(0x04) && attList2 != nullptr && targetActor != nullptr) {
        if (alink->checkRequestTalkActor(attList2, targetActor)) {
            fopAcM_orderTalkItemBtnEvent(8, alink, targetActor, 0, 0);
            *result = 1;
        }
    }
}

void check_iron_boots_unequip_on_overwrite() {
    if (isTitleOrMainMenu()) return;

    daAlink_c* link = static_cast<daAlink_c*>(daPy_getPlayerActorClass());
    if (link == nullptr) return;

    if (link->checkEquipHeavyBoots()) {
        bool assigned = false;
        for (int i = 0; i < 3; i++) {
            u8 slotIdx = dComIfGs_getSelectItemIndex(i);
            if (slotIdx == SLOT_3 || slotIdx == 3) {
                assigned = true;
                break;
            }
            if (slotIdx != 0xFF && slotIdx < 24) {
                u8 item = dComIfGs_getItem(slotIdx, true);
                if (link->checkGroupItem(dItemNo_HVY_BOOTS_e, item)) {
                    assigned = true;
                    break;
                }
            }
            u8 playItem = dComIfGp_getSelectItem(i);
            if (link->checkGroupItem(dItemNo_HVY_BOOTS_e, playItem)) {
                assigned = true;
                break;
            }
            if (!isNativeZButtonEngine()) {
                if (link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(i))) {
                    assigned = true;
                    break;
                }
            }
        }

        if (!assigned) {
            if (!dComIfGp_checkPlayerStatus1(0, 0x10000) || !link->checkHookshotRoofLv7Boss()) {
                link->setHeavyBoots(0);
            }
        }
    }
}
