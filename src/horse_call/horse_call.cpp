#include "horse_call.hpp"

#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_object.h"
#define private public
#include "d/d_meter2_draw.h"
#undef private
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "Z2AudioLib/Z2SeMgr.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

bool g_configDpadHorseCallEnabled = false;
bool g_configDpadHorseCallAllowAnytime = false;
bool g_configDpadHorseCallRequireEquipped = false;

static const LogService* s_logSvc = nullptr;
static ModContext* s_modCtx = nullptr;

static bool s_dpadDownHeld = false;
static bool s_dpadDownTrig = false;

static bool s_actionAvailable = false;

static J2DPicture* s_horseCallPic = nullptr;
static bool s_horseTextureLoaded = false;

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

static bool isHorseCallItem(u8 itemNo) {
    return itemNo == dItemNo_HORSE_FLUTE_e || itemNo == 0x84;
}

static bool isHorseCallEquipped() {
    u8 slot0 = dComIfGp_getSelectItem(0);
    u8 slot1 = dComIfGp_getSelectItem(1);
    u8 slot2 = dComIfGp_getSelectItem(2);

    if (isHorseCallItem(slot0) || isHorseCallItem(slot1) || isHorseCallItem(slot2)) {
        return true;
    }

    u8 zSlot = dComIfGs_getSelectItemIndex(2);
    if (zSlot != 0xFF && zSlot != 0x00 && zSlot < 24) {
        u8 zItem = dComIfGs_getItem(zSlot, false);
        if (isHorseCallItem(zItem)) {
            return true;
        }
    }

    return false;
}

static bool isHorseCallUnlocked() {
    if (g_configDpadHorseCallAllowAnytime) {
        return true;
    }
    for (u8 slot = 0; slot < 24; slot++) {
        u8 item = dComIfGs_getItem(slot, false);
        if (isHorseCallItem(item)) {
            return true;
        }
    }
    if (dComIfGs_isItemFirstBit(dItemNo_HORSE_FLUTE_e)) {
        return true;
    }
    return false;
}

DEFINE_HOOK(&mDoCPd_c::read, PadReadHorseCallHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, Meter2DrawHorseCallHook);

#include "mods/svc/resource.h"

extern const ResourceService* get_resource_service();

static ResourceBuffer s_eponaBtiBuf = RESOURCE_BUFFER_INIT;

static bool load_horse_call_texture() {
    if (s_horseTextureLoaded && s_horseCallPic != nullptr) {
        return true;
    }

    const ResourceService* res_svc = get_resource_service();
    if (res_svc != nullptr && s_modCtx != nullptr) {
        if (s_eponaBtiBuf.data == nullptr) {
            res_svc->load(s_modCtx, "textures/epona_head.bti", &s_eponaBtiBuf);
        }

        if (s_eponaBtiBuf.data != nullptr) {
            ResTIMG* mainBuf = reinterpret_cast<ResTIMG*>(s_eponaBtiBuf.data);
            if (s_horseCallPic == nullptr) {
                s_horseCallPic = JKR_NEW J2DPicture(mainBuf);
            } else {
                s_horseCallPic->changeTexture(mainBuf, 0);
            }
            s_horseTextureLoaded = (s_horseCallPic != nullptr);
            return s_horseTextureLoaded;
        }
    }
    return false;
}

