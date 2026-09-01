#pragma once

#include "z_itemwheel.hpp"
#include <d/d_menu_item_explain.h>
#include <JSystem/JUtility/JUTFont.h>
#include <cmath>

DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SetActiveCursorHook);
DEFINE_HOOK(&dSv_player_status_a_c::setSelectItemIndex, SetSelectItemIndexHook);
DEFINE_HOOK(&dMeter2_c::checkStatus, CheckStatusHook);
DEFINE_HOOK(&dMenu_Ring_c::setMixItem, SetMixItemHook);
DEFINE_HOOK(&dMenu_Ring_c::setItem, SetItemHook);
DEFINE_HOOK(&dMenu_Ring_c::setJumpItem, SetJumpItemHook);
DEFINE_HOOK(&dMenu_Ring_c::setSelectItem, SetSelectItemHook);
DEFINE_HOOK(&dMenu_Ring_c::setSelectItemForce, SetSelectItemForceHook);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOn, IsMixItemOnHook);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOff, IsMixItemOffHook);
DEFINE_HOOK(&dMenu_Ring_c::checkExplainForce, CheckExplainForceHook);
DEFINE_HOOK(&dMenu_Ring_c::_create, RingCreateHook);
DEFINE_HOOK(&dMenu_Ring_c::_delete, RingDeleteHook);
DEFINE_HOOK(&dMenu_Ring_c::_draw, RingDrawHook);

u8 get_ring_slot_for_item(dMenu_Ring_c* ring, u8 slotOrItem) {
    if (!ring || slotOrItem == 0xFF || slotOrItem == dItemNo_NONE_e) return 0xFF;
    u8 targetItem = (slotOrItem < 24) ? dComIfGs_getItem(slotOrItem, false) : slotOrItem;
    if (targetItem == 0xFF || targetItem == 0x00 || targetItem == dItemNo_NONE_e) return 0xFF;

    u8 total = ring->mItemsTotal;
    u8* slots = ring->mItemSlots;
    if (!slots) return 0xFF;
    for (int i = 0; i < total; i++) {
        if (slots[i] == targetItem || (slots[i] < 24 && dComIfGs_getItem(slots[i], false) == targetItem)) {
            return (u8)i;
        }
    }
    return 0xFF;
}

void trigger_ring_item_slide_z(dMenu_Ring_c* ring, u8 itemNo) {
    if (!ring) return;

    ring->setSelectItem(2, itemNo);
    ring->field_0x674[2] = 1;
#if TARGET_PC
    ring->mSelectItemSlideElapsed[2] = 0.0f;
#endif
    ring->field_0x538[0] = g_ringHIO.mUnselectItemScale;
    ring->field_0x538[1] = g_ringHIO.mUnselectItemScale;
    ring->field_0x538[2] = g_ringHIO.mSelectItemScale;
}

void commit_pending_z_slot(dMenu_Ring_c* ring) {
    if (g_zPendingZSlot != 0xFF) {
        g_zInventorySlot = g_zPendingZSlot;
        g_zPendingZSlot = 0xFF;
        if (ring != nullptr) {
            update_ring_z_slots(ring);
        }
    }
}

void update_ring_z_slots(dMenu_Ring_c* ring) {
    if (!ring) return;

    if (ring->field_0x674[0] != 0 || ring->field_0x674[1] != 0 || ring->field_0x674[2] != 0) {
        return;
    }

    u8 zSlot = dComIfGs_getSelectItemIndex(2);
    if (zSlot == 0xFF || zSlot >= 24) {
        zSlot = g_zInventorySlot;
    }
    if (zSlot != 0xFF && zSlot < 24) {
        u8 itm = dComIfGs_getItem(zSlot, false);
        if (itm != 0xFF && itm != 0x00 && itm != dItemNo_NONE_e) {
            ring->field_0x6ac = get_ring_slot_for_item(ring, zSlot);
            ring->field_0x6b4[2] = zSlot;
            ring->field_0x6b8[2] = dComIfGs_getMixItemIndex(2);
            return;
        }
    }
    ring->field_0x6ac = 0xFF;
    ring->field_0x6b4[2] = 0xFF;
    ring->field_0x6b8[2] = 0xFF;
}

HookAction on_set_active_cursor_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring) return HOOK_CONTINUE;
    g_activeRing = ring;
    update_ring_z_slots(ring);

    return HOOK_CONTINUE;
}

void on_set_active_cursor_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring) {
        return;
    }

    g_activeRing = ring;
    g_meter2_info.onUseButton(0x800);
    commit_pending_z_slot(ring);

    JUTGamePad* rawGamePad = JUTGamePad::getGamePad(0);
    bool physZTrig = rawGamePad != nullptr && (rawGamePad->getTrigger() & PAD_TRIGGER_Z) != 0;
    if (!physZTrig) {
        return;
    }

    daPy_py_c* alink = daPy_getLinkPlayerActorClass();
    if (alink && alink->checkWolf()) {
        return;
    }

    u8 hoveredSlot = ring->mItemSlots[ring->mCurrentSlot];
    if (hoveredSlot == 0xFF || hoveredSlot == dItemNo_NONE_e) {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }

    u8 hoveredItem = dComIfGs_getItem(hoveredSlot, false);
    if (hoveredItem == dItemNo_NONE_e || hoveredItem == 0x00 || hoveredItem == 0xFF) {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }

    for (int i = 0; i < 4; i++) {
        ring->setSelectItemForce(i);
    }
    ring->field_0x6b3 = 2;
    if (!ring->checkCombineBomb(ring->field_0x6b3)) {
        ring->setItem();
        if (ring->mpItemExplain && ring->mpItemExplain->getStatus() == 0) {
            ring->setStatus(dMenu_Ring_c::STATUS_WAIT);
        }
    }
}

