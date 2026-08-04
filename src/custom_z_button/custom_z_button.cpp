#include "custom_z_button.hpp"
#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "d/d_meter_HIO.h"
#include "d/d_meter2_info.h"
#include "d/d_meter2.h"
#define private public
#include "d/d_meter2_draw.h"
#include "d/d_meter_button.h"
#include "d/d_menu_ring.h"
#undef private
#include "d/d_pane_class.h"
#include "d/d_kantera_icon_meter.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "f_op/f_op_actor_mng.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include <cstdio>
#include <cstring>

bool g_configCustomZButtonEnabled = false;

static const LogService* s_logSvc = nullptr;
static ModContext* s_modCtx = nullptr;
static bool s_dpadLeftHeld = false;
static bool s_dpadLeftTrig = false;

static u8* s_zTexBufMain[2] = { nullptr, nullptr };
static u8* s_zTexBufShine[2] = { nullptr, nullptr };
static u8 s_zTexBufIdx = 0;

static void update_z_item_texture(dMeter2Draw_c* draw = nullptr);
static void init_z_hud_nodes(dMeter2Draw_c* draw);
static void set_pane_influenced_alpha_recursive(J2DPane* pane, bool influenced);

// We keep track of the Z slot ourselves because mSelectItem[2] stores an ItemNo,
// not a slot index. Passing that to dComIfGs_getItem() reads garbage.
static u8 s_zInventorySlot = 0xFF;
static u8 s_pendingZSlot = 0xFF;

static bool is_bomb_item(u8 item) {
    return item == 0x50 || item == 0x53 || item == 0x54 || item == 0x70 || item == 0x71 || item == 0x72;
}

// Finds the inventory slot index (0..23) for a given ItemNo or returns itemNo if already a valid slot index.
static u8 find_slot_for_item(u8 itemNo) {
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

static void sync_z_item_state() {
    u8 zItem = (s_zInventorySlot != 0xFF) ? dComIfGs_getItem(s_zInventorySlot, false) : 0xFF;
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        // Slot got emptied somehow, scan for any valid item
        for (u8 i = 0; i < 24; i++) {
            u8 itm = dComIfGs_getItem(i, false);
            if (itm != 0xFF && itm != 0x00 && itm != dItemNo_NONE_e) {
                s_zInventorySlot = i;
                zItem = itm;
                break;
            }
        }
    }
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) return;

    dComIfGs_setSelectItemIndex(2, s_zInventorySlot);
    g_dComIfG_gameInfo.play.setSelectItem(2, zItem);

    s16 count = 99;
    if (is_bomb_item(zItem)) {
        u8 bagIdx = 0;
        if (s_zInventorySlot == 15) bagIdx = 0;
        else if (s_zInventorySlot == 16) bagIdx = 1;
        else if (s_zInventorySlot == 17) bagIdx = 2;
        count = dComIfGs_getBombNum(bagIdx);
    } else if (zItem == 0x43 || zItem == 0x44) {
        count = dComIfGs_getArrowNum();
    } else if (zItem == 0x47) {
        count = dComIfGs_getPachinkoNum();
    }

    dComIfGp_setSelectItemNum(2, count);
}

static u8 get_ring_items_total(dMenu_Ring_c* ring) {
    return ring ? ring->mItemsTotal : 0;
}
static u8* get_ring_item_slots(dMenu_Ring_c* ring) {
    return ring ? ring->mItemSlots : nullptr;
}
static u8& get_ring_x_button_slot(dMenu_Ring_c* ring) {
    return ring->mXButtonSlot;
}
static u8& get_ring_y_button_slot(dMenu_Ring_c* ring) {
    return ring->mYButtonSlot;
}
static u8& get_ring_z_button_slot(dMenu_Ring_c* ring) {
    return ring->field_0x6ac;
}
static u8* get_ring_field_0x6b4(dMenu_Ring_c* ring) {
    return ring->field_0x6b4;
}

static u8 get_ring_slot_for_item(dMenu_Ring_c* ring, u8 slotOrItem) {
    if (!ring || slotOrItem == 0xFF || slotOrItem == dItemNo_NONE_e) return 0xFF;
    u8 targetItem = (slotOrItem < 24) ? dComIfGs_getItem(slotOrItem, false) : slotOrItem;
    if (targetItem == 0xFF || targetItem == 0x00 || targetItem == dItemNo_NONE_e) return 0xFF;

    u8 total = get_ring_items_total(ring);
    u8* slots = get_ring_item_slots(ring);
    if (!slots) return 0xFF;
    for (int i = 0; i < total; i++) {
        if (slots[i] == targetItem) {
            return (u8)i;
        }
    }
    return 0xFF;
}

static u8 get_ring_slide_timer(dMenu_Ring_c* ring, int i_idx) {
    if (!ring) return 0;
    return (u8)ring->field_0x674[i_idx];
}

static u8 get_ring_current_slot_item(dMenu_Ring_c* ring) {
    if (!ring) return 0xFF;
    return ring->mItemSlots[ring->mCurrentSlot];
}

static void trigger_ring_item_slide_z(dMenu_Ring_c* ring, u8 itemNo) {
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

static void commit_pending_z_slot(dMenu_Ring_c* ring = nullptr) {
    if (s_pendingZSlot == 0xFF) {
        return;
    }

    // Wait for the item slide animation to finish before committing
    if (ring != nullptr) {
        u8 slideTimer = get_ring_slide_timer(ring, 2);
        if (slideTimer != 0) {
            return;
        }
    }

    s_zInventorySlot = s_pendingZSlot;
    s_pendingZSlot = 0xFF;

    sync_z_item_state();
    update_z_item_texture();
}

static void ensure_z_buffers() {
    if (s_zTexBufMain[0] == nullptr) {
#if defined(_MSC_VER)
        s_zTexBufMain[0] = static_cast<u8*>(_aligned_malloc(0xC00, 32));
        s_zTexBufMain[1] = static_cast<u8*>(_aligned_malloc(0xC00, 32));
        s_zTexBufShine[0] = static_cast<u8*>(_aligned_malloc(0xC00, 32));
        s_zTexBufShine[1] = static_cast<u8*>(_aligned_malloc(0xC00, 32));
#else
        s_zTexBufMain[0] = static_cast<u8*>(aligned_alloc(32, 0xC00));
        s_zTexBufMain[1] = static_cast<u8*>(aligned_alloc(32, 0xC00));
        s_zTexBufShine[0] = static_cast<u8*>(aligned_alloc(32, 0xC00));
        s_zTexBufShine[1] = static_cast<u8*>(aligned_alloc(32, 0xC00));
#endif
    }
}

static J2DPicture* s_cachedZMainPic = nullptr;
static f32 s_cachedZW = 32.0f;
static f32 s_cachedZH = 32.0f;

static u8 s_lastLoadedZItem = 0xFF;

// These digit pictures get parented to zbtn_n so they fade with the Z button
static J2DPicture* s_zDigitPic[3] = { nullptr, nullptr, nullptr };
static J2DPane*    s_zDigitParent  = nullptr;
static bool        s_zDigitsInitialized = false;

// Owned by draw_item_count_digits – promoted to module scope so shutdown can null them
static J2DPicture* s_drawDigitPic[3] = { nullptr, nullptr, nullptr };

// Owned by on_meter2_draw_draw_post – promoted to module scope so shutdown can null it
static dKantera_icon_c* s_zKanteraIcon = nullptr;

static void ensure_z_slot_initialized() {
    if (s_zInventorySlot == 0xFF) {
        u8 savedItemIdx = dComIfGs_getSelectItemIndex(2);
        u8 gpItem = dComIfGp_getSelectItem(2);
        u8 slot = find_slot_for_item(savedItemIdx);
        if (slot == 0xFF) {
            slot = find_slot_for_item(gpItem);
        }
        if (slot == 0xFF) {
            // Neither saved index nor gpItem gave us anything, scan the whole inventory
            for (u8 i = 0; i < 24; i++) {
                u8 itm = dComIfGs_getItem(i, false);
                if (itm != 0xFF && itm != 0x00 && itm != dItemNo_NONE_e) {
                    slot = i;
                    break;
                }
            }
        }
        if (slot != 0xFF) {
            s_zInventorySlot = slot;
            sync_z_item_state();

            if (s_logSvc && s_modCtx) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "[DpadMidna] INITIAL LOAD DETECTED: savedItemIdx=0x%02X, gpItem=0x%02X -> Resolved s_zInventorySlot=%d",
                    savedItemIdx, gpItem, s_zInventorySlot);
                s_logSvc->info(s_modCtx, buf);
            }
        }
    } else {
        sync_z_item_state();
    }
}

