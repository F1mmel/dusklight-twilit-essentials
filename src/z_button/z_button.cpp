#include "z_button.hpp"

// Sub-module implementation includes
#include "z_common.cpp"
#include "midna_location.cpp"
#include "change_input.cpp"
#include "z_itemwheel.cpp"
#include "z_item_actions.cpp"
#include "z_draw.cpp"
#include "f_pc/f_pc_profile_lst.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

bool isNativeZButtonEngine() {
    /** this should now more definitively check if mItemHeap is [2] or [3] by checking if the size of daAlink_c on
    runtime is correct vs what the SDK assumes it is **/
    const u32 actualSize = g_profile_ALINK.base.base.process_size; // size of daAlink_c on runtime
    const size_t linkSize = sizeof(daAlink_c); // expected size based on dusklight's SDK
    const size_t animHeapSize = sizeof(daPy_anmHeap_c); // expected size of mItemHeap/mAnmHeap based on dusklight's SDK

    /** lazy tweaks or a z-items fork that increased mItemHeap to [3] will shift the size of daAlink_c by more than
    what is calculated here, but this will know it's specifically a z-items fork if daAlink_c is increased by
    specifically 32 (another mItemHeap) (assuming no other heaps are increased or any other size is added);
    if a fork happens to do other weird stuff (which I'm going to avoid doing with lazy tweaks),
    this won't catch it, but this works for now unless a universal implementation is wanted **/
    if (static_cast<const int>(actualSize) == static_cast<const int>(linkSize) + static_cast<const int>(animHeapSize)) {
        return true;
    }
    return false;
}

ModResult init_z_button(const HookService* hook_svc, const LogService* log_svc, ModContext* mod_ctx, ModError*) {
    g_zLogSvc = log_svc;
    g_zModCtx = mod_ctx;

    if (!hook_svc)
        return MOD_OK;

    // HUD and Item Wheel UI hooks (run on all builds, including native Z-button engines)
    mods::hook::add_pre<MeterButtonExecuteHook>(hook_svc, on_meter_button_execute_pre);
    mods::hook::add_post<MeterButtonExecuteHook>(hook_svc, on_meter_button_execute_post);
    mods::hook::add_post<RingCreateHook>(hook_svc, after_ring_create);
    mods::hook::add_pre<RingDeleteHook>(hook_svc, before_ring_delete);
    mods::hook::add_post<RingDrawHook>(hook_svc, after_ring_draw);
    mods::hook::add_pre<SetActiveCursorHook>(hook_svc, on_set_active_cursor_pre);
    mods::hook::add_post<SetActiveCursorHook>(hook_svc, on_set_active_cursor_post);

    if (isNativeZButtonEngine()) {
        if (log_svc) {
            log_svc->info(mod_ctx, "Native 3-item / Z-button engine detected (Lazy Tweaks). Skipping custom low-level inventory hooks.");
        }
        return MOD_OK;
    }

    mods::hook::add_post<PadReadHook>(hook_svc, on_pad_read_post);
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

    return MOD_OK;
}

void update_z_button(const LogService* log_svc, ModContext* mod_ctx) {
    if (isNativeZButtonEngine()) {
        return;
    }

    g_zLogSvc = log_svc;
    g_zModCtx = mod_ctx;

    static bool s_wasActive = false;
    if (!g_configCustomZButtonEnabled || isTitleOrMainMenu()) {
        if (s_wasActive) {
            shutdown_z_button();
            s_wasActive = false;
        }
        return;
    }
    s_wasActive = true;

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

    for (int i = 0; i < 3; i++) {
        g_drawDigitPic[i] = nullptr;
    }

    g_cachedZMainPic = nullptr;
    g_lastLoadedZItem = 0xFF;

    g_dpadLeftHeld = false;
    g_dpadLeftTrig = false;

    if (!isTitleOrMainMenu() && g_meter2_info.getMeterClass() != nullptr) {
        dMeter2Draw_c* draw = g_meter2_info.getMeterClass()->getMeterDrawPtr();
        if (draw) {
            CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
            if (itemR) {
                safe_pane_hide(itemR);
            }
        }
        reset_midna_pane();
    }
}