HookAction on_set_item_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring) return HOOK_CONTINUE;

    u8 uVar1 = dComIfGs_getSelectItemIndex(0);
    u8 uVar2 = dComIfGs_getSelectItemIndex(1);
    u8 uVar3 = dComIfGs_getSelectItemIndex(2);
    u8 uVar4 = dComIfGs_getSelectItemIndex(3);

    u8 mixItemIndex0 = dComIfGs_getMixItemIndex(0);
    u8 mixItemIndex1 = dComIfGs_getMixItemIndex(1);
    u8 mixItemIndex2 = dComIfGs_getMixItemIndex(2);

    for (int i = 0; i < 4; i++) {
        ring->setSelectItemForce(i);
    }

    if (ring->field_0x6b3 == 0) {
        if (ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(1)) {
            u8 temp = dComIfGs_getSelectItemIndex(0);
            uVar2 = temp;
            mixItemIndex1 = dComIfGs_getMixItemIndex(0);
            if (temp == dItemNo_NONE_e) {
                ring->mYButtonSlot = dItemNo_NONE_e;
            } else {
                ring->mYButtonSlot = ring->mXButtonSlot;
            }
            ring->mXButtonSlot = ring->mCurrentSlot;
            uVar1 = ring->mItemSlots[ring->mXButtonSlot];
            mixItemIndex0 = dItemNo_NONE_e;
        }
        else if (ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(2)) {
            u8 temp = dComIfGs_getSelectItemIndex(0);
            uVar3 = temp;
            mixItemIndex2 = dComIfGs_getMixItemIndex(0);
            if (temp == dItemNo_NONE_e) {
                ring->field_0x6ac = dItemNo_NONE_e;
            } else {
                ring->field_0x6ac = ring->mXButtonSlot;
            }
            ring->mXButtonSlot = ring->mCurrentSlot;
            uVar1 = ring->mItemSlots[ring->mXButtonSlot];
            mixItemIndex0 = dItemNo_NONE_e;
        }
        else {
            if (dComIfGs_getMixItemIndex(1) == ring->mItemSlots[ring->mCurrentSlot]) {
                uVar2 = dComIfGs_getSelectItemIndex(0);
                mixItemIndex1 = dItemNo_NONE_e;
                if (uVar2 == dItemNo_NONE_e) {
                    ring->mYButtonSlot = dItemNo_NONE_e;
                } else {
                    ring->mYButtonSlot = ring->mXButtonSlot;
                }
                ring->mXButtonSlot = ring->mCurrentSlot;
                uVar1 = ring->mItemSlots[ring->mXButtonSlot];
                mixItemIndex0 = dItemNo_NONE_e;
            }
            else if (dComIfGs_getMixItemIndex(2) == ring->mItemSlots[ring->mCurrentSlot]) {
                uVar3 = dComIfGs_getSelectItemIndex(0);
                mixItemIndex2 = dItemNo_NONE_e;
                if (uVar3 == dItemNo_NONE_e) {
                    ring->field_0x6ac = dItemNo_NONE_e;
                } else {
                    ring->field_0x6ac = ring->mXButtonSlot;
                }
                ring->mXButtonSlot = ring->mCurrentSlot;
                uVar1 = ring->mItemSlots[ring->mXButtonSlot];
                mixItemIndex0 = dItemNo_NONE_e;
            }
            else {
                ring->mXButtonSlot = ring->mCurrentSlot;
                uVar1 = ring->mItemSlots[ring->mXButtonSlot];
                mixItemIndex0 = dItemNo_NONE_e;
            }
        }
    } else if (ring->field_0x6b3 == 1) {
        if (ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(0)) {
            u8 temp = dComIfGs_getSelectItemIndex(1);
            uVar1 = temp;
            mixItemIndex0 = dComIfGs_getMixItemIndex(1);
            if (temp == dItemNo_NONE_e) {
                ring->mXButtonSlot = dItemNo_NONE_e;
            } else {
                ring->mXButtonSlot = ring->mYButtonSlot;
            }
            ring->mYButtonSlot = ring->mCurrentSlot;
            uVar2 = ring->mItemSlots[ring->mYButtonSlot];
            mixItemIndex1 = dItemNo_NONE_e;
        }
        else if (ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(2)) {
            u8 temp = dComIfGs_getSelectItemIndex(1);
            uVar3 = temp;
            mixItemIndex2 = dComIfGs_getMixItemIndex(1);
            if (temp == dItemNo_NONE_e) {
                ring->field_0x6ac = dItemNo_NONE_e;
            } else {
                ring->field_0x6ac = ring->mYButtonSlot;
            }
            ring->mYButtonSlot = ring->mCurrentSlot;
            uVar2 = ring->mItemSlots[ring->mYButtonSlot];
            mixItemIndex1 = dItemNo_NONE_e;
        }
        else {
            if (dComIfGs_getMixItemIndex(0) == ring->mItemSlots[ring->mCurrentSlot]) {
                uVar1 = dComIfGs_getSelectItemIndex(1);
                mixItemIndex0 = dItemNo_NONE_e;
                if (uVar1 == dItemNo_NONE_e) {
                    ring->mXButtonSlot = dItemNo_NONE_e;
                } else {
                    ring->mXButtonSlot = ring->mYButtonSlot;
                }
                ring->mYButtonSlot = ring->mCurrentSlot;
                uVar2 = ring->mItemSlots[ring->mYButtonSlot];
                mixItemIndex1 = dItemNo_NONE_e;
            }
            else if (dComIfGs_getMixItemIndex(2) == ring->mItemSlots[ring->mCurrentSlot]) {
                uVar3 = dComIfGs_getSelectItemIndex(1);
                mixItemIndex2 = dItemNo_NONE_e;
                if (uVar3 == dItemNo_NONE_e) {
                    ring->field_0x6ac = dItemNo_NONE_e;
                } else {
                    ring->field_0x6ac = ring->mYButtonSlot;
                }
                ring->mYButtonSlot = ring->mCurrentSlot;
                uVar2 = ring->mItemSlots[ring->mYButtonSlot];
                mixItemIndex1 = dItemNo_NONE_e;
            } else {
                ring->mYButtonSlot = ring->mCurrentSlot;
                uVar2 = ring->mItemSlots[ring->mYButtonSlot];
                mixItemIndex1 = dItemNo_NONE_e;
            }
        }
    } else if (ring->field_0x6b3 == 2) {
        if (ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(0)) {
            u8 temp = dComIfGs_getSelectItemIndex(2);
            uVar1 = temp;
            mixItemIndex0 = dComIfGs_getMixItemIndex(2);
            if (temp == dItemNo_NONE_e) {
                ring->mXButtonSlot = dItemNo_NONE_e;
            } else {
                ring->mXButtonSlot = ring->field_0x6ac;
            }
            ring->field_0x6ac = ring->mCurrentSlot;
            uVar3 = ring->mItemSlots[ring->field_0x6ac];
            mixItemIndex2 = dItemNo_NONE_e;
        }
        else if (ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(1)) {
            u8 temp = dComIfGs_getSelectItemIndex(2);
            uVar2 = temp;
            mixItemIndex1 = dComIfGs_getMixItemIndex(2);
            if (temp == dItemNo_NONE_e) {
                ring->mYButtonSlot = dItemNo_NONE_e;
            } else {
                ring->mYButtonSlot = ring->field_0x6ac;
            }
            ring->field_0x6ac = ring->mCurrentSlot;
            uVar3 = ring->mItemSlots[ring->field_0x6ac];
            mixItemIndex2 = dItemNo_NONE_e;
        }
        else {
            if (dComIfGs_getMixItemIndex(0) == ring->mItemSlots[ring->mCurrentSlot]) {
                uVar1 = dComIfGs_getSelectItemIndex(2);
                mixItemIndex0 = dItemNo_NONE_e;
                if (uVar1 == dItemNo_NONE_e) {
                    ring->mXButtonSlot = dItemNo_NONE_e;
                } else {
                    ring->mXButtonSlot = ring->field_0x6ac;
                }
                ring->field_0x6ac = ring->mCurrentSlot;
                uVar3 = ring->mItemSlots[ring->field_0x6ac];
                mixItemIndex2 = dItemNo_NONE_e;
            }
            else if (dComIfGs_getMixItemIndex(1) == ring->mItemSlots[ring->mCurrentSlot]) {
                uVar2 = dComIfGs_getSelectItemIndex(2);
                mixItemIndex1 = dItemNo_NONE_e;
                if (uVar2 == dItemNo_NONE_e) {
                    ring->mYButtonSlot = dItemNo_NONE_e;
                } else {
                    ring->mYButtonSlot = ring->field_0x6ac;
                }
                ring->field_0x6ac = ring->mCurrentSlot;
                uVar3 = ring->mItemSlots[ring->field_0x6ac];
                mixItemIndex2 = dItemNo_NONE_e;
            } else {
                ring->field_0x6ac = ring->mCurrentSlot;
                uVar3 = ring->mItemSlots[ring->field_0x6ac];
                mixItemIndex2 = dItemNo_NONE_e;
            }
        }
    }

    ring->field_0x6b4[0] = uVar1;
    ring->field_0x6b4[1] = uVar2;
    ring->field_0x6b4[2] = uVar3;
    ring->field_0x6b4[3] = uVar4;
    ring->field_0x6b8[0] = mixItemIndex0;
    ring->field_0x6b8[1] = mixItemIndex1;
    ring->field_0x6b8[2] = mixItemIndex2;
    ring->field_0x6b8[3] = dItemNo_NONE_e;
    ring->field_0x6cd = dItemNo_NONE_e;

    ring->setJumpItem(true);

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_jump_item_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    bool i_useVibrationM = mods::arg<bool>(args, 1);
    if (!ring) return HOOK_CONTINUE;

    for (int i = 0; i < 4; i++) {
        if (i == 3) {
            ring->setSelectItem(i, ring->field_0x6b4[i]);
        } else if (i == ring->field_0x6cd) {
            ring->setSelectItem(i, ring->getItem(ring->field_0x6cb, 0));
        } else {
            ring->setSelectItem(i, ring->getItem(ring->field_0x6b4[i], ring->field_0x6b8[i]));
        }
    }

    if (ring->mXButtonSlot != dItemNo_NONE_e) {
        ring->field_0x518[0] = ring->mItemSlotPosX[ring->mXButtonSlot];
        ring->field_0x528[0] = ring->mItemSlotPosY[ring->mXButtonSlot];
    }
    if (ring->mYButtonSlot != dItemNo_NONE_e) {
        ring->field_0x518[1] = ring->mItemSlotPosX[ring->mYButtonSlot];
        ring->field_0x528[1] = ring->mItemSlotPosY[ring->mYButtonSlot];
    }
    if (ring->field_0x6ac != dItemNo_NONE_e) {
        ring->field_0x518[2] = ring->mItemSlotPosX[ring->field_0x6ac];
        ring->field_0x528[2] = ring->mItemSlotPosY[ring->field_0x6ac];
    }
    if (ring->field_0x6ad != dItemNo_NONE_e) {
        ring->field_0x518[3] = ring->mItemSlotPosX[ring->field_0x6ad];
        ring->field_0x528[3] = ring->mItemSlotPosY[ring->field_0x6ad];
    }

    if (ring->field_0x6b3 == 0) {
        ring->field_0x538[0] = g_ringHIO.mSelectItemScale;
        ring->field_0x538[1] = g_ringHIO.mUnselectItemScale;
        ring->field_0x538[2] = g_ringHIO.mUnselectItemScale;
        if (ring->field_0x6b4[0] != dComIfGs_getSelectItemIndex(0) ||
            ring->field_0x6b8[0] != dComIfGs_getMixItemIndex(0))
        {
            ring->field_0x674[0] = 1;
#if TARGET_PC
            ring->mSelectItemSlideElapsed[0] = 0.0f;
#endif
        }
    } else if (ring->field_0x6b3 == 1) {
        ring->field_0x538[0] = g_ringHIO.mUnselectItemScale;
        ring->field_0x538[1] = g_ringHIO.mSelectItemScale;
        ring->field_0x538[2] = g_ringHIO.mUnselectItemScale;
        if (ring->field_0x6b4[1] != dComIfGs_getSelectItemIndex(1) ||
            ring->field_0x6b8[1] != dComIfGs_getMixItemIndex(1))
        {
            ring->field_0x674[1] = 1;
#if TARGET_PC
            ring->mSelectItemSlideElapsed[1] = 0.0f;
#endif
        }
    } else if (ring->field_0x6b3 == 2) {
        ring->field_0x538[0] = g_ringHIO.mUnselectItemScale;
        ring->field_0x538[1] = g_ringHIO.mUnselectItemScale;
        ring->field_0x538[2] = g_ringHIO.mSelectItemScale;
        if (ring->field_0x6b4[2] != dComIfGs_getSelectItemIndex(2) ||
            ring->field_0x6b8[2] != dComIfGs_getMixItemIndex(2))
        {
            ring->field_0x674[2] = 1;
#if TARGET_PC
            ring->mSelectItemSlideElapsed[2] = 0.0f;
#endif
        }
    }

    if (ring->field_0x674[0] == 1 || ring->field_0x674[1] == 1 || ring->field_0x674[2] == 1) {
        if (i_useVibrationM) {
            dMeter2Info_set2DVibrationM();
        }
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    } else if (ring->field_0x674[3] == 1) {
        if (i_useVibrationM) {
            dMeter2Info_set2DVibrationM();
        }
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_B, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    } else {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_select_item_force_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    int i_idx = mods::arg<int>(args, 1);
    if (!ring || i_idx < 0 || i_idx >= 4) return HOOK_CONTINUE;

    if (ring->field_0x674[i_idx] != 0) {
        if (i_idx == 3) {
            dComIfGs_setSelectItemIndex(i_idx, ring->field_0x6b4[i_idx]);
        } else {
            for (int i = 0; i < 3; i++) {
                dComIfGs_setMixItemIndex(i, ring->field_0x6b8[i]);
                dComIfGs_setSelectItemIndex(i, ring->field_0x6b4[i]);
            }
        }
        ring->field_0x674[i_idx] = 0;
#if TARGET_PC
        ring->mSelectItemSlideElapsed[i_idx] = 0.0f;
#endif
        g_zInventorySlot = ring->field_0x6b4[2];
        g_zMixSlot = ring->field_0x6b8[2];
        sync_z_item_state();
        update_z_item_texture();
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_select_item_index_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || g_inSetSelectItemIndex) {
        return HOOK_CONTINUE;
    }

    dSv_player_status_a_c* status = mods::arg<dSv_player_status_a_c*>(args, 0);
    int i_no = mods::arg<int>(args, 1);
    u8 i_slotNo = mods::arg<u8>(args, 2);

    if (status == nullptr) {
        return HOOK_CONTINUE;
    }

    if (i_no == 2) {
        status->mSelectItem[2] = i_slotNo;
        g_zInventorySlot = i_slotNo;
        sync_z_item_state();
        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
}

HookAction on_set_mix_item_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring) return HOOK_CONTINUE;

    bool bVar1 = false;
    u8 selectItemIndex0 = dComIfGs_getSelectItemIndex(0);
    u8 selectItemIndex1 = dComIfGs_getSelectItemIndex(1);
    u8 selectItemIndex2 = dComIfGs_getSelectItemIndex(2);
    u8 local_28[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

    u8 item = dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false);

    if (dComIfGs_getMixItemIndex(0) == 4 && ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(0)) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        ring->field_0x6cb = selectItemIndex0;
        selectItemIndex0 = 4;
        local_28[0] = get_ring_slot_for_item(ring, 4);
        ring->field_0x6b8[0] = 0xFF;
        ring->field_0x6b3 = 0;
        ring->field_0x6cd = 0;
        bVar1 = true;
    } else if (dComIfGs_getMixItemIndex(1) == 4 && ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(1)) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        ring->field_0x6cb = selectItemIndex1;
        selectItemIndex1 = 4;
        local_28[1] = get_ring_slot_for_item(ring, 4);
        ring->field_0x6b8[1] = 0xFF;
        ring->field_0x6b3 = 1;
        ring->field_0x6cd = 1;
        bVar1 = true;
    } else if (dComIfGs_getMixItemIndex(2) == 4 && ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(2)) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        ring->field_0x6cb = selectItemIndex2;
        selectItemIndex2 = 4;
        local_28[2] = get_ring_slot_for_item(ring, 4);
        ring->field_0x6b8[2] = 0xFF;
        ring->field_0x6b3 = 2;
        ring->field_0x6cd = 2;
        bVar1 = true;
    } else {
        switch (item) {
        case dItemNo_NORMAL_BOMB_e:
        case dItemNo_WATER_BOMB_e:
        case dItemNo_POKE_BOMB_e:
        case dItemNo_HAWK_EYE_e:
            if ((dComIfGs_getSelectItemIndex(0) == 4 && dComIfGs_getMixItemIndex(0) == 0xFF) ||
                dComIfGs_getMixItemIndex(0) == 4)
            {
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_ON, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                selectItemIndex0 = ring->mItemSlots[ring->mCurrentSlot];
                ring->field_0x6b8[0] = 4;
                ring->field_0x6b3 = 0;
                ring->mXButtonSlot = ring->mCurrentSlot;
                ring->field_0x6cd = 0xFF;
                bVar1 = true;
                if (selectItemIndex1 == ring->mItemSlots[ring->mCurrentSlot]) {
                    selectItemIndex1 = 0xFF;
                    ring->mYButtonSlot = 0xFF;
                } else if (selectItemIndex2 == ring->mItemSlots[ring->mCurrentSlot]) {
                    selectItemIndex2 = 0xFF;
                    ring->field_0x6ac = 0xFF;
                }
            } else if ((dComIfGs_getSelectItemIndex(1) == 4 && dComIfGs_getMixItemIndex(1) == 0xFF) ||
                       dComIfGs_getMixItemIndex(1) == 4)
            {
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_ON, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                selectItemIndex1 = ring->mItemSlots[ring->mCurrentSlot];
                ring->field_0x6b8[1] = 4;
                ring->field_0x6b3 = 1;
                ring->mYButtonSlot = ring->mCurrentSlot;
                ring->field_0x6cd = 0xFF;
                bVar1 = true;
                if (selectItemIndex0 == ring->mItemSlots[ring->mCurrentSlot]) {
                    selectItemIndex0 = 0xFF;
                    ring->mXButtonSlot = 0xFF;
                } else if (selectItemIndex2 == ring->mItemSlots[ring->mCurrentSlot]) {
                    selectItemIndex2 = 0xFF;
                    ring->field_0x6ac = 0xFF;
                }
            } else if ((dComIfGs_getSelectItemIndex(2) == 4 && dComIfGs_getMixItemIndex(2) == 0xFF) ||
                       dComIfGs_getMixItemIndex(2) == 4)
            {
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_ON, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                selectItemIndex2 = ring->mItemSlots[ring->mCurrentSlot];
                ring->field_0x6b8[2] = 4;
                ring->field_0x6b3 = 2;
                ring->field_0x6ac = ring->mCurrentSlot;
                ring->field_0x6cd = 0xFF;
                bVar1 = true;
                if (selectItemIndex0 == ring->mItemSlots[ring->mCurrentSlot]) {
                    selectItemIndex0 = 0xFF;
                    ring->mXButtonSlot = 0xFF;
                } else if (selectItemIndex1 == ring->mItemSlots[ring->mCurrentSlot]) {
                    selectItemIndex1 = 0xFF;
                    ring->mYButtonSlot = 0xFF;
                }
            }
            break;
        }
    }

    if (bVar1) {
        ring->field_0x6b4[0] = selectItemIndex0;
        ring->field_0x6b4[1] = selectItemIndex1;
        ring->field_0x6b4[2] = selectItemIndex2;
        ring->setJumpItem(false);
        if (local_28[0] != 0xFF) ring->mXButtonSlot = local_28[0];
        if (local_28[1] != 0xFF) ring->mYButtonSlot = local_28[1];
        if (local_28[2] != 0xFF) ring->field_0x6ac = local_28[2];

        dComIfGs_setSelectItemIndex(0, selectItemIndex0);
        dComIfGs_setSelectItemIndex(1, selectItemIndex1);
        dComIfGs_setSelectItemIndex(2, selectItemIndex2);
        dComIfGs_setMixItemIndex(0, ring->field_0x6b8[0]);
        dComIfGs_setMixItemIndex(1, ring->field_0x6b8[1]);
        dComIfGs_setMixItemIndex(2, ring->field_0x6b8[2]);

        g_zInventorySlot = selectItemIndex2;
        g_zMixSlot = ring->field_0x6b8[2];
        sync_z_item_state();
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_is_mix_item_on_pre(ModContext*, void* args, void* retval, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring || ring->mPlayerIsWolf) return HOOK_CONTINUE;

    u8 item = dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false);
    switch (item) {
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_HAWK_EYE_e:
        for (int i = 0; i < 3; i++) {
            if ((dComIfGs_getSelectItemIndex(i) == 4 && dComIfGs_getMixItemIndex(i) == 0xFF) ||
                (dComIfGs_getMixItemIndex(i) == 4))
            {
                *reinterpret_cast<bool*>(retval) = true;
                return HOOK_SKIP_ORIGINAL;
            }
        }
        break;
    }
    *reinterpret_cast<bool*>(retval) = false;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_is_mix_item_off_pre(ModContext*, void* args, void* retval, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring || ring->mPlayerIsWolf) return HOOK_CONTINUE;

    for (int i = 0; i < 3; i++) {
        if (dComIfGs_getMixItemIndex(i) == 4 &&
            ring->mItemSlots[ring->mCurrentSlot] == dComIfGs_getSelectItemIndex(i))
        {
            *reinterpret_cast<bool*>(retval) = true;
            return HOOK_SKIP_ORIGINAL;
        }
    }
    *reinterpret_cast<bool*>(retval) = false;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_check_explain_force_pre(ModContext*, void* args, void* retval, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) {
        return HOOK_CONTINUE;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring || ring->mPlayerIsWolf) return HOOK_CONTINUE;

    u8 item = dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false);
    u8 item0 = (dComIfGs_getSelectItemIndex(0) != 0xFF) ? dComIfGs_getItem(dComIfGs_getSelectItemIndex(0), false) : 0xFF;
    u8 item1 = (dComIfGs_getSelectItemIndex(1) != 0xFF) ? dComIfGs_getItem(dComIfGs_getSelectItemIndex(1), false) : 0xFF;
    u8 item2 = (dComIfGs_getSelectItemIndex(2) != 0xFF) ? dComIfGs_getItem(dComIfGs_getSelectItemIndex(2), false) : 0xFF;

    u8 local_18[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

    switch (item) {
    case dItemNo_BOW_e:
        switch (item0) {
        case dItemNo_NORMAL_BOMB_e:
        case dItemNo_WATER_BOMB_e:
        case dItemNo_POKE_BOMB_e:
            local_18[0] = dItemNo_BOMB_ARROW_e;
            break;
        case dItemNo_HAWK_EYE_e:
            local_18[0] = dItemNo_HAWK_ARROW_e;
            break;
        }
        switch (item1) {
        case dItemNo_NORMAL_BOMB_e:
        case dItemNo_WATER_BOMB_e:
        case dItemNo_POKE_BOMB_e:
            local_18[1] = dItemNo_BOMB_ARROW_e;
            break;
        case dItemNo_HAWK_EYE_e:
            local_18[1] = dItemNo_HAWK_ARROW_e;
            break;
        }
        switch (item2) {
        case dItemNo_NORMAL_BOMB_e:
        case dItemNo_WATER_BOMB_e:
        case dItemNo_POKE_BOMB_e:
            local_18[2] = dItemNo_BOMB_ARROW_e;
            break;
        case dItemNo_HAWK_EYE_e:
            local_18[2] = dItemNo_HAWK_ARROW_e;
            break;
        }
        break;
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
        if (item0 == dItemNo_BOW_e) local_18[0] = dItemNo_BOMB_ARROW_e;
        if (item1 == dItemNo_BOW_e) local_18[1] = dItemNo_BOMB_ARROW_e;
        if (item2 == dItemNo_BOW_e) local_18[2] = dItemNo_BOMB_ARROW_e;
        break;
    case dItemNo_HAWK_EYE_e:
        if (item0 == dItemNo_BOW_e) local_18[0] = dItemNo_HAWK_ARROW_e;
        if (item1 == dItemNo_BOW_e) local_18[1] = dItemNo_HAWK_ARROW_e;
        if (item2 == dItemNo_BOW_e) local_18[2] = dItemNo_HAWK_ARROW_e;
        break;
    case dItemNo_BEE_CHILD_e:
        if (item0 == dItemNo_FISHING_ROD_1_e) local_18[0] = dItemNo_BEE_ROD_e;
        if (item1 == dItemNo_FISHING_ROD_1_e) local_18[1] = dItemNo_BEE_ROD_e;
        if (item2 == dItemNo_FISHING_ROD_1_e) local_18[2] = dItemNo_BEE_ROD_e;
        break;
    case dItemNo_WORM_e:
        if (item0 == dItemNo_FISHING_ROD_1_e) local_18[0] = dItemNo_WORM_ROD_e;
        if (item1 == dItemNo_FISHING_ROD_1_e) local_18[1] = dItemNo_WORM_ROD_e;
        if (item2 == dItemNo_FISHING_ROD_1_e) local_18[2] = dItemNo_WORM_ROD_e;
        break;
    case dItemNo_ZORAS_JEWEL_e:
        if (item0 == dItemNo_FISHING_ROD_1_e) local_18[0] = dItemNo_JEWEL_ROD_e;
        if (item1 == dItemNo_FISHING_ROD_1_e) local_18[1] = dItemNo_JEWEL_ROD_e;
        if (item2 == dItemNo_FISHING_ROD_1_e) local_18[2] = dItemNo_JEWEL_ROD_e;
        break;
    case dItemNo_FISHING_ROD_1_e:
        if (item0 == dItemNo_BEE_CHILD_e) local_18[0] = dItemNo_BEE_ROD_e;
        else if (item0 == dItemNo_ZORAS_JEWEL_e) local_18[0] = dItemNo_JEWEL_ROD_e;
        else if (item0 == dItemNo_WORM_e) local_18[0] = dItemNo_WORM_ROD_e;

        if (item1 == dItemNo_BEE_CHILD_e) local_18[1] = dItemNo_BEE_ROD_e;
        else if (item1 == dItemNo_ZORAS_JEWEL_e) local_18[1] = dItemNo_JEWEL_ROD_e;
        else if (item1 == dItemNo_WORM_e) local_18[1] = dItemNo_WORM_ROD_e;

        if (item2 == dItemNo_BEE_CHILD_e) local_18[2] = dItemNo_BEE_ROD_e;
        else if (item2 == dItemNo_ZORAS_JEWEL_e) local_18[2] = dItemNo_JEWEL_ROD_e;
        else if (item2 == dItemNo_WORM_e) local_18[2] = dItemNo_WORM_ROD_e;
        break;
    }

    if (local_18[0] != 0xFF && local_18[1] == 0xFF && local_18[2] == 0xFF && local_18[3] == 0xFF && dComIfGs_getMixItemIndex(0) == 0xFF) {
        ring->field_0x6c7[0] = local_18[0];
        ring->field_0x6c7[1] = 0xFF;
        ring->field_0x6c7[2] = 0xFF;
        ring->field_0x6c7[3] = 0xFF;
    } else if (local_18[0] == 0xFF && local_18[1] != 0xFF && local_18[2] == 0xFF && local_18[3] == 0xFF && dComIfGs_getMixItemIndex(1) == 0xFF) {
        ring->field_0x6c7[0] = 0xFF;
        ring->field_0x6c7[1] = local_18[1];
        ring->field_0x6c7[2] = 0xFF;
        ring->field_0x6c7[3] = 0xFF;
    } else if (local_18[0] == 0xFF && local_18[1] == 0xFF && local_18[2] != 0xFF && local_18[3] == 0xFF && dComIfGs_getMixItemIndex(2) == 0xFF) {
        ring->field_0x6c7[0] = 0xFF;
        ring->field_0x6c7[1] = 0xFF;
        ring->field_0x6c7[2] = local_18[2];
        ring->field_0x6c7[3] = 0xFF;
    } else {
        ring->field_0x6c7[0] = 0xFF;
        ring->field_0x6c7[1] = 0xFF;
        ring->field_0x6c7[2] = 0xFF;
        ring->field_0x6c7[3] = 0xFF;
    }

    *reinterpret_cast<bool*>(retval) = (ring->field_0x6c7[0] != 0xFF || ring->field_0x6c7[1] != 0xFF || ring->field_0x6c7[2] != 0xFF);
    return HOOK_SKIP_ORIGINAL;
}