static bool isTitleOrMainMenu() {
    daPy_py_c* player = daPy_getLinkPlayerActorClass();
    if (player == nullptr) {
        return true;
    }
    const char* stageName = dComIfGp_getStartStageName();
    if (stageName != nullptr) {
        if (std::strcmp(stageName, "F_SP102") == 0 || std::strcmp(stageName, "title") == 0) {
            return true;
        }
    }
    return false;
}

static bool isWolfPlayer() {
    daAlink_c* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (player != nullptr && player->checkWolf()) {
        return true;
    }
    return false;
}

static bool is_pause_menu_open(dMeter2Draw_c* draw = nullptr) {
    u8 winStatus = g_meter2_info.getWindowStatus();
    // windowStatus 2 is the Item Wheel / Ring Menu (where Z item MUST stay visible)
    if (winStatus == 2) {
        return false;
    }

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

// Safe wrappers around CPaneMgr methods.
// CPaneMgrAlpha::hide()/show()/etc. all dereference an internal J2DPane* without
// null-checking it first. After a soft-reset the game heap is wiped, so that
// pointer becomes nullptr / garbage. Call these wrappers everywhere instead.
static inline bool pane_is_ready(CPaneMgr* p) {
    return p != nullptr && p->getPanePtr() != nullptr;
}
static inline void safe_pane_hide(CPaneMgr* p)  { if (pane_is_ready(p)) p->hide(); }
static inline void safe_pane_show(CPaneMgr* p)  { if (pane_is_ready(p)) p->show(); }
static inline void safe_pane_resize(CPaneMgr* p, f32 w, f32 h) { if (pane_is_ready(p)) p->resize(w, h); }
static inline void safe_pane_trans(CPaneMgr* p, f32 x, f32 y)  { if (pane_is_ready(p)) p->paneTrans(x, y); }
static inline void safe_pane_alpha_rate(CPaneMgr* p, f32 a)    { if (pane_is_ready(p)) p->setAlphaRate(a); }
static inline void safe_pane_alpha(CPaneMgr* p, u8 a)          { if (pane_is_ready(p)) p->setAlpha(a); }

static void update_z_item_texture(dMeter2Draw_c* draw) {
    if (draw == nullptr) {
        if (g_meter2_info.getMeterClass() != nullptr) {
            draw = g_meter2_info.getMeterClass()->getMeterDrawPtr();
        }
    }

    if (draw == nullptr || isTitleOrMainMenu()) {
        return;
    }

    CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
    if (isWolfPlayer() || is_pause_menu_open(draw)) {
        safe_pane_hide(itemR);
        s_cachedZMainPic = nullptr;
        return;
    }

    if (draw != nullptr) {
        init_z_hud_nodes(draw);
    }

    if (itemR && itemR->getPanePtr() && draw) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen) {
            J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
            J2DPane* itemPane = itemR->getPanePtr();
            if (zbtn && itemPane && itemPane->getParentPane() != zbtn) {
                zbtn->appendChild(itemPane);
                set_pane_influenced_alpha_recursive(itemPane, true);
            }
        }
    }

    ensure_z_buffers();
    ensure_z_slot_initialized();

    // dComIfGs_getSelectItemIndex(2) returns an ItemNo, not a slot index.
    // Using it directly with dComIfGs_getItem() would cause an out-of-bounds read.
    u8 zItem = dItemNo_NONE_e;
    if (s_zInventorySlot != 0xFF) {
        zItem = dComIfGs_getItem(s_zInventorySlot, false);
    }
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        u8 zItem1 = dComIfGp_getSelectItem(2);
        if (zItem1 != 0xFF && zItem1 != 0x00 && zItem1 != dItemNo_NONE_e) {
            zItem = zItem1;
        }
    }

    if (!itemR) {
        return;
    }

    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        safe_pane_hide(itemR);
        s_cachedZMainPic = nullptr;
        return;
    }

    J2DPane* pane = itemR->getPanePtr();
    if (pane == nullptr) {
        return;
    }

    J2DPicture* mainPic = static_cast<J2DPicture*>(pane);
    J2DPicture* shinePic = (pane->getFirstChild() != nullptr && pane->getFirstChild()->getObject() != nullptr)
        ? static_cast<J2DPicture*>(pane->getFirstChild()->getObject())
        : nullptr;

    // r_itm_pp would show a white ruby icon on top of the icon, always hide it
    if (shinePic) {
        shinePic->hide();
    }

    ResTIMG* mainBuf  = reinterpret_cast<ResTIMG*>(s_zTexBufMain[s_zTexBufIdx]);
    ResTIMG* shineBuf = reinterpret_cast<ResTIMG*>(s_zTexBufShine[s_zTexBufIdx]);

    bool forceReload = (s_lastLoadedZItem != zItem || s_cachedZMainPic != mainPic);
    if (!forceReload && mainPic != nullptr) {
        JUTTexture* currentTex = mainPic->getTexture(0);
        if (currentTex == nullptr || currentTex->getTexInfo() != mainBuf) {
            forceReload = true;
        }
    }
    s_cachedZMainPic = mainPic;

    if (forceReload) {
        s_lastLoadedZItem = zItem;

        s_zTexBufIdx = (s_zTexBufIdx == 0) ? 1 : 0;

        mainBuf  = reinterpret_cast<ResTIMG*>(s_zTexBufMain[s_zTexBufIdx]);
        shineBuf = reinterpret_cast<ResTIMG*>(s_zTexBufShine[s_zTexBufIdx]);

        int readResult = dMeter2Info_readItemTexture(
            zItem,
            mainBuf,
            mainPic,
            shineBuf,
            shinePic,
            nullptr, nullptr, nullptr, nullptr, -1);

        if (readResult == 0) {
            // Archive wasn't ready yet. Retry next frame.
            s_lastLoadedZItem = 0xFF;
            s_cachedZMainPic = nullptr;
        }

        if (shinePic) {
            shinePic->hide();
        }
    }

    f32 texScale = g_drawHIO.mItemScaleAdjustON
        ? (g_drawHIO.mItemScalePercent / 100.0f)
        : (dItem_data::getTexScale(zItem) / 100.0f);

    f32 baseSize = 42.0f;
    f32 w = texScale * ((mainBuf->width  * baseSize) / 48.0f);
    f32 h = texScale * ((mainBuf->height * baseSize) / 48.0f);

    f32 widthShift = (w / 42.0f) * 20.0f;
    f32 offsetX = (baseSize - w) * 0.5f + widthShift - 5;
    f32 offsetY = (baseSize - h) * 0.5f - 10;

    if (draw != nullptr) {
        draw->field_0x6ac[2] = offsetX;
        draw->field_0x6b8[2] = offsetY;
        draw->field_0x6c4[2] = w;
        draw->field_0x6d0[2] = h;

        J2DPane* midnaPane = draw->getMainScreenPtr()->search(MULTI_CHAR('midona_n'));
        J2DPane* jujiPane = draw->getMainScreenPtr()->search(MULTI_CHAR('juji_n'));
        if (jujiPane != nullptr) {
            f32 jujiY = jujiPane->getTranslateY();
            if (jujiY != 0.0f) {

            }

    static u32 s_diagTimer = 0;
    if (++s_diagTimer % 60 == 0 && s_logSvc && s_modCtx) {
        J2DScreen* scr = draw ? draw->getMainScreenPtr() : nullptr;
        JUTTexture* tex = mainPic ? mainPic->getTexture(0) : nullptr;
        const ResTIMG* texInfo = tex ? tex->getTexInfo() : nullptr;

        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "[ZButtonDiag f=%u] draw=%p scr=%p slot=%d zItem=0x%02X lastLoaded=0x%02X itemR=%p pane=%p (vis=%d alpha=%d parent=%p) mainPic=%p tex=%p texInfo=%p buf0=%p buf1=%p idx=%d",
            s_diagTimer, draw, scr, (int)s_zInventorySlot, zItem, s_lastLoadedZItem,
            itemR, pane, pane ? pane->isVisible() : -1, pane ? pane->getAlpha() : -1, pane ? pane->getParentPane() : nullptr,
            mainPic, tex, texInfo, s_zTexBufMain[0], s_zTexBufMain[1], (int)s_zTexBufIdx);
        s_logSvc->info(s_modCtx, buf);
    }
        }
        if (midnaPane != nullptr) {
            midnaPane->show();
            midnaPane->setAlpha(255);
        }
    }

    safe_pane_resize(itemR, w, h);
    if (shinePic) shinePic->resize(w, h);

    safe_pane_trans(itemR, offsetX, offsetY);

    safe_pane_show(itemR);
    safe_pane_alpha_rate(itemR, 1.0f);
    safe_pane_alpha(itemR, 255);
    pane->show();
    pane->setAlpha(255);
    mainPic->setAlpha(255);

    s_cachedZMainPic = mainPic;
    s_cachedZW = w;
    s_cachedZH = h;
}

