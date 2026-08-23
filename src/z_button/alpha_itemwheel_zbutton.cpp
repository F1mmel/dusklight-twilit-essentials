#pragma once

#include "alpha_itemwheel_zbutton.hpp"

DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SetActiveCursorHook);
DEFINE_HOOK(&dSv_player_status_a_c::setSelectItemIndex, SetSelectItemIndexHook);
DEFINE_HOOK(&dMeter2_c::checkStatus, CheckStatusHook);

u8 get_ring_slot_for_item(dMenu_Ring_c* ring, u8 slotOrItem) {
    if (!ring || slotOrItem == 0xFF || slotOrItem == dItemNo_NONE_e) return 0xFF;
    u8 targetItem = (slotOrItem < 24) ? dComIfGs_getItem(slotOrItem, false) : slotOrItem;
    if (targetItem == 0xFF || targetItem == 0x00 || targetItem == dItemNo_NONE_e) return 0xFF;

    u8 total = ring->mItemsTotal;
    u8* slots = ring->mItemSlots;
    if (!slots) return 0xFF;
    for (int i = 0; i < total; i++) {
        if (slots[i] == targetItem) {
            return (u8)i;
        }
    }
    return 0xFF;
}

void trigger_ring_item_slide_z(dMenu_Ring_c* ring, u8 itemNo) {
    if (!ring) return;
    ring->setSelectItem(2, itemNo);
    u8 currentSlot = ring->mCurrentSlot;
    ring->field_0x6ac = currentSlot;
    ring->field_0x518[2] = ring->mItemSlotPosX[currentSlot];
    ring->field_0x528[2] = ring->mItemSlotPosY[currentSlot];
    ring->field_0x538[2] = g_ringHIO.mSelectItemScale;
    ring->field_0x6b4[2] = itemNo;
    ring->field_0x674[2] = 1;
}

void commit_pending_z_slot(dMenu_Ring_c* ring) {
    if (g_zPendingZSlot == 0xFF) {
        return;
    }

    if (ring != nullptr) {
        u8 slideTimer = (u8)ring->field_0x674[2];
        if (slideTimer != 0) {
            return;
        }
    }

    g_zInventorySlot = g_zPendingZSlot;
    g_zPendingZSlot = 0xFF;

    sync_z_item_state();
    update_z_item_texture();
}

void update_ring_z_slots(dMenu_Ring_c* ring) {
    if (!ring) return;
    u8 zSlot = g_zInventorySlot;
    if (zSlot == 0xFF) {
        zSlot = find_slot_for_item(dComIfGs_getSelectItemIndex(2));
        g_zInventorySlot = zSlot;
    }
    u8 zItemNo = (zSlot != 0xFF && zSlot < 24) ? dComIfGs_getItem(zSlot, false) : 0xFF;
    ring->field_0x6b4[2] = zItemNo;
    u8 ringSlot = get_ring_slot_for_item(ring, zSlot);
    ring->field_0x6ac = ringSlot;
    if (zItemNo != 0xFF && zItemNo != dItemNo_NONE_e) {
        ring->setSelectItem(2, zItemNo);
        if (ringSlot != 0xFF && ringSlot < ring->mItemsTotal) {
            ring->field_0x518[2] = ring->mItemSlotPosX[ringSlot];
            ring->field_0x528[2] = ring->mItemSlotPosY[ringSlot];
            ring->field_0x538[2] = g_ringHIO.mSelectItemScale;
        }
    }
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

    u8 hoveredItemNo = ring->mItemSlots[ring->mCurrentSlot];
    u8 realSlot = find_slot_for_item(hoveredItemNo);

    if (realSlot == 0xFF) {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }

    u8 itemNo = dComIfGs_getItem(realSlot, false);
    if (itemNo == dItemNo_NONE_e || itemNo == 0x00 || itemNo == 0xFF) {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }

    u8 oldZSlot = g_zInventorySlot;
    if (oldZSlot == 0xFF) {
        oldZSlot = find_slot_for_item(dComIfGs_getSelectItemIndex(2));
        g_zInventorySlot = oldZSlot;
    }

    u8 oldZItemNo = (oldZSlot != 0xFF) ? dComIfGs_getItem(oldZSlot, false) : 0xFF;
    if (realSlot == oldZSlot || (oldZItemNo != 0xFF && itemNo == oldZItemNo)) {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }

    trigger_ring_item_slide_z(ring, itemNo);
    ring->field_0x6b4[2] = itemNo;

    u8 xVal = dComIfGs_getSelectItemIndex(0);
    u8 yVal = dComIfGs_getSelectItemIndex(1);

    u8 xItemNo = (xVal < 24) ? dComIfGs_getItem(xVal, false) : xVal;
    u8 yItemNo = (yVal < 24) ? dComIfGs_getItem(yVal, false) : yVal;

    bool xMatch = (xVal == realSlot) || (xItemNo != 0xFF && xItemNo == itemNo);
    bool yMatch = (yVal == realSlot) || (yItemNo != 0xFF && yItemNo == itemNo);

    u8 itemToMoveToXY = (oldZSlot != 0xFF) ? dComIfGs_getItem(oldZSlot, false) : 0xFF;

    if (xMatch) {
        dComIfGs_setSelectItemIndex(0, (oldZSlot != 0xFF) ? oldZSlot : 0xFF);
        ring->mXButtonSlot = get_ring_slot_for_item(ring, oldZSlot);
        ring->field_0x6b4[0] = itemToMoveToXY;
    } else if (yMatch) {
        dComIfGs_setSelectItemIndex(1, (oldZSlot != 0xFF) ? oldZSlot : 0xFF);
        ring->mYButtonSlot = get_ring_slot_for_item(ring, oldZSlot);
        ring->field_0x6b4[1] = itemToMoveToXY;
    }

    g_zPendingZSlot = realSlot;
    g_zInventorySlot = realSlot;
    sync_z_item_state();

    trigger_ring_item_slide_z(ring, itemNo);
    update_ring_z_slots(ring);

    dMeter2Info_set2DVibrationM();
    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
}