static void on_pad_read_horse_call_post(ModContext*, void*, void*, void*) {
    if (!g_configDpadHorseCallEnabled || isTitleOrMainMenu()) {
        return;
    }

    interface_of_controller_pad& pad = mDoCPd_c::getCpadInfo(PAD_1);

    u8 windowStatus = dMeter2Info_getWindowStatus();
    bool isMenuOrPause = (windowStatus != 0) || dComIfGp_isPauseFlag() || dScnPly_c::isPause()
                         || dComIfGp_event_runCheck() || dMeter2Info_isShopTalkFlag()
                         || dMsgObject_isTalkNowCheck();

    if (isMenuOrPause) {
        s_dpadDownHeld = false;
        s_dpadDownTrig = false;
    } else {
        s_dpadDownHeld = (pad.mButtonFlags & PAD_BUTTON_DOWN) != 0;
        s_dpadDownTrig = (pad.mPressedButtonFlags & PAD_BUTTON_DOWN) != 0;

        if (s_dpadDownHeld) pad.mButtonFlags &= ~PAD_BUTTON_DOWN;
        if (s_dpadDownTrig) pad.mPressedButtonFlags &= ~PAD_BUTTON_DOWN;

        if (s_dpadDownTrig) {
            daAlink_c* alink = static_cast<daAlink_c*>(daPy_getLinkPlayerActorClass());
            bool canCall = isHorseCallUnlocked() && (!g_configDpadHorseCallRequireEquipped || isHorseCallEquipped());
            if (alink != nullptr && s_actionAvailable && canCall) {
                u8 oldGpItem = dComIfGp_getSelectItem(2);
                g_dComIfG_gameInfo.play.setSelectItem(2, dItemNo_HORSE_FLUTE_e);

                int proc_type = alink->checkNewItemChange(2);
                if (proc_type != 0) {
                    alink->changeItemTriggerKeepProc(2, proc_type);
                } else {
                    Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                }

                g_dComIfG_gameInfo.play.setSelectItem(2, oldGpItem);
            } else if (alink != nullptr) {
                Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            }
        }
    }
}

static dMeter2Draw_c* s_lastDraw = nullptr;
static J2DScreen* s_lastScreen = nullptr;

static void on_meter2_draw_horse_call_post(ModContext*, void* args, void*, void*) {
    s_actionAvailable = false;
    if (!args || !g_configDpadHorseCallEnabled || isTitleOrMainMenu()) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw || !draw->getMainScreenPtr() || isWolfPlayer()) {
        return;
    }

    J2DScreen* screen = draw->getMainScreenPtr();
    if (draw != s_lastDraw || screen != s_lastScreen) {
        s_lastDraw = draw;
        s_lastScreen = screen;
        s_horseCallPic = nullptr;
        s_horseTextureLoaded = false;
    }

    if (!isHorseCallUnlocked()) {
        return;
    }

    if (g_configDpadHorseCallRequireEquipped && !isHorseCallEquipped()) {
        return;
    }

    J2DPane* juji = screen->search(MULTI_CHAR('juji_n'));
    if (juji == nullptr || !juji->isVisible() || juji->getAlpha() == 0) {
        return;
    }

    if (!load_horse_call_texture()) {
        return;
    }

    ResTIMG* mainBuf = reinterpret_cast<ResTIMG*>(s_eponaBtiBuf.data);
    if (mainBuf != nullptr) {
        mainBuf->alphaEnabled = 1;
    }

    f32 targetH = 26.0f;
    f32 targetW = 18.0f;
    if (mainBuf != nullptr && mainBuf->width > 0 && mainBuf->height > 0) {
        targetW = targetH * (static_cast<f32>(mainBuf->width) / static_cast<f32>(mainBuf->height));
    }

    const JGeometry::TBox2<f32>& bounds = juji->getGlbBounds();
    f32 drawX = bounds.i.x + (bounds.getWidth() - targetW) * 0.32f;
    f32 drawY = bounds.i.y + 33.0f;

    u8 alpha = juji->getAlpha();

    J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
    if (midnaPane != nullptr && midnaPane->isVisible()) {
        alpha = midnaPane->getAlpha();
    }

    s_actionAvailable = alpha == 255;

    s_horseCallPic->setAlpha(alpha);
    s_horseCallPic->draw(drawX, drawY, targetW, targetH, false, false, false);
}

ModResult init_horse_call(const HookService* hook_svc, ModError*) {
    if (hook_svc) {
        mods::hook::add_post<PadReadHorseCallHook>(hook_svc, on_pad_read_horse_call_post);
        mods::hook::add_post<Meter2DrawHorseCallHook>(hook_svc, on_meter2_draw_horse_call_post);
    }
    return MOD_OK;
}

void update_horse_call(const LogService* log_svc, ModContext* mod_ctx) {
    s_logSvc = log_svc;
    s_modCtx = mod_ctx;

    if (!g_configDpadHorseCallEnabled || isTitleOrMainMenu()) {
        shutdown_horse_call();
        return;
    }
}

void shutdown_horse_call() {
    s_horseCallPic = nullptr;
    s_horseTextureLoaded = false;
    s_dpadDownHeld = false;
    s_dpadDownTrig = false;
    s_actionAvailable = false;
}