DEFINE_HOOK(&mDoCPd_c::read, PadReadHook);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SetActiveCursorHook);
static void* s_ringArgs = nullptr;
DEFINE_HOOK(&dMeter2_c::checkStatus, CheckStatusHook);
DEFINE_HOOK(&dMeter2_c::_execute, Meter2ExecuteHook);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconMidonaAlpha, MidonaAlphaHook);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconAlpha, ButtonIconAlphaHook);
DEFINE_HOOK(&dMeter2Draw_c::changeTextureItemXY, ChangeTextureItemXYHook);
DEFINE_HOOK(&dMeter2_c::moveButtonXY, MoveButtonXYHook);
DEFINE_HOOK(&daAlink_c::orderTalk, OrderTalkHook);
DEFINE_HOOK(&daAlink_c::checkItemSetButton, CheckItemSetButtonHook);
DEFINE_HOOK(&daAlink_c::checkSetItemTrigger, CheckSetItemTriggerHook);
DEFINE_HOOK(&daAlink_c::checkItemButtonChange, CheckItemButtonChangeHook);
DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton, CheckItemChangeFromButtonHook);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger, MidnaTalkTriggerHook);
DEFINE_HOOK(&daAlink_c::setStickData, SetStickDataHook);
DEFINE_HOOK(&dSv_player_status_a_c::setSelectItemIndex, SetSelectItemIndexHook);

DEFINE_HOOK(&dMeter2Draw_c::drawButtonZ, DrawButtonZHook);

static void set_pane_influenced_alpha_recursive(J2DPane* pane, bool influenced) {
    if (!pane) return;
    pane->setInfluencedAlpha(influenced, true);
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr; child = child->getNextChildPane()) {
        set_pane_influenced_alpha_recursive(child, influenced);
    }
}

// Creates the three digit picture nodes under zbtn_n so they inherit its alpha.
// Also reparents r_itm_p here so the item icon fades with the Z button.
// Called lazily on first draw and re-runs whenever zbtn_n is recreated.
static void init_z_hud_nodes(dMeter2Draw_c* draw) {
    if (!draw) return;

    J2DScreen* screen = draw->getMainScreenPtr();
    if (!screen) return;

    J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
    if (!zbtn) return;

    if (s_zDigitParent != zbtn) {
        s_zDigitsInitialized = false;
        s_zDigitParent = nullptr;
        s_zDigitPic[0] = nullptr;
        s_zDigitPic[1] = nullptr;
        s_zDigitPic[2] = nullptr;
        s_lastLoadedZItem = 0xFF;
        s_cachedZMainPic = nullptr;
    }

    if (s_zDigitsInitialized) return;

    JKRArchive* arc = dComIfGp_getMain2DArchive();
    if (!arc) return;

    ResTIMG* timg = reinterpret_cast<ResTIMG*>(arc->getResource('TIMG', dMeter2Info_getNumberTextureName(0)));
    if (!timg) return;

    for (int i = 0; i < 3; i++) {
        s_zDigitPic[i] = JKR_NEW J2DPicture(timg);
        if (!s_zDigitPic[i]) return;
        s_zDigitPic[i]->hide();
        zbtn->appendChild(s_zDigitPic[i]);
        set_pane_influenced_alpha_recursive(s_zDigitPic[i], true);
    }

    s_zDigitParent = zbtn;
    s_zDigitsInitialized = true;

    // Move r_itm_p under zbtn_n so its alpha is inherited from zbtn_n automatically
    CPaneMgr* itemRMgr = dMeter2Info_getMeterItemPanePtr(2);
    if (itemRMgr && itemRMgr->getPanePtr()) {
        J2DPane* itemPane = itemRMgr->getPanePtr();
        zbtn->appendChild(itemPane);
        set_pane_influenced_alpha_recursive(itemPane, true);
    }
}

static void on_draw_button_z_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw) {
        return;
    }

    init_z_hud_nodes(draw);

    if (isWolfPlayer()) {
        CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
        safe_pane_hide(itemR);
        for (int i = 0; i < 3; i++) {
            if (s_zDigitPic[i]) {
                s_zDigitPic[i]->hide();
            }
        }
        J2DPane* textPane = draw->getMainScreenPtr()->search(MULTI_CHAR('r_text_n'));
        if (textPane != nullptr) {
            textPane->hide();
        }
        return;
    }

    ensure_z_slot_initialized();
    u8 zItem = (s_zInventorySlot != 0xFF) ? dComIfGs_getItem(s_zInventorySlot, false) : 0xFF;
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        u8 zItem1 = dComIfGp_getSelectItem(2);
        if (zItem1 != 0xFF && zItem1 != 0x00 && zItem1 != dItemNo_NONE_e) { zItem = zItem1; }
    }

    if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e) {
        update_z_item_texture(draw);

        J2DPane* textPane = draw->getMainScreenPtr()->search(MULTI_CHAR('r_text_n'));
        if (textPane != nullptr) {
            textPane->hide();
        }
    }
}

DEFINE_HOOK(&dMeter2Draw_c::setAlphaButtonChange, SetAlphaButtonChangeHook);

static void on_set_alpha_button_change_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw) {
        return;
    }

    u8 zItem = dItemNo_NONE_e;
    if (s_zInventorySlot != 0xFF) {
        zItem = dComIfGs_getItem(s_zInventorySlot, false);
    }
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        u8 zItem1 = dComIfGp_getSelectItem(2);
        if (zItem1 != 0xFF && zItem1 != 0x00 && zItem1 != dItemNo_NONE_e) {
            zItem = zItem1;
        }
    }
}

static void update_ring_z_slots(dMenu_Ring_c* ring) {
    if (!ring) return;
    u8 zSlot = s_zInventorySlot;
    if (zSlot == 0xFF) {
        zSlot = find_slot_for_item(dComIfGs_getSelectItemIndex(2));
        s_zInventorySlot = zSlot;
    }
    u8 zItemNo = (zSlot != 0xFF) ? dComIfGs_getItem(zSlot, false) : 0xFF;
    get_ring_field_0x6b4(ring)[2] = zItemNo;
    get_ring_z_button_slot(ring) = get_ring_slot_for_item(ring, zSlot);
}

static bool s_inSetSelectItemIndex = false;

static HookAction on_set_select_item_index_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || s_inSetSelectItemIndex) {
        return HOOK_CONTINUE;
    }

    struct Guard {
        Guard() { s_inSetSelectItemIndex = true; }
        ~Guard() { s_inSetSelectItemIndex = false; }
    } guard;

    dSv_player_status_a_c* status = mods::arg<dSv_player_status_a_c*>(args, 0);
    int i_no = mods::arg<int>(args, 1);
    u8 i_slotNo = mods::arg<u8>(args, 2);

    if (status == nullptr) {
        return HOOK_CONTINUE;
    }

    u8 targetSlot = find_slot_for_item(i_slotNo);

    if (i_no == 2) {
        if (s_pendingZSlot != 0xFF && s_zInventorySlot != 0xFF) {
            status->mSelectItem[2] = s_zInventorySlot;
        } else if (s_zInventorySlot != 0xFF) {
            status->mSelectItem[2] = s_zInventorySlot;
        } else if (targetSlot != 0xFF) {
            s_zInventorySlot = targetSlot;
            status->mSelectItem[2] = targetSlot;
        }
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

        u8 currentZSlot = s_zInventorySlot;
        if (currentZSlot == 0xFF) {
            currentZSlot = find_slot_for_item(status->mSelectItem[2]);
            s_zInventorySlot = currentZSlot;
        }

        int otherButtonIdx = 1 - i_no;
        u8 otherSlot = status->mSelectItem[otherButtonIdx];

        if (currentZSlot != 0xFF && targetSlot == currentZSlot) {
            u8 oldXYSlot = currentSlot;
            u8 newZSlot = (oldXYSlot < 24) ? oldXYSlot : find_slot_for_item(oldXYSlot);

            s_zInventorySlot = newZSlot;
            status->mSelectItem[2] = (newZSlot != 0xFF) ? newZSlot : 0xFF;

            if (s_logSvc && s_modCtx) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "[DpadMidna] SWAP (%s<->Z): %s got slot %d (0x%02X) <-> Z got slot %d (0x%02X)",
                    (i_no == 0) ? "X" : "Y", (i_no == 0) ? "X" : "Y",
                    targetSlot, (targetSlot < 24) ? dComIfGs_getItem(targetSlot, false) : 0xFF,
                    newZSlot, (newZSlot < 24) ? dComIfGs_getItem(newZSlot, false) : 0xFF);
                s_logSvc->info(s_modCtx, buf);
            }

            if (s_ringArgs) {
                dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(s_ringArgs, 0);
                if (ring) {
                    update_ring_z_slots(ring);
                }
            }
        }
        else if (otherSlot != 0xFF && targetSlot == otherSlot) {
            u8 oldSlot = currentSlot;
            status->mSelectItem[otherButtonIdx] = oldSlot;

            if (s_logSvc && s_modCtx) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "[DpadMidna] SWAP (X<->Y): %s got slot %d (0x%02X) <-> %s got slot %d (0x%02X)",
                    (i_no == 0) ? "X" : "Y", targetSlot, (targetSlot < 24) ? dComIfGs_getItem(targetSlot, false) : 0xFF,
                    (otherButtonIdx == 0) ? "X" : "Y", oldSlot, (oldSlot < 24) ? dComIfGs_getItem(oldSlot, false) : 0xFF);
                s_logSvc->info(s_modCtx, buf);
            }

            if (s_ringArgs) {
                dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(s_ringArgs, 0);
                if (ring) {
                    if (otherButtonIdx == 0) {
                        get_ring_x_button_slot(ring) = get_ring_slot_for_item(ring, oldSlot);
                        get_ring_field_0x6b4(ring)[0] = (oldSlot < 24) ? dComIfGs_getItem(oldSlot, false) : 0xFF;
                    } else {
                        get_ring_y_button_slot(ring) = get_ring_slot_for_item(ring, oldSlot);
                        get_ring_field_0x6b4(ring)[1] = (oldSlot < 24) ? dComIfGs_getItem(oldSlot, false) : 0xFF;
                    }
                }
            }
        }
        else {
            if (s_logSvc && s_modCtx) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "[DpadMidna] ASSIGN (%s): %s assigned slot %d (0x%02X) | State: X=%d Y=%d Z=%d",
                    (i_no == 0) ? "X" : "Y", (i_no == 0) ? "X" : "Y",
                    targetSlot, (targetSlot < 24) ? dComIfGs_getItem(targetSlot, false) : 0xFF,
                    (i_no == 0) ? targetSlot : status->mSelectItem[0],
                    (i_no == 1) ? targetSlot : status->mSelectItem[1],
                    s_zInventorySlot);
                s_logSvc->info(s_modCtx, buf);
            }
        }

        status->mSelectItem[i_no] = targetSlot;

        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
}

static HookAction on_midna_talk_trigger_pre(ModContext*, void* args, void* ret, void*) {
    if (!g_configCustomZButtonEnabled || !args || !ret) {
        return HOOK_CONTINUE;
    }

    u8 windowStatus = dMeter2Info_getWindowStatus();
    bool isMenuOrPause = (windowStatus != 0) || dComIfGp_isPauseFlag() || dScnPly_c::isPause();
    if (isMenuOrPause) {
        return HOOK_CONTINUE;
    }

    BOOL& r = *reinterpret_cast<BOOL*>(ret);
    r = s_dpadLeftTrig ? 1 : 0;
    return HOOK_SKIP_ORIGINAL;
}

static HookAction on_check_item_change_from_button_pre(ModContext*, void* args, void* ret, void*) {
    if (!g_configCustomZButtonEnabled || !args || !ret) {
        return HOOK_CONTINUE;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    int& r = *reinterpret_cast<int*>(ret);

    if (alink != nullptr && !alink->checkWolf()) {
        u8 zItem = dComIfGp_getSelectItem(2);
        if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e || is_bomb_item(zItem)) {
            if (s_zInventorySlot != 0xFF) {
                u8 slotItem = dComIfGs_getItem(s_zInventorySlot, false);
                if (slotItem != 0xFF && slotItem != 0x00 && slotItem != dItemNo_NONE_e) {
                    zItem = slotItem;
                }
            }
        }

        if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e && s_zInventorySlot != 0xFF) {
            u8 oldGpItem = dComIfGp_getSelectItem(2);
            g_dComIfG_gameInfo.play.setSelectItem(2, zItem);

            int proc_type = alink->checkNewItemChange(2);
            if (proc_type != 0 && alink->itemTriggerCheck(1 << 2)) {
                r = alink->changeItemTriggerKeepProc(2, proc_type);
                g_dComIfG_gameInfo.play.setSelectItem(2, oldGpItem);
                return HOOK_SKIP_ORIGINAL;
            }

            g_dComIfG_gameInfo.play.setSelectItem(2, oldGpItem);
        }
    }

    return HOOK_CONTINUE;
}

static void on_set_stick_data_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink != nullptr && !alink->checkWolf()) {
        u8 windowStatus = dMeter2Info_getWindowStatus();
        if (windowStatus == 0) {
            JUTGamePad* rawGamePad = JUTGamePad::getGamePad(0);
            if (rawGamePad != nullptr) {
                bool physZHeld = (rawGamePad->getButton() & PAD_TRIGGER_Z) != 0;
                bool physZTrig = (rawGamePad->getTrigger() & PAD_TRIGGER_Z) != 0;

                u8 zItem = dComIfGp_getSelectItem(2);
                if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e || is_bomb_item(zItem)) {
                    if (s_zInventorySlot != 0xFF) {
                        u8 slotItem = dComIfGs_getItem(s_zInventorySlot, false);
                        if (slotItem != 0xFF && slotItem != 0x00 && slotItem != dItemNo_NONE_e) {
                            zItem = slotItem;
                        }
                    }
                }
                if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e && s_zInventorySlot != 0xFF) {
                    if (physZHeld) {
                        alink->mItemButton |= 0x04;
                    }
                    if (physZTrig) {
                        alink->mItemTrigger |= 0x04;
                    }
                }
            }
        }
    }
}