void on_check_status_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    u8 windowStatus = dMeter2Info_getWindowStatus();
    dMeter2_c* meter = *reinterpret_cast<dMeter2_c**>(args);
    if (meter != nullptr && windowStatus == 2) {
        u32* pStatus = reinterpret_cast<u32*>(reinterpret_cast<uintptr_t>(meter) + 0x124);
        *pStatus &= ~0x1000000u;
        g_meter2_info.onUseButton(0x800);
    }

    update_z_item_texture();
}

HookAction on_set_select_item_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) return HOOK_CONTINUE;
    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    int i_idx = mods::arg<int>(args, 1);
    u8 i_itemNo = mods::arg<u8>(args, 2);

    if (ring == nullptr || i_idx < 0 || i_idx >= 4) return HOOK_CONTINUE;

    f32 texScale = 1.0f;
    if (i_itemNo != dItemNo_NONE_e) {
        if (ring->field_0x6be[i_idx] == 0) {
            ring->field_0x6be[i_idx] = 1;
        } else {
            ring->field_0x6be[i_idx] = 0;
        }
        u8 bufIdx = ring->field_0x6be[i_idx];
        if (bufIdx > 2) bufIdx = 0;

        ring->field_0x686[i_idx] = dMeter2Info_readItemTexture(
            i_itemNo,
            ring->mpSelectItemTexBuf[i_idx][bufIdx][0], ring->mpSelectItemTex[i_idx][0],
            ring->mpSelectItemTexBuf[i_idx][bufIdx][1], ring->mpSelectItemTex[i_idx][1],
            nullptr, ring->mpSelectItemTex[i_idx][2],
            nullptr, nullptr,
            -1
        );
        texScale = dItem_data::getTexScale(i_itemNo) / 100.0f;

        if (ring->mpSelectItemTexBuf[i_idx][ring->field_0x6be[i_idx]][0] != nullptr) {
            ring->field_0x548[i_idx] = ring->mpSelectItemTexBuf[i_idx][ring->field_0x6be[i_idx]][0]->width / 48.0f * texScale;
            ring->field_0x558[i_idx] = ring->mpSelectItemTexBuf[i_idx][ring->field_0x6be[i_idx]][0]->height / 48.0f * texScale;
        }
    } else {
        ring->field_0x686[i_idx] = 0;
        ring->field_0x548[i_idx] = 0.0f;
        ring->field_0x558[i_idx] = 0.0f;
    }
    return HOOK_SKIP_ORIGINAL;
}

