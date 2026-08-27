#pragma once

#include "midna_location.hpp"
#include "z_mobile.hpp"

DEFINE_HOOK(&dMeterButton_c::_execute, MeterButtonExecuteHook);

void update_midna_pane(dMeter2Draw_c* draw) {
    if (draw == nullptr) return;
    J2DScreen* screen = draw->getMainScreenPtr();
    if (screen == nullptr) return;

    J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
    if (midnaPane == nullptr) return;

    // While the mobile fallback borrows `midona_n` to mirror the Z item onto the
    // touch button, leave that pane alone
    if (z_mobile_wants_midona_host()) {
        if (g_configCustomZButtonEnabled) {
            J2DPane* z_btnl_hosted = screen->search(MULTI_CHAR('z_btnl'));
            if (z_btnl_hosted) {
                z_btnl_hosted->hide();
                z_btnl_hosted->setAlpha(0);
            }
        }
        return;
    }

    if (is_pause_menu_open(draw) || !isMidnaUnlocked()) {
        midnaPane->hide();
    } else {
        J2DPane* juji = screen->search(MULTI_CHAR('juji_n'));
        if (midnaPane != nullptr && juji != nullptr) {
            if (midnaPane->getParentPane() != juji) {
                J2DPane* oldParent = midnaPane->getParentPane();
                if (oldParent != nullptr) {
                    oldParent->mPaneTree.removeChild(&midnaPane->mPaneTree);
                }
                juji->appendChild(midnaPane);
                set_pane_influenced_alpha_recursive(midnaPane, true);
            }
            midnaPane->move(-18.0f, 0.0f);
            midnaPane->show();
        }
    }

    if (g_configCustomZButtonEnabled) {
        J2DPane* z_btnl_main = screen->search(MULTI_CHAR('z_btnl'));
        if (z_btnl_main) {
            z_btnl_main->hide();
            z_btnl_main->setAlpha(0);
        }
    }
}

void reset_midna_pane() {
    if (isTitleOrMainMenu()) return;

    dMeter2Draw_c* draw = nullptr;
    if (g_meter2_info.getMeterClass() != nullptr) {
        draw = g_meter2_info.getMeterClass()->getMeterDrawPtr();
    }
    if (draw != nullptr && draw->getMainScreenPtr() != nullptr) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen != nullptr) {
            J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
            J2DPane* contPane = screen->search(MULTI_CHAR('cont_n'));
            if (midnaPane != nullptr) {
                if (contPane != nullptr && midnaPane->getParentPane() != nullptr && midnaPane->getParentPane() != contPane) {
                    J2DPane* oldParent = midnaPane->getParentPane();
                    if (oldParent != nullptr) {
                        oldParent->mPaneTree.removeChild(&midnaPane->mPaneTree);
                    }
                    contPane->appendChild(midnaPane);
                }
                midnaPane->translate(-88.0f, -24.5f);
                midnaPane->show();
            }
        }
    }
}

#include "mods/svc/resource.h"

extern const ResourceService* get_resource_service();

static ResourceBuffer s_dpadLeftBtiBuf = RESOURCE_BUFFER_INIT;

static ResTIMG* get_dpad_left_texture() {
    if (s_dpadLeftBtiBuf.data == nullptr) {
        const ResourceService* res_svc = get_resource_service();
        ModContext* ctx = g_zModCtx;
        if (res_svc != nullptr && ctx != nullptr) {
            res_svc->load(ctx, "textures/dpad_left.bti", &s_dpadLeftBtiBuf);
        }
    }

    if (s_dpadLeftBtiBuf.data != nullptr) {
        return reinterpret_cast<ResTIMG*>(s_dpadLeftBtiBuf.data);
    }
    return nullptr;
}

static const ResTIMG* s_origZbtnTex = nullptr;
static JUtility::TColor s_origBlack(0, 0, 0, 0);
static JUtility::TColor s_origWhite(255, 255, 255, 255);
static bool s_hasOrigProps = false;

const ResTIMG* get_orig_z_button_texture() {
    return s_origZbtnTex;
}

JUtility::TColor get_orig_z_button_black() { return s_origBlack; }
JUtility::TColor get_orig_z_button_white() { return s_origWhite; }

static void update_custom_z_button_prompt(dMeterButton_c* meterButton) {
    if (!meterButton || !meterButton->mpButtonScreen) return;

    J2DScreen* buttonScreen = meterButton->mpButtonScreen;
    J2DPicture* zbtnPic = static_cast<J2DPicture*>(buttonScreen->search('zbtn'));
    J2DPane* zbtn_n = buttonScreen->search(MULTI_CHAR('zbtn_n'));
    J2DPane* midonaPane = buttonScreen->search(MULTI_CHAR('midona'));
    J2DPane* z_btnl = buttonScreen->search(MULTI_CHAR('z_btnl'));

    if (zbtnPic) {
        if (!s_origZbtnTex && zbtnPic->getTexture(0)) {
            s_origZbtnTex = zbtnPic->getTexture(0)->getTexInfo();
        }
        if (!s_hasOrigProps) {
            s_origBlack = zbtnPic->getBlack();
            s_origWhite = zbtnPic->getWhite();
            s_hasOrigProps = true;
        }

        if (g_configCustomZButtonEnabled) {
            ResTIMG* dpadTex = get_dpad_left_texture();
            if (dpadTex && zbtnPic->getTexture(0) && zbtnPic->getTexture(0)->getTexInfo() != dpadTex) {
                zbtnPic->changeTexture(dpadTex, 0);
            }
            zbtnPic->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));

            // Hide only the letter "Z" components inside zbtn_n
            if (z_btnl) {
                z_btnl->hide();
                z_btnl->setAlpha(0);
            }
            if (zbtn_n) {
                for (J2DPane* child = zbtn_n->getFirstChildPane(); child != nullptr; child = child->getNextChildPane()) {
                    if (child == zbtnPic || child == midonaPane) {
                        continue;
                    }
                    child->hide();
                    child->setAlpha(0);
                }
            }
        } else {
            if (s_origZbtnTex && zbtnPic->getTexture(0) && zbtnPic->getTexture(0)->getTexInfo() != s_origZbtnTex) {
                zbtnPic->changeTexture(s_origZbtnTex, 0);
            }
            if (s_hasOrigProps) {
                zbtnPic->setBlackWhite(s_origBlack, s_origWhite);
            }
            if (z_btnl) {
                z_btnl->show();
                z_btnl->setAlpha(255);
            }
        }
    }
}

HookAction on_meter_button_execute_pre(ModContext*, void* args, void*, void*) {
    if (isTitleOrMainMenu() || !args) return HOOK_CONTINUE;
    dMeterButton_c* meterButton = mods::arg<dMeterButton_c*>(args, 0);
    update_custom_z_button_prompt(meterButton);
    return HOOK_CONTINUE;
}

void on_meter_button_execute_post(ModContext*, void* args, void*, void*) {
    if (isTitleOrMainMenu() || !args) return;
    dMeterButton_c* meterButton = mods::arg<dMeterButton_c*>(args, 0);
    update_custom_z_button_prompt(meterButton);
}