static HookAction on_check_item_button_change_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return HOOK_CONTINUE;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink != nullptr && alink->mEquipItem != dItemNo_NONE_e && !alink->checkEquipAnime()) {
        for (u8 i = 0; i < 3; i++) {
            u8 selItem = dComIfGp_getSelectItem(i);
            if (selItem == 0xFF || selItem == 0x00 || selItem == dItemNo_NONE_e || is_bomb_item(selItem)) {
                if (i == 2 && s_zInventorySlot != 0xFF) {
                    u8 slotItem = dComIfGs_getItem(s_zInventorySlot, false);
                    if (slotItem != 0xFF && slotItem != 0x00 && slotItem != dItemNo_NONE_e) {
                        selItem = slotItem;
                    }
                }
            }
            if (alink->mEquipItem == selItem || (is_bomb_item(alink->mEquipItem) && is_bomb_item(selItem))) {
                alink->mSelectItemId = i;
                break;
            }
        }
        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
}

static HookAction on_check_item_set_button_pre(ModContext*, void* args, void* ret, void*) {
    if (!g_configCustomZButtonEnabled || !args || !ret) {
        return HOOK_CONTINUE;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    int itemNo = mods::arg<int>(args, 1);
    int& r = *reinterpret_cast<int*>(ret);

    if (alink != nullptr) {
        for (u8 i = 0; i < 3; i++) {
            u8 selItem = dComIfGp_getSelectItem(i);
            if (selItem == 0xFF || selItem == 0x00 || selItem == dItemNo_NONE_e || is_bomb_item(selItem)) {
                if (i == 2 && s_zInventorySlot != 0xFF) {
                    u8 slotItem = dComIfGs_getItem(s_zInventorySlot, false);
                    if (slotItem != 0xFF && slotItem != 0x00 && slotItem != dItemNo_NONE_e) {
                        selItem = slotItem;
                    }
                }
            }
            if (selItem != 0xFF && selItem != 0x00 && (alink->checkGroupItem(itemNo, selItem) || (is_bomb_item(itemNo) && is_bomb_item(selItem)))) {
                if (i == 2) {
                    r = 0;
                } else {
                    r = (int)i;
                }
                return HOOK_SKIP_ORIGINAL;
            }
        }
    }

    r = 2;
    return HOOK_SKIP_ORIGINAL;
}

static HookAction on_check_set_item_trigger_pre(ModContext*, void* args, void* ret, void*) {
    if (!g_configCustomZButtonEnabled || !args || !ret) {
        return HOOK_CONTINUE;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    int itemNo = mods::arg<int>(args, 1);
    int& r = *reinterpret_cast<int*>(ret);

    if (alink != nullptr) {
        for (u8 i = 0; i < 3; i++) {
            u8 selItem = dComIfGp_getSelectItem(i);
            if (selItem == 0xFF || selItem == 0x00 || selItem == dItemNo_NONE_e || is_bomb_item(selItem)) {
                if (i == 2 && s_zInventorySlot != 0xFF) {
                    u8 slotItem = dComIfGs_getItem(s_zInventorySlot, false);
                    if (slotItem != 0xFF && slotItem != 0x00 && slotItem != dItemNo_NONE_e) {
                        selItem = slotItem;
                    }
                }
            }
            if (selItem != 0xFF && selItem != 0x00 && (alink->checkGroupItem(itemNo, selItem) || (is_bomb_item(itemNo) && is_bomb_item(selItem))) && alink->itemTriggerCheck(1 << i)) {
                if (itemNo != dItemNo_HVY_BOOTS_e) {
                    alink->mSelectItemId = i;
                }
                r = 1;
                return HOOK_SKIP_ORIGINAL;
            }
        }
    }

    r = 0;
    return HOOK_SKIP_ORIGINAL;
}


static void on_pad_read_post(ModContext*, void*, void*, void*) {
    if (!g_configCustomZButtonEnabled || isTitleOrMainMenu()) {
        return;
    }

    interface_of_controller_pad& pad = mDoCPd_c::getCpadInfo(PAD_1);

    u8 windowStatus = dMeter2Info_getWindowStatus();
    bool isMenuOrPause = (windowStatus != 0) || dComIfGp_isPauseFlag() || dScnPly_c::isPause();

    if (isMenuOrPause) {
        // In menus D-Pad Left works normally for UI navigation
        s_dpadLeftHeld = false;
        s_dpadLeftTrig = false;
    } else {
        s_dpadLeftHeld = (pad.mButtonFlags & PAD_BUTTON_LEFT) != 0;
        s_dpadLeftTrig = (pad.mPressedButtonFlags & PAD_BUTTON_LEFT) != 0;

        // Strip D-Pad Left during gameplay so it doesn't toggle the map
        if (s_dpadLeftHeld) pad.mButtonFlags &= ~PAD_BUTTON_LEFT;
        if (s_dpadLeftTrig) pad.mPressedButtonFlags &= ~PAD_BUTTON_LEFT;
    }

    JUTGamePad* rawGamePad = JUTGamePad::getGamePad(0);
    bool physZHeld = false;
    bool physZTrig = false;
    if (rawGamePad != nullptr) {
        physZHeld = (rawGamePad->getButton() & PAD_TRIGGER_Z) != 0;
        physZTrig = (rawGamePad->getTrigger() & PAD_TRIGGER_Z) != 0;
    }

    // Always strip Z so D-Pad Left can't accidentally fire Z-item usage
    pad.mButtonFlags &= ~PAD_TRIGGER_Z;
    pad.mPressedButtonFlags &= ~PAD_TRIGGER_Z;

    // Re-inject Z only when physical R1 is pressed during gameplay
    if (!isMenuOrPause && !s_dpadLeftHeld && !s_dpadLeftTrig && (physZHeld || physZTrig)) {
        if (physZHeld) pad.mButtonFlags |= PAD_TRIGGER_Z;
        if (physZTrig) pad.mPressedButtonFlags |= PAD_TRIGGER_Z;

        if (s_zInventorySlot != 0xFF) {
            u8 zItem = dComIfGs_getItem(s_zInventorySlot, false);
            if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e && zItem != 0x72) {
                daAlink_c* alink = static_cast<daAlink_c*>(daPy_getLinkPlayerActorClass());
                if (alink != nullptr && !alink->checkWolf() && dComIfGs_getLife() > 0) {
                    alink->mSelectItemId = 2;

                    if (physZTrig && s_logSvc && s_modCtx) {
                        char buf[256];
                        std::snprintf(buf, sizeof(buf), "[CustomZButton] Z-Button Pressed! Interacting with Item: 0x%02X (Slot=%d)", zItem, s_zInventorySlot);
                        s_logSvc->info(s_modCtx, buf);
                    }
                }
            }
        }
    }
}

static HookAction on_set_active_cursor_pre(ModContext*, void* args, void*, void*) {
    s_ringArgs = args;
    if (g_configCustomZButtonEnabled && args) {
        dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
        if (ring) {
            update_ring_z_slots(ring);
        }
    }
    return HOOK_CONTINUE;
}

static void on_set_active_cursor_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return;
    }

    dMenu_Ring_c* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!ring) {
        return;
    }

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

    u8 hoveredItemNo = get_ring_current_slot_item(ring);
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

    u8 oldZSlot = s_zInventorySlot;

    if (oldZSlot == 0xFF) {
        oldZSlot = find_slot_for_item(dComIfGs_getSelectItemIndex(2));
        s_zInventorySlot = oldZSlot;
    }

    // If this item is already on Z, just play the error sound
    u8 oldZItemNo = (oldZSlot != 0xFF) ? dComIfGs_getItem(oldZSlot, false) : 0xFF;
    if (realSlot == oldZSlot || (oldZItemNo != 0xFF && itemNo == oldZItemNo)) {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }

    trigger_ring_item_slide_z(ring, itemNo);

    u8* pField0x6b4 = reinterpret_cast<u8*>(reinterpret_cast<uintptr_t>(ring) + 0x6B4);
    pField0x6b4[2]  = itemNo;

    u8 xVal = dComIfGs_getSelectItemIndex(0);
    u8 yVal = dComIfGs_getSelectItemIndex(1);

    u8 xItemNo = (xVal < 24) ? dComIfGs_getItem(xVal, false) : xVal;
    u8 yItemNo = (yVal < 24) ? dComIfGs_getItem(yVal, false) : yVal;

    bool xMatch = (xVal == realSlot) || (xItemNo != 0xFF && xItemNo == itemNo);
    bool yMatch = (yVal == realSlot) || (yItemNo != 0xFF && yItemNo == itemNo);

    u8 itemToMoveToXY = (oldZSlot != 0xFF) ? dComIfGs_getItem(oldZSlot, false) : 0xFF;

    if (s_logSvc && s_modCtx) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[DpadMidna] Z Equip PRE: realSlot=%d itemNo=0x%02X | xVal=0x%02X yVal=0x%02X oldZSlot=%d xMatch=%d yMatch=%d moveItem=0x%02X",
            realSlot, itemNo, xVal, yVal, oldZSlot, xMatch, yMatch, itemToMoveToXY);
        s_logSvc->info(s_modCtx, buf);
    }

    // If the item being assigned to Z is currently on X or Y, swap them
    if (xMatch) {
        dComIfGs_setSelectItemIndex(0, (oldZSlot != 0xFF) ? oldZSlot : 0xFF);
        get_ring_x_button_slot(ring) = get_ring_slot_for_item(ring, oldZSlot);
        get_ring_field_0x6b4(ring)[0] = itemToMoveToXY;
    } else if (yMatch) {
        dComIfGs_setSelectItemIndex(1, (oldZSlot != 0xFF) ? oldZSlot : 0xFF);
        get_ring_y_button_slot(ring) = get_ring_slot_for_item(ring, oldZSlot);
        get_ring_field_0x6b4(ring)[1] = itemToMoveToXY;
    }

    s_pendingZSlot = realSlot;
    s_zInventorySlot = realSlot;
    sync_z_item_state();

    trigger_ring_item_slide_z(ring, itemNo);
    update_ring_z_slots(ring);

    dMeter2Info_set2DVibrationM();
    Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);

    if (s_logSvc && s_modCtx) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[DpadMidna] Z Equip POST (Pending Slide): xSlot=%d ySlot=%d s_zPendingSlot=%d",
            dComIfGs_getSelectItemIndex(0), dComIfGs_getSelectItemIndex(1), s_pendingZSlot);
        s_logSvc->info(s_modCtx, buf);
    }
}


