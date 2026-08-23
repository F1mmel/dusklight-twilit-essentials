#pragma once

#include "z_common.hpp"

bool g_configCustomZButtonEnabled = false;
bool g_configZButtonEnabled = false;

const LogService* g_zLogSvc = nullptr;
ModContext* g_zModCtx = nullptr;

u8 g_zInventorySlot = 0xFF;
u8 g_zMixSlot = 0xFF;
u8 g_zPendingZSlot = 0xFF;

bool g_dpadLeftHeld = false;
bool g_dpadLeftTrig = false;
bool g_physZHeld = false;
bool g_physZTrig = false;

bool g_inSetSelectItemIndex = false;
dMenu_Ring_c* g_activeRing = nullptr;

alignas(32) static u8 s_staticTexBufMain[2][0x1000];
alignas(32) static u8 s_staticTexBufShine[2][0x1000];

u8* g_zTexBufMain[2] = { s_staticTexBufMain[0], s_staticTexBufMain[1] };
u8* g_zTexBufShine[2] = { s_staticTexBufShine[0], s_staticTexBufShine[1] };
u8 g_zTexBufIdx = 0;

J2DPicture* g_cachedZMainPic = nullptr;
f32 g_cachedZW = 32.0f;
f32 g_cachedZH = 32.0f;
u8 g_lastLoadedZItem = 0xFF;

J2DPicture* g_drawDigitPic[3] = { nullptr, nullptr, nullptr };
dKantera_icon_c* g_zKanteraIcon = nullptr;

void log_z_info(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (g_zLogSvc && g_zModCtx) {
        g_zLogSvc->info(g_zModCtx, buf);
    }
    std::printf("%s\n", buf);
    std::fflush(stdout);
}

void ensure_z_buffers() {
    g_zTexBufMain[0] = s_staticTexBufMain[0];
    g_zTexBufMain[1] = s_staticTexBufMain[1];
    g_zTexBufShine[0] = s_staticTexBufShine[0];
    g_zTexBufShine[1] = s_staticTexBufShine[1];
}

void sync_z_item_state() {
    u8 zItem = (g_zInventorySlot != 0xFF) ? dComIfGs_getItem(g_zInventorySlot, true) : 0xFF;
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        for (u8 i = 0; i < 24; i++) {
            u8 itm = dComIfGs_getItem(i, true);
            if (itm != 0xFF && itm != 0x00 && itm != dItemNo_NONE_e) {
                g_zInventorySlot = i;
                zItem = itm;
                break;
            }
        }
    }
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) return;

    dComIfGs_setSelectItemIndex(2, g_zInventorySlot);
    g_dComIfG_gameInfo.play.setSelectItem(2, zItem);

    s16 count = 99;
    if (is_bomb_item(zItem)) {
        u8 bagIdx = 0;
        if (g_zInventorySlot == 15) bagIdx = 0;
        else if (g_zInventorySlot == 16) bagIdx = 1;
        else if (g_zInventorySlot == 17) bagIdx = 2;
        count = dComIfGs_getBombNum(bagIdx);
    } else if (zItem == 0x43 || zItem == 0x53 || zItem == 0x54 || zItem == 0x55 || zItem == 0x56 || zItem == 0x59 || zItem == 0x5A) {
        count = dComIfGs_getArrowNum();
    } else if (zItem == 0x4B) {
        count = dComIfGs_getPachinkoNum();
    }

    dComIfGp_setSelectItemNum(2, count);
}

void ensure_z_slot_initialized() {
    if (g_zInventorySlot == 0xFF) {
        u8 savedItemIdx = dComIfGs_getSelectItemIndex(2);
        u8 gpItem = dComIfGp_getSelectItem(2);
        u8 slot = find_slot_for_item(savedItemIdx);
        if (slot == 0xFF) {
            slot = find_slot_for_item(gpItem);
        }
        if (slot == 0xFF) {
            for (u8 i = 0; i < 24; i++) {
                u8 itm = dComIfGs_getItem(i, false);
                if (itm != 0xFF && itm != 0x00 && itm != dItemNo_NONE_e) {
                    slot = i;
                    break;
                }
            }
        }
        if (slot != 0xFF) {
            g_zInventorySlot = slot;
            sync_z_item_state();
            log_z_info("[ZButton] INITIAL LOAD: slot=%d item=0x%02X", g_zInventorySlot, dComIfGs_getItem(g_zInventorySlot, false));
        }
    } else {
        sync_z_item_state();
    }
}

u8 find_slot_for_item(u8 itemNo) {
    if (itemNo == 0xFF || itemNo == dItemNo_NONE_e) {
        return 0xFF;
    }
    if (itemNo < 24 && dComIfGs_getItem(itemNo, false) != dItemNo_NONE_e) {
        return itemNo;
    }
    for (u8 slot = 0; slot < 24; slot++) {
        if (dComIfGs_getItem(slot, false) == itemNo) {
            return slot;
        }
    }
    return 0xFF;
}

bool is_bomb_item(u8 itemNo) {
    return itemNo == 0x4F || itemNo == 0x50 || itemNo == 0x51 || itemNo == 0x70 || itemNo == 0x71 || itemNo == 0x72;
}

bool isWolfPlayer() {
    daPy_py_c* player = daPy_getLinkPlayerActorClass();
    return player != nullptr && player->checkWolf();
}

