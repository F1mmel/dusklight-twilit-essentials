#include "z_button.hpp"

// Sub-module implementation includes
#include "z_common.cpp"
#include "midna_location.cpp"
#include "change_input.cpp"
#include "z_itemwheel.cpp"
#include "z_item_actions.cpp"
#include "z_draw.cpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

bool isNativeZButtonEngine() {
    static int s_cachedResult = -1;
    if (s_cachedResult != -1) {
        return s_cachedResult == 1;
    }

#if defined(_WIN32)
    HMODULE hExe = GetModuleHandleA(nullptr);
    if (hExe != nullptr) {
        if (GetProcAddress(hExe, "?checkShieldCrouch@daAlink_c@@QEAAHXZ") != nullptr ||
            GetProcAddress(hExe, "?setShieldCrouch@daAlink_c@@QEAAXXZ") != nullptr ||
            GetProcAddress(hExe, "?loseRupees@daAlink_c@@QEAAXXZ") != nullptr)
        {
            s_cachedResult = 1;
            return true;
        }
    }
#else
    if (dlsym(RTLD_DEFAULT, "_ZN8daAlink_c17checkShieldCrouchEv") != nullptr ||
        dlsym(RTLD_DEFAULT, "_ZN8daAlink_c15setShieldCrouchEv") != nullptr ||
        dlsym(RTLD_DEFAULT, "_ZN8daAlink_c10loseRupeesEv") != nullptr)
    {
        s_cachedResult = 1;
        return true;
    }
#endif

    s_cachedResult = 0;
    return false;
}

ModResult init_z_button(const HookService* hook_svc, const LogService* log_svc, ModContext* mod_ctx, ModError*) {
    g_zLogSvc = log_svc;
    g_zModCtx = mod_ctx;

    if (isNativeZButtonEngine()) {
        if (log_svc) {
            log_svc->info(mod_ctx, "Native 3-item / Z-button engine detected (Lazy Tweaks). Skipping custom Z-button hooks.");
        }
        return MOD_OK;
    }

    if (!hook_svc || !g_configCustomZButtonEnabled)
        return MOD_OK;

    mods::hook::add_post<PadReadHook>(hook_svc, on_pad_read_post);
    mods::hook::add_pre<SetActiveCursorHook>(hook_svc, on_set_active_cursor_pre);
    mods::hook::add_post<SetActiveCursorHook>(hook_svc, on_set_active_cursor_post);
    mods::hook::add_post<CheckStatusHook>(hook_svc, on_check_status_post);
    mods::hook::add_post<Meter2ExecuteHook>(hook_svc, on_meter2_execute_post);
    mods::hook::add_post<Meter2DrawDrawHook>(hook_svc, on_meter2_draw_draw_post);
    mods::hook::add_pre<MidonaAlphaHook>(hook_svc, on_set_button_icon_midona_alpha_pre);
    mods::hook::add_pre<ButtonIconAlphaHook>(hook_svc, on_set_button_icon_alpha_pre);
    mods::hook::add_pre<ChangeTextureItemXYHook>(hook_svc, on_change_texture_item_xy_pre);
    mods::hook::add_post<MoveButtonXYHook>(hook_svc, on_move_button_xy_post);
    mods::hook::add_post<OrderTalkHook>(hook_svc, on_order_talk_post);
    mods::hook::add_pre<CheckItemSetButtonHook>(hook_svc, on_check_item_set_button_pre);
    mods::hook::add_pre<CheckSetItemTriggerHook>(hook_svc, on_check_set_item_trigger_pre);
    mods::hook::add_pre<SetHeavyBootsHook>(hook_svc, on_set_heavy_boots_pre);
    mods::hook::add_pre<CheckItemButtonChangeHook>(hook_svc, on_check_item_button_change_pre);
    mods::hook::add_pre<CheckItemChangeFromButtonHook>(hook_svc, on_check_item_change_from_button_pre);
    mods::hook::add_pre<MidnaTalkTriggerHook>(hook_svc, on_midna_talk_trigger_pre);
    mods::hook::add_post<SetStickDataHook>(hook_svc, on_set_stick_data_post);
    mods::hook::add_pre<SetSelectItemIndexHook>(hook_svc, on_set_select_item_index_pre);
    mods::hook::add_post<DrawButtonZHook>(hook_svc, on_draw_button_z_post);
    mods::hook::add_pre<SetMixItemHook>(hook_svc, on_set_mix_item_pre);
    mods::hook::add_pre<SetItemHook>(hook_svc, on_set_item_pre);
    mods::hook::add_pre<SetJumpItemHook>(hook_svc, on_set_jump_item_pre);
    mods::hook::add_pre<SetSelectItemForceHook>(hook_svc, on_set_select_item_force_pre);
    mods::hook::add_pre<IsMixItemOnHook>(hook_svc, on_is_mix_item_on_pre);
    mods::hook::add_pre<IsMixItemOffHook>(hook_svc, on_is_mix_item_off_pre);
    mods::hook::add_pre<CheckExplainForceHook>(hook_svc, on_check_explain_force_pre);
    mods::hook::add_pre<GetSelectItemHook>(hook_svc, on_get_select_item_pre);

    return MOD_OK;
}

void update_z_button(const LogService* log_svc, ModContext* mod_ctx) {
    if (isNativeZButtonEngine()) {
        return;
    }

    g_zLogSvc = log_svc;
    g_zModCtx = mod_ctx;

    if (!g_configCustomZButtonEnabled || isTitleOrMainMenu()) {
        shutdown_z_button();
        return;
    }

    if (g_configCustomZButtonEnabled) {
        f32 scale = 0.85f;
        g_drawHIO.mMidnaIconScale = scale;

        g_drawHIO.mButtonZItemPosX = 0.0f;
        g_drawHIO.mButtonZItemPosY = 0.0f;
        g_drawHIO.mButtonZItemScale = 1.0f;

        if (!isWolfPlayer()) {
            update_z_item_texture();
        }
    }
}

void shutdown_z_button() {
    g_zKanteraIcon = nullptr;

    g_drawDigitPic[0] = nullptr;
    g_drawDigitPic[1] = nullptr;
    g_drawDigitPic[2] = nullptr;

    g_cachedZMainPic = nullptr;
    g_lastLoadedZItem = 0xFF;

    g_dpadLeftHeld = false;
    g_dpadLeftTrig = false;

    reset_midna_pane();
}