struct RingZButtonPrompt {
    dMenu_Ring_c* ring = nullptr;
    J2DScreen* screen = nullptr;
    CPaneMgr* button = nullptr;
};

static RingZButtonPrompt s_ringZPrompt;

static void hide_pane_tree(J2DPane* pane) {
    if (pane == nullptr) return;
    pane->hide();
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr; child = child->getNextChildPane()) {
        hide_pane_tree(child);
    }
}

static void show_pane_tree(J2DPane* pane) {
    if (pane == nullptr) return;
    pane->show();
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr; child = child->getNextChildPane()) {
        show_pane_tree(child);
    }
}

static void show_pane_parents(J2DPane* pane) {
    for (J2DPane* parent = pane; parent != nullptr; parent = parent->getParentPane()) {
        parent->show();
    }
}

static void clear_ring_z_prompt_refs() {
    s_ringZPrompt.button = nullptr;
    s_ringZPrompt.screen = nullptr;
    s_ringZPrompt.ring = nullptr;
}

static bool s_guideBaseXValid = false;
static f32 s_guideBaseX = 0.0f;
static constexpr f32 kItemWheelPromptXOffset = -15.0f;

void reset_ring_z_prompt() {
    clear_ring_z_prompt_refs();
    // Put the shared HIO guide X back to its pristine value. apply_item_wheel_
    // centering() re-derives base + offset on the next menu open, so it never
    // accumulates across fades / warps / stage loads.
    if (s_guideBaseXValid) {
        g_ringHIO.mGuidePosX[0] = s_guideBaseX;
    }
}

static void apply_item_wheel_centering(dMenu_Ring_c* ring) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine()) return;

    if (!s_guideBaseXValid) {
        s_guideBaseX = g_ringHIO.mGuidePosX[0];   // capture the pristine value once
        s_guideBaseXValid = true;
    }
    // Absolute write - idempotent, so repeated calls (each item-wheel open) can't
    // drift the guide text further left every time.
    g_ringHIO.mGuidePosX[0] = s_guideBaseX + kItemWheelPromptXOffset;

    if (ring != nullptr) {
        ring->mRingGuidePosX[0] = g_ringHIO.mGuidePosX[0];
        if (ring->mpTextParent[0] != nullptr) {
            ring->mpTextParent[0]->paneTrans(ring->mRingGuidePosX[0], ring->mRingGuidePosY[0]);
        }
    }
}

static void destroy_ring_z_prompt(dMenu_Ring_c* ring) {
    if (s_ringZPrompt.ring != ring) return;
    // The ring menu owns this heap lifetime; keep only per-menu references here.
    clear_ring_z_prompt_refs();
}

static void create_ring_z_prompt(dMenu_Ring_c* ring) {
    clear_ring_z_prompt_refs();
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || ring == nullptr || ring->mPlayerIsWolf || ring->mpScreen == nullptr) {
        return;
    }

    apply_item_wheel_centering(ring);

    auto* archive = dComIfGp_getMain2DArchive();
    if (archive == nullptr) return;

    J2DPane* anchor = ring->mpScreen->search(MULTI_CHAR('r_btn_n'));
    if (anchor != nullptr) {
        anchor->translate(anchor->getTranslateX() + 64.0f, anchor->getTranslateY());
        anchor->hide();
    }

    J2DScreen* screen = JKR_NEW J2DScreen();
    if (screen == nullptr) return;
    if (!screen->setPriority("zelda_game_image.blo", 0x20000, archive)) {
        JKR_DELETE(screen);
        return;
    }

    dPaneClass_showNullPane(screen);
    hide_pane_tree(screen->search('ROOT'));

    J2DPane* zButtonPane = screen->search(MULTI_CHAR('zbtn_n'));
    if (zButtonPane == nullptr) {
        JKR_DELETE(screen);
        return;
    }

    show_pane_parents(zButtonPane);
    show_pane_tree(zButtonPane);

    CPaneMgr* button = JKR_NEW CPaneMgr(screen, MULTI_CHAR('zbtn_n'), 2, nullptr);
    if (button == nullptr) {
        JKR_DELETE(screen);
        return;
    }

    button->setAlphaRate(1.0f);
    button->show();
    s_ringZPrompt = {.ring = ring, .screen = screen, .button = button};
}