bool isTitleOrMainMenu() {
    daPy_py_c* player = daPy_getLinkPlayerActorClass();
    if (player == nullptr) {
        return true;
    }
    const char* stageName = dComIfGp_getStartStageName();
    if (stageName != nullptr) {
        if (std::strcmp(stageName, "F_SP102") == 0 ||
            std::strcmp(stageName, "title") == 0 ||
            std::strcmp(stageName, "opening") == 0 ||
            std::strcmp(stageName, "name") == 0) {
            return true;
        }
    }
    return false;
}

bool isMidnaUnlocked() {
    if (isWolfPlayer()) {
        return true;
    }
    if (dComIfGs_getTransformStatus() != 0) {
        return true;
    }
    if (dComIfGs_isEventBit(0x0520) || dComIfGs_isEventBit(0x0510) ||
        dComIfGs_isEventBit(0x0501) || dComIfGs_isEventBit(0x0640) ||
        dComIfGs_isEventBit(0x0504) || dComIfGs_isEventBit(0x0502)) {
        return true;
    }
    return false;
}

bool is_pause_menu_open(dMeter2Draw_c* draw) {
    u8 winStatus = g_meter2_info.getWindowStatus();
    if (winStatus == 2) {
        return false;
    }

    if (dComIfGp_event_runCheck()) return true;
    if (dComIfGp_isPauseFlag()) return true;
    if (g_meter2_info.getPauseStatus() != 0) return true;
    if (winStatus != 0) return true;

    if (draw == nullptr && g_meter2_info.getMeterClass() != nullptr) {
        draw = g_meter2_info.getMeterClass()->getMeterDrawPtr();
    }
    if (draw != nullptr && draw->getMainScreenPtr() != nullptr) {
        J2DScreen* screen = draw->getMainScreenPtr();
        J2DPane* contPane = screen->search(MULTI_CHAR('cont_n'));
        if (contPane != nullptr && (!contPane->isVisible() || contPane->getAlpha() == 0)) {
            return true;
        }
    }
    return false;
}

bool is_z_item_usable() {
    u8 winStatus = dMeter2Info_getWindowStatus();
    if (winStatus != 0) {
        return true;
    }
    if (isWolfPlayer()) {
        return false;
    }
    if (g_zInventorySlot == 0xFF) {
        return false;
    }
    u8 zItem = dComIfGs_getItem(g_zInventorySlot, false);
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        return false;
    }
    if (!dMeter2Info_isUseButton(4) && !dMeter2Info_isUseButton(8)) {
        return false;
    }
    return true;
}

void set_pane_influenced_alpha_recursive(J2DPane* pane, bool influenced) {
    if (!pane) return;
    pane->setInfluencedAlpha(influenced, true);
    JSUTreeIterator<J2DPane> it(pane->getFirstChild());
    while (it != pane->getEndChild()) {
        if (it.getObject() != nullptr) {
            set_pane_influenced_alpha_recursive(it.getObject(), influenced);
        }
        ++it;
    }
}

u8 combine_select_item(u8 playItem, u8 mixSlot) {
    if (mixSlot == dItemNo_NONE_e || mixSlot == 0xFF) {
        return playItem;
    }

    u8 saveItem = (mixSlot < 24) ? dComIfGs_getItem(mixSlot, false) : dItemNo_NONE_e;
    if (saveItem == dItemNo_BOW_e || mixSlot == 4) {
        saveItem = playItem;
        playItem = dItemNo_BOW_e;
    } else if (saveItem == dItemNo_FISHING_ROD_1_e) {
        saveItem = playItem;
        playItem = dItemNo_FISHING_ROD_1_e;
    }

    if (playItem == dItemNo_BOW_e) {
        switch (saveItem) {
        case dItemNo_NORMAL_BOMB_e:
        case dItemNo_WATER_BOMB_e:
        case dItemNo_POKE_BOMB_e:
        case dItemNo_BOMB_BAG_LV1_e:
            return dItemNo_BOMB_ARROW_e;
        case dItemNo_HAWK_EYE_e:
            return dItemNo_HAWK_ARROW_e;
        default:
            break;
        }
    } else if (playItem == dItemNo_FISHING_ROD_1_e) {
        switch (saveItem) {
        case dItemNo_BEE_CHILD_e:
            return dItemNo_BEE_ROD_e;
        case dItemNo_WORM_e:
            return dItemNo_WORM_ROD_e;
        case dItemNo_ZORAS_JEWEL_e:
            return dItemNo_JEWEL_ROD_e;
        default:
            break;
        }
    }

    return playItem;
}

u8 resolved_select_item(int index) {
    u8 slot = (index == 2) ? g_zInventorySlot : dComIfGs_getSelectItemIndex(index);
    if (slot == dItemNo_NONE_e || slot == 0xFF || slot >= 24) {
        return dItemNo_NONE_e;
    }

    u8 mixSlot = (index == 2) ? g_zMixSlot : dComIfGs_getMixItemIndex(index);
    return combine_select_item(dComIfGs_getItem(slot, false), mixSlot);
}

void sync_play_select_item(int index) {
    if (index != 2) return;
    g_dComIfG_gameInfo.play.setSelectItem(2, resolved_select_item(2));
}