static void on_check_status_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    // Clear 0x1000000 during the Ring Menu only
    u8 windowStatus = dMeter2Info_getWindowStatus();
    dMeter2_c* meter = *reinterpret_cast<dMeter2_c**>(args);
    if (meter != nullptr && windowStatus == 2) {
        u32* pStatus = reinterpret_cast<u32*>(reinterpret_cast<uintptr_t>(meter) + 0x124);
        *pStatus &= ~0x1000000u;
    }

    commit_pending_z_slot();
    update_z_item_texture();
}

static void on_meter2_execute_post(ModContext*, void*, void*, void*) {
    if (!g_configCustomZButtonEnabled || isTitleOrMainMenu()) {
        return;
    }
    if (is_pause_menu_open()) {
        CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
        safe_pane_hide(itemR);
        return;
    }
    g_meter2_info.onUseButton(0x800);
    if (!isWolfPlayer()) {
        update_z_item_texture();
    }
}

static HookAction on_set_button_icon_midona_alpha_pre(ModContext*, void* args, void*, void*) {
    if (!args) {
        return HOOK_CONTINUE;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!g_configCustomZButtonEnabled) {
        if (draw != nullptr && draw->getMainScreenPtr() != nullptr) {
            J2DScreen* screen = draw->getMainScreenPtr();
            J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
            if (itemRPane != nullptr) itemRPane->hide();
            J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));
            if (itemRChild != nullptr) itemRChild->hide();
        }
        return HOOK_CONTINUE;
    }

    u32& param0 = mods::arg_ref<u32>(args, 1);
    param0 &= ~0x1000000u;

    if (draw != nullptr) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen != nullptr) {
            if (is_pause_menu_open(draw)) {
                J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
                if (itemRPane != nullptr) itemRPane->hide();
                J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));
                if (itemRChild != nullptr) itemRChild->hide();
                J2DPane* textPane = screen->search(MULTI_CHAR('r_text_n'));
                if (textPane != nullptr) textPane->hide();
                return HOOK_CONTINUE;
            }
            J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
            J2DPane* jujiPane = screen->search(MULTI_CHAR('juji_n'));

            if (jujiPane != nullptr) {
                f32 jujiY = jujiPane->getGlbBounds().i.y - 5.0f;

                // Larger minimap needs a bit more offset
                if (dMeter2Info_getMapStatus() == 1) {
                    jujiY -= 5.0f;
                }

                g_drawHIO.mMidnaIconPosY = jujiY;
                g_drawHIO.mEmpButton.mMidnaIconPosY = jujiY;
                if (midnaPane != nullptr) {
                    midnaPane->translate(midnaPane->getTranslateX(), jujiY);
                }
            }

            u8 crossAlpha = 255;
            if (jujiPane != nullptr) {
                crossAlpha = jujiPane->getAlpha();
            } else {
                J2DPane* contPane = screen->search(MULTI_CHAR('cont_n'));
                if (contPane != nullptr) {
                    crossAlpha = contPane->getAlpha();
                }
            }

            if (midnaPane != nullptr) {
                midnaPane->setAlpha(crossAlpha);
                if (crossAlpha == 0) {
                    midnaPane->hide();
                } else {
                    midnaPane->show();
                }

                JSUTreeIterator<J2DPane> it(midnaPane->getFirstChild());
                while (it != midnaPane->getEndChild()) {
                    if (crossAlpha == 0) {
                        it->hide();
                    } else {
                        it->show();
                    }
                    it->setAlpha(crossAlpha);
                    ++it;
                }
            }

            J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
            if (zbtn != nullptr) {
                zbtn->show();
                zbtn->setAlpha(255);
            }
            J2DPane* rbtn = screen->search(MULTI_CHAR('rbtn_n'));
            if (rbtn != nullptr) {
                rbtn->show();
                rbtn->setAlpha(255);
            }

            u8 zItem = dComIfGp_getSelectItem(2);
            if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
                if (s_zInventorySlot != 0xFF) {
                    zItem = dComIfGs_getItem(s_zInventorySlot, false);
                }
            }

            J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
            J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));

            if (isWolfPlayer()) {
                if (itemRPane != nullptr) itemRPane->hide();
                if (itemRChild != nullptr) itemRChild->hide();
                J2DPane* textPane = screen->search(MULTI_CHAR('r_text_n'));
                if (textPane != nullptr) textPane->hide();
            } else if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e) {
                if (itemRPane != nullptr) {
                    itemRPane->show();
                    itemRPane->setAlpha(255);
                }
                if (itemRChild != nullptr) {
                    itemRChild->show();
                    itemRChild->setAlpha(255);
                }
            }
        }
    }

    return HOOK_CONTINUE;
}

static HookAction on_set_button_icon_alpha_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return HOOK_CONTINUE;
    }

    int i_no = mods::arg<int>(args, 1);
    if (i_no != 2) {
        return HOOK_CONTINUE;
    }

    g_meter2_info.onUseButton(0x800);

    return HOOK_CONTINUE;
}