HookAction on_set_select_item_index_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || g_inSetSelectItemIndex) {
        return HOOK_CONTINUE;
    }

    struct Guard {
        Guard() { g_inSetSelectItemIndex = true; }
        ~Guard() { g_inSetSelectItemIndex = false; }
    } guard;

    dSv_player_status_a_c* status = mods::arg<dSv_player_status_a_c*>(args, 0);
    int i_no = mods::arg<int>(args, 1);
    u8 i_slotNo = mods::arg<u8>(args, 2);

    if (status == nullptr) {
        return HOOK_CONTINUE;
    }

    u8 targetSlot = find_slot_for_item(i_slotNo);

    if (i_no == 2) {
        if (g_zPendingZSlot != 0xFF && g_zInventorySlot != 0xFF) {
            status->mSelectItem[2] = g_zInventorySlot;
        } else if (g_zInventorySlot != 0xFF) {
            status->mSelectItem[2] = g_zInventorySlot;
        } else if (targetSlot != 0xFF) {
            g_zInventorySlot = targetSlot;
            status->mSelectItem[2] = targetSlot;
        }
        sync_z_item_state();
        return HOOK_SKIP_ORIGINAL;
    }

    if (i_no == 0 || i_no == 1) {
        if (targetSlot == 0xFF) {
            status->mSelectItem[i_no] = 0xFF;
            return HOOK_SKIP_ORIGINAL;
        }

        u8 currentSlot = status->mSelectItem[i_no];

        if (targetSlot == currentSlot) {
            status->mSelectItem[i_no] = targetSlot;
            return HOOK_SKIP_ORIGINAL;
        }

        u8 currentZSlot = g_zInventorySlot;
        if (currentZSlot == 0xFF) {
            currentZSlot = find_slot_for_item(status->mSelectItem[2]);
            g_zInventorySlot = currentZSlot;
        }

        int otherButtonIdx = 1 - i_no;
        u8 otherSlot = status->mSelectItem[otherButtonIdx];

        if (currentZSlot != 0xFF && targetSlot == currentZSlot) {
            u8 oldXYSlot = currentSlot;
            u8 newZSlot = (oldXYSlot < 24) ? oldXYSlot : find_slot_for_item(oldXYSlot);

            g_zInventorySlot = newZSlot;
            status->mSelectItem[2] = (newZSlot != 0xFF) ? newZSlot : 0xFF;
            dComIfGs_setSelectItemIndex(2, (newZSlot != 0xFF) ? newZSlot : 0xFF);

            if (g_activeRing) {
                update_ring_z_slots(g_activeRing);
            }
        }
        else if (otherSlot != 0xFF && targetSlot == otherSlot) {
            u8 oldSlot = currentSlot;
            status->mSelectItem[otherButtonIdx] = oldSlot;
            dComIfGs_setSelectItemIndex(otherButtonIdx, oldSlot);

            if (g_activeRing) {
                if (otherButtonIdx == 0) {
                    g_activeRing->mXButtonSlot = get_ring_slot_for_item(g_activeRing, oldSlot);
                    g_activeRing->field_0x6b4[0] = (oldSlot < 24) ? dComIfGs_getItem(oldSlot, false) : 0xFF;
                } else {
                    g_activeRing->mYButtonSlot = get_ring_slot_for_item(g_activeRing, oldSlot);
                    g_activeRing->field_0x6b4[1] = (oldSlot < 24) ? dComIfGs_getItem(oldSlot, false) : 0xFF;
                }
            }
        }

        status->mSelectItem[i_no] = targetSlot;
        dComIfGs_setSelectItemIndex(i_no, targetSlot);
        sync_z_item_state();

        if (g_activeRing) {
            update_ring_z_slots(g_activeRing);
            if (i_no == 0) {
                g_activeRing->mXButtonSlot = get_ring_slot_for_item(g_activeRing, targetSlot);
                g_activeRing->field_0x6b4[0] = (targetSlot < 24) ? dComIfGs_getItem(targetSlot, false) : 0xFF;
            } else {
                g_activeRing->mYButtonSlot = get_ring_slot_for_item(g_activeRing, targetSlot);
                g_activeRing->field_0x6b4[1] = (targetSlot < 24) ? dComIfGs_getItem(targetSlot, false) : 0xFF;
            }
        }

        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
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
