#include "collection_menu.hpp"

// Sub-module implementation includes (matching modular architecture)
#include "collection_common.cpp"
#include "collection_layout.cpp"
#include "collection_nav.cpp"
#include "collection_equip.cpp"

bool g_configCollectionStarterEquip = false;
bool g_configCollectionKeepOrdonShield = false;
bool g_configCollectionUnequip = false;

ModResult init_collection_menu(const HookService* hook_svc, const LogService* log_svc, ModContext* mod_ctx, ModError* error) {
    g_modCtx = mod_ctx;
    g_logSvc = log_svc;

    log_collect_info("[CollectionMenu] init_collection_menu: keepOrdonShield=%d, starterEquip=%d, unequip=%d",
                     g_configCollectionKeepOrdonShield, g_configCollectionStarterEquip, g_configCollectionUnequip);

    if (hook_svc) {
        // Area transition & spawn preservation
        mods::hook::add_pre<DaAlinkCreateHook>(hook_svc, on_da_alink_create_pre);
        mods::hook::add_post<DaAlinkCreateHook>(hook_svc, on_da_alink_create_post);
        mods::hook::add_pre<SetSelectEquipClothesHook>(hook_svc, on_set_select_equip_clothes_pre);
        mods::hook::add_pre<Meter2InfoSetShieldHook>(hook_svc, on_meter2_info_set_shield_pre);

        // Screen layout & lifecycle
        mods::hook::add_post<MenuCollect2DCreateHook>(hook_svc, on_menu_collect_2d_create_post);
        mods::hook::add_pre<MenuCollect2DDeleteHook>(hook_svc, on_menu_collect_2d_delete_pre);
        mods::hook::add_pre<ScreenSetHook>(hook_svc, on_screen_set_pre);
        mods::hook::add_post<ScreenSetHook>(hook_svc, on_screen_set_post);
        mods::hook::add_post<MenuCollectWideHook>(hook_svc, on_menu_collect_wide_post);
        mods::hook::add_post<MenuCollect2DMoveHook>(hook_svc, on_menu_collect_2d_move_post);
        mods::hook::add_post<MwExecuteHook>(hook_svc, on_mw_execute_post);

        // Menu navigation & item description strings
        mods::hook::add_pre<GetItemTagHook>(hook_svc, on_get_item_tag_pre);
        mods::hook::add_pre<CursorPosSetHook>(hook_svc, on_cursor_pos_set_pre);
        mods::hook::add_pre<CursorMoveHook>(hook_svc, on_cursor_move_pre);
        mods::hook::add_pre<SetItemNameStringHook>(hook_svc, on_set_item_name_string_pre);
        mods::hook::add_pre<GetStringKanjiHook>(hook_svc, on_get_string_kanji_pre);
        mods::hook::add_pre<MsgStringGetStringLocalHook>(hook_svc, on_get_string_local_pre);

        // Equipment actions & frame highlights
        mods::hook::add_pre<WaitProcHook>(hook_svc, on_wait_proc_pre);
        mods::hook::add_post<WaitProcHook>(hook_svc, on_wait_proc_post);
        mods::hook::add_pre<PointerActivateCurrentHook>(hook_svc, on_pointer_activate_current_pre);
        mods::hook::add_pre<ChangeSwordHook>(hook_svc, on_change_sword_pre);
        mods::hook::add_pre<ChangeShieldHook>(hook_svc, on_change_shield_pre);
        mods::hook::add_pre<ChangeClotheHook>(hook_svc, on_change_clothes_pre);
        mods::hook::add_pre<SetEquipFrameColorSwordHook>(hook_svc, on_set_equip_frame_sword_pre);
        mods::hook::add_pre<SetEquipFrameColorShieldHook>(hook_svc, on_set_equip_frame_shield_pre);
        mods::hook::add_pre<SetEquipFrameColorClothesHook>(hook_svc, on_set_equip_frame_clothes_pre);
    }
    return MOD_OK;
}

void request_collection_menu_reload() {
    log_collect_info("[CollectionMenu] request_collection_menu_reload requested");
    s_needReloadCollect = true;
}

void update_collection_menu(const LogService* log_svc, ModContext* mod_ctx) {
    g_logSvc = log_svc;
    g_modCtx = mod_ctx;
}

void shutdown_collection_menu() {
    s_paneKenMid = nullptr;
    s_paneTateMid = nullptr;
    s_paneFukuStart = nullptr;
    s_picKenMidFrame = nullptr;
    s_picKenMidIcon = nullptr;
    s_picTateMidFrame = nullptr;
    s_picTateMidIcon = nullptr;
    s_picFukuStartFrame = nullptr;
    s_picFukuStartIcon = nullptr;
    s_picTunagiKen2 = nullptr;
    s_picTunagiTate2 = nullptr;
    s_picTunagiFuku3 = nullptr;
    s_cachedScreen = nullptr;
    s_capturedScreen = nullptr;
    s_currentCollect2D = nullptr;
}