// changeTextureItemXY only supports indices 0 (X) and 1 (Y).
// Calling it with i_no==2 reads mpItemR and mpBTextA via out-of-bounds array access,
// which corrupts the Z button pane. We skip the original and handle it ourselves.
static HookAction on_change_texture_item_xy_pre(ModContext*, void* args, void*, void*) {
    if (!args) {
        return HOOK_CONTINUE;
    }

    int i_no = mods::arg<int>(args, 1);
    if (i_no != 2) {
        return HOOK_CONTINUE;
    }

    if (isWolfPlayer()) {
        CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
        safe_pane_hide(itemR);
        return HOOK_SKIP_ORIGINAL;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    update_z_item_texture(draw);
    return HOOK_SKIP_ORIGINAL;
}

static void on_move_button_xy_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return;
    }

    dMeter2_c* meter2 = mods::arg<dMeter2_c*>(args, 0);
    if (!meter2) {
        return;
    }

    dMeter2Draw_c* draw = meter2->getMeterDrawPtr();
    if (!draw) {
        return;
    }

    u8 zSlot = s_zInventorySlot;

    if (zSlot == 0xFF) {
        u8 zItemNo = dComIfGs_getSelectItemIndex(2);
        if (zItemNo != 0xFF && zItemNo != 0x00 && zItemNo != dItemNo_NONE_e) {
            for (int si = 0; si < 24; si++) {
                u8 lineupSlot = dComIfGs_getLineUpItem(si);
                if (lineupSlot == 0xFF) break;
                if (dComIfGs_getItem(lineupSlot, false) == zItemNo) {
                    zSlot = lineupSlot;
                    s_zInventorySlot = lineupSlot;
                    break;
                }
            }
        }
    }

    if (zSlot != 0xFF) {
        u8 zItem = dComIfGs_getItem(zSlot, false);

        if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e) {
            update_z_item_texture(draw);
            draw->changeTextureItemXY(2, zItem);
            draw->setItemParamZ(zItem);

            if (s_logSvc && s_modCtx) {
                static u8 s_lastLoggedSlot = 0xFF;
                static u8 s_lastLoggedItem = 0xFF;
                if ((s_lastLoggedSlot != zSlot || s_lastLoggedItem != zItem) && zItem != 0x01 && zItem != 0x09) {
                    s_lastLoggedSlot = zSlot;
                    s_lastLoggedItem = zItem;
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[DpadMidna] SavedZ Slot=%d (Item=0x%02X)", zSlot, zItem);
                    s_logSvc->info(s_modCtx, buf);
                }
            }
        }
    }
}

static void on_order_talk_post(ModContext*, void* args, void* retval, void*) {
    if (!g_configCustomZButtonEnabled || !args || !retval) {
        return;
    }

    int* result = reinterpret_cast<int*>(retval);
    if (*result != 0) {
        return;
    }

    daAlink_c* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr || alink->checkWolf()) {
        return;
    }

    u8 zItem = dComIfGp_getSelectItem(2);
    if (zItem == dItemNo_NONE_e || zItem == 0x00 || zItem == 0xFF) {
        return;
    }

    dAttList_c* attList2 = *reinterpret_cast<dAttList_c**>(reinterpret_cast<uintptr_t>(alink) + 0x27E8);
    fopAc_ac_c* targetActor = *reinterpret_cast<fopAc_ac_c**>(reinterpret_cast<uintptr_t>(alink) + 0x27F8);

    if (daPy_py_c::checkTradeItem(zItem) && alink->itemTriggerCheck(0x04) && attList2 != nullptr && targetActor != nullptr) {
        if (alink->checkRequestTalkActor(attList2, targetActor)) {
            fopAcM_orderTalkItemBtnEvent(8, alink, targetActor, 0, 0);
            *result = 1;

            if (s_logSvc && s_modCtx) {
                s_logSvc->info(s_modCtx, "[DpadMidna] Z button trade item talk triggered with NPC");
            }
        }
    }
}

static void draw_item_count_digits(int num, int maxNum, f32 baseX, f32 baseY, f32 iconW, f32 iconH, f32 alphaRate) {
    if (num < 0 || alphaRate <= 0.0f) return;
    if (num > 999) num = 999;

    // s_drawDigitPic is a module-level static; reset by shutdown_custom_z_button on soft-reset
    if (s_drawDigitPic[0] == nullptr) {
        for (int i = 0; i < 3; i++) {
            s_drawDigitPic[i] = JKR_NEW J2DPicture();
        }
    }

    JKRArchive* arc = dComIfGp_getMain2DArchive();
    if (!arc) return;

    JUtility::TColor black(0, 0, 0, 0);
    JUtility::TColor white(255, 255, 255, 255);
    if (maxNum > 0 && num == maxNum) {
        black.set(30, 30, 30, 0);
        white.set(255, 200, 50, 255);
    } else if (num == 0) {
        black.set(30, 30, 30, 0);
        white.set(180, 180, 180, 255);
    }

    for (int i = 0; i < 3; i++) {
        s_drawDigitPic[i]->setBlackWhite(black, white);
        s_drawDigitPic[i]->setAlpha((u8)(alphaRate * 255.0f));
    }

    f32 digitW = 14.0f;
    f32 digitH = 14.0f;
    f32 startX = baseX - 5.0f;
    f32 startY = baseY + iconH - digitH + 2.0f - 10.0f;

    if (num < 10) {
        ResTIMG* timg = (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(num));
        if (timg) {
            s_drawDigitPic[0]->changeTexture(timg, 0);
            s_drawDigitPic[0]->show();
            s_drawDigitPic[0]->draw(startX + iconW - digitW, startY, digitW, digitH, false, false, false);
        }
    } else if (num < 100) {
        int tens = num / 10;
        int units = num % 10;

        ResTIMG* t1 = (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(tens));
        ResTIMG* t2 = (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(units));

        if (t1 && t2) {
            s_drawDigitPic[0]->changeTexture(t1, 0);
            s_drawDigitPic[0]->show();
            s_drawDigitPic[0]->draw(startX + iconW - (digitW * 1.8f), startY, digitW, digitH, false, false, false);

            s_drawDigitPic[1]->changeTexture(t2, 0);
            s_drawDigitPic[1]->show();
            s_drawDigitPic[1]->draw(startX + iconW - (digitW * 0.9f), startY, digitW, digitH, false, false, false);
        }
    } else {
        int hundreds = num / 100;
        int tens = (num / 10) % 10;
        int units = num % 10;

        ResTIMG* t1 = (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(hundreds));
        ResTIMG* t2 = (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(tens));
        ResTIMG* t3 = (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(units));

        if (t1 && t2 && t3) {
            s_drawDigitPic[0]->changeTexture(t1, 0);
            s_drawDigitPic[0]->draw(startX + iconW - (digitW * 2.7f), startY, digitW, digitH, false, false, false);
            s_drawDigitPic[1]->changeTexture(t2, 0);
            s_drawDigitPic[1]->draw(startX + iconW - (digitW * 1.8f), startY, digitW, digitH, false, false, false);
            s_drawDigitPic[2]->changeTexture(t3, 0);
            s_drawDigitPic[2]->draw(startX + iconW - (digitW * 0.9f), startY, digitW, digitH, false, false, false);
        }
    }
}

DEFINE_HOOK(&dMeter2Draw_c::draw, Meter2DrawDrawHook);

