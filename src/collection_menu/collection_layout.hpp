#pragma once

#include "collection_common.hpp"

// Layout & visual functions
void update_screen_bases(J2DScreen* screen, JKRExpHeap* heap);
void apply_collect_shifts(dMenu_Collect2D_c* collect2D);
void update_frame_highlights(dMenu_Collect2D_c* collect2D);

// Hook definitions for screen lifecycle
DEFINE_HOOK(&dMenu_Collect2D_c::_create, MenuCollect2DCreateHook);
void on_menu_collect_2d_create_post(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::_delete, MenuCollect2DDeleteHook);
HookAction on_menu_collect_2d_delete_pre(ModContext*, void*, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::screenSet, ScreenSetHook);
HookAction on_screen_set_pre(ModContext*, void* args, void*, void*);
void on_screen_set_post(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::menuCollectWide, MenuCollectWideHook);
void on_menu_collect_wide_post(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::_move, MenuCollect2DMoveHook);
void on_menu_collect_2d_move_post(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMw_c::_execute, MwExecuteHook);
void on_mw_execute_post(ModContext*, void* args, void*, void*);