static void draw_ring_z_prompt(dMenu_Ring_c* ring) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() ||
        s_ringZPrompt.ring != ring ||
        s_ringZPrompt.screen == nullptr || s_ringZPrompt.button == nullptr ||
        ring == nullptr || ring->mpScreen == nullptr || ring->mPlayerIsWolf)
    {
        return;
    }

    if (ring->mStatus == dMenu_Ring_c::STATUS_EXPLAIN ||
        ring->mStatus == dMenu_Ring_c::STATUS_EXPLAIN_FORCE ||
        ring->mpItemExplain == nullptr || ring->mpItemExplain->getStatus() != 0)
    {
        return;
    }

    apply_item_wheel_centering(ring);

    J2DPane* anchor = ring->mpScreen->search(MULTI_CHAR('r_btn_n'));
    if (anchor == nullptr) return;

    CPaneMgr paneMgr;
    Vec pos = paneMgr.getGlobalVtxCenter(anchor, true, 0);
    pos.x += ring->mCenterPosX;
    pos.y += ring->mCenterPosY;
    pos.x += 5.0f;
    pos.y -= 5.0f;

    s_ringZPrompt.button->scale(0.9f, 0.9f);
    s_ringZPrompt.button->paneTrans(
        pos.x - s_ringZPrompt.button->getInitGlobalCenterPosX(),
        pos.y - s_ringZPrompt.button->getInitGlobalCenterPosY()
    );
    s_ringZPrompt.button->setAlphaRate(ring->mAlphaRate);
    s_ringZPrompt.screen->draw(0.0f, 0.0f, dComIfGp_getCurrentGrafPort());
}

void after_ring_create(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) return;
    create_ring_z_prompt(mods::arg<dMenu_Ring_c*>(args, 0));
}

HookAction before_ring_delete(ModContext*, void* args, void*, void*) {
    if (args) {
        destroy_ring_z_prompt(mods::arg<dMenu_Ring_c*>(args, 0));
    }
    return HOOK_CONTINUE;
}

void after_ring_draw(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || isNativeZButtonEngine() || !args) return;
    draw_ring_z_prompt(mods::arg<dMenu_Ring_c*>(args, 0));
}