static void on_meter2_draw_draw_post(ModContext*, void* args, void*, void*) {
    if (!args) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw || !draw->getMainScreenPtr()) {
        return;
    }

    if (!g_configCustomZButtonEnabled || isWolfPlayer() || isTitleOrMainMenu()) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen != nullptr) {
            J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
            if (itemRPane != nullptr) itemRPane->hide();
            J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));
            if (itemRChild != nullptr) itemRChild->hide();
            J2DPane* textPane = screen->search(MULTI_CHAR('r_text_n'));
            if (textPane != nullptr) textPane->hide();
        }
        return;
    }

    J2DScreen* screen = draw->getMainScreenPtr();
    J2DPane* contPane = screen->search(MULTI_CHAR('cont_n'));
    J2DPane* zbtnPane = screen->search(MULTI_CHAR('zbtn_n'));



    // Don't draw when the HUD is hidden (pause menu, cutscenes, etc.)
    if (contPane != nullptr && (!contPane->isVisible() || contPane->getAlpha() == 0)) {
        return;
    }
    if (zbtnPane != nullptr && (!zbtnPane->isVisible() || zbtnPane->getAlpha() == 0)) {
        return;
    }

    f32 alphaRate = 1.0f;
    if (contPane != nullptr) {
        alphaRate *= ((f32)contPane->getAlpha() / 255.0f);
    }
    if (zbtnPane != nullptr) {
        alphaRate *= ((f32)zbtnPane->getAlpha() / 255.0f);
    }

    f32 baseX = 575.0f;
    f32 baseY = 35.0f;
    f32 iconW = 42.0f;
    f32 iconH = 42.0f;

    if (zbtnPane != nullptr) {
        const JGeometry::TBox2<f32>& bounds = zbtnPane->getGlbBounds();
        baseX = bounds.i.x;
        baseY = bounds.i.y;
        iconW = bounds.getWidth();
        iconH = bounds.getHeight();
    }

    u8 zItem = dItemNo_NONE_e;
    if (s_zInventorySlot != 0xFF) {
        zItem = dComIfGs_getItem(s_zInventorySlot, false);
    }
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        zItem = dComIfGp_getSelectItem(2);
    }

    if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e) {
        f32 texScale = g_drawHIO.mItemScaleAdjustON
            ? (g_drawHIO.mItemScalePercent / 100.0f)
            : (dItem_data::getTexScale(zItem) / 100.0f);
        f32 baseSize = 42.0f;
        f32 w = texScale * baseSize;
        f32 widthShift = (w / 42.0f) * 24.0f;
        baseX += widthShift;
        baseY += 6.0f;
    }

    int count    = -1;
    int maxCount = -1;

    // Bow and variants
    if (zItem == 0x43 || zItem == 0x53 || zItem == 0x54 || zItem == 0x55 || zItem == 0x56 || zItem == 0x5A) {
        count    = dComIfGs_getArrowNum();
        maxCount = dComIfGs_getArrowMax();
    }
    // Slingshot
    else if (zItem == 0x4B || zItem == 0x76) {
        count    = dComIfGs_getPachinkoNum();
        maxCount = dComIfGs_getPachinkoMax();
    }
    // Bombs
    else if (zItem == 0x50 || zItem == 0x70 || zItem == 0x71 || zItem == 0x72) {
        u8 bombType = zItem;
        u8 bagIdx   = 0;
        for (u8 i = 0; i < 3; i++) {
            u8 itemInBag = dComIfGs_getItem((u8)(i + 15), false);
            if (itemInBag == zItem || (zItem == 0x50 && itemInBag != 0x00 && itemInBag != 0xFF)) {
                bagIdx   = i;
                bombType = itemInBag;
                break;
            }
        }
        if (bombType != 0x70 && bombType != 0x71 && bombType != 0x72) {
            bombType = 0x70;
        }
        count    = dComIfGs_getBombNum(bagIdx);
        maxCount = dComIfGs_getBombMax(bombType);
    }
    // Lantern uses its own oil gauge widget
    // s_zKanteraIcon is a module-level static; reset by shutdown_custom_z_button on soft-reset
    else if (zItem == 0x48) {
        if (s_zKanteraIcon == nullptr) {
            s_zKanteraIcon = JKR_NEW dKantera_icon_c();
        }
        if (s_zKanteraIcon != nullptr) {
            f32 kanteraX = baseX + iconW * 0.5f + 8.0f;
            f32 kanteraY = baseY + iconH - 4.0f;
            s_zKanteraIcon->setPos(kanteraX, kanteraY);
            s_zKanteraIcon->setScale(0.6f, 0.6f);
            s_zKanteraIcon->setNowGauge(dComIfGs_getMaxOil(), dComIfGs_getOil());
            s_zKanteraIcon->setAlphaRate(alphaRate);
            s_zKanteraIcon->drawSelf();
        }
    }

    if (count >= 0) {
        draw_item_count_digits(count, maxCount, baseX, baseY, iconW, iconH, alphaRate);
    }
}

ModResult init_custom_z_button(const HookService* hook_svc, ModError*) {
    if (hook_svc) {
        mods::hook_add_post<PadReadHook>(hook_svc, on_pad_read_post);
        mods::hook_add_pre<SetActiveCursorHook>(hook_svc, on_set_active_cursor_pre);
        mods::hook_add_post<SetActiveCursorHook>(hook_svc, on_set_active_cursor_post);
        mods::hook_add_post<CheckStatusHook>(hook_svc, on_check_status_post);
        mods::hook_add_post<Meter2ExecuteHook>(hook_svc, on_meter2_execute_post);
        mods::hook_add_post<Meter2DrawDrawHook>(hook_svc, on_meter2_draw_draw_post);
        mods::hook_add_pre<MidonaAlphaHook>(hook_svc, on_set_button_icon_midona_alpha_pre);
        mods::hook_add_pre<ButtonIconAlphaHook>(hook_svc, on_set_button_icon_alpha_pre);
        mods::hook_add_pre<ChangeTextureItemXYHook>(hook_svc, on_change_texture_item_xy_pre);
        mods::hook_add_post<MoveButtonXYHook>(hook_svc, on_move_button_xy_post);
        mods::hook_add_post<OrderTalkHook>(hook_svc, on_order_talk_post);
        mods::hook_add_pre<CheckItemSetButtonHook>(hook_svc, on_check_item_set_button_pre);
        mods::hook_add_pre<CheckSetItemTriggerHook>(hook_svc, on_check_set_item_trigger_pre);
        mods::hook_add_pre<CheckItemButtonChangeHook>(hook_svc, on_check_item_button_change_pre);
        mods::hook_add_pre<CheckItemChangeFromButtonHook>(hook_svc, on_check_item_change_from_button_pre);
        mods::hook_add_pre<MidnaTalkTriggerHook>(hook_svc, on_midna_talk_trigger_pre);
        mods::hook_add_post<SetStickDataHook>(hook_svc, on_set_stick_data_post);
        mods::hook_add_pre<SetSelectItemIndexHook>(hook_svc, on_set_select_item_index_pre);
        mods::hook_add_post<DrawButtonZHook>(hook_svc, on_draw_button_z_post);
        mods::hook_add_post<SetAlphaButtonChangeHook>(hook_svc, on_set_alpha_button_change_post);
    }
    return MOD_OK;
}

void update_custom_z_button(const LogService* log_svc, ModContext* mod_ctx) {
    s_logSvc = log_svc;
    s_modCtx = mod_ctx;

    if (!g_configCustomZButtonEnabled || isTitleOrMainMenu()) {
        shutdown_custom_z_button();
        return;
    }
        g_meter2_info.onUseButton(0x800);

        g_drawHIO.mParentAlpha = 1.0f;
        g_drawHIO.mMainHUDButtonsAlpha = 1.0f;
        g_drawHIO.mRingHUDButtonsAlpha = 1.0f;
        g_drawHIO.mButtonXYBaseDimAlpha = 255;
        g_drawHIO.mButtonXYItemDimAlpha = 255;

        f32 aspect = mDoGph_gInf_c::getAspect();
        f32 extraWidescreenWidth = (aspect - (16.0f / 9.0f)) * 240.0f;

        f32 posX = -750.0f - extraWidescreenWidth;
        f32 scale = 0.85f;

        g_drawHIO.mMidnaIconPosX = posX;
        g_drawHIO.mMidnaIconScale = scale;
        g_drawHIO.mMidnaIconAlpha = 1.0f;

        g_drawHIO.mEmpButton.mMidnaIconPosX = posX;
        g_drawHIO.mEmpButton.mMidnaIconScale = scale;

        g_drawHIO.mButtonZItemPosX = 0.0f;
        g_drawHIO.mButtonZItemPosY = 0.0f;
        g_drawHIO.mButtonZItemScale = 1.0f;

        if (!isWolfPlayer()) {
            update_z_item_texture();
        }
}

void shutdown_custom_z_button() {
    // Reset all pointers that reference game-heap objects.
    // The game heap is wiped on soft-reset (title screen return), so any cached
    // J2DPicture / J2DPane / dKantera_icon_c pointers become dangling.
    // Nulling them here forces lazy re-initialization on the next game session.

    s_zKanteraIcon = nullptr;

    s_drawDigitPic[0] = nullptr;
    s_drawDigitPic[1] = nullptr;
    s_drawDigitPic[2] = nullptr;

    s_zDigitPic[0] = nullptr;
    s_zDigitPic[1] = nullptr;
    s_zDigitPic[2] = nullptr;
    s_zDigitParent      = nullptr;
    s_zDigitsInitialized = false;

    s_cachedZMainPic    = nullptr;
    s_lastLoadedZItem   = 0xFF;

    s_zInventorySlot = 0xFF;
    s_pendingZSlot   = 0xFF;

    s_ringArgs = nullptr;

    s_dpadLeftHeld = false;
    s_dpadLeftTrig = false;
}
