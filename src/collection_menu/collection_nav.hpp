#pragma once

#include "collection_common.hpp"

// Target pane lookup for grid navigation and scaling
J2DPane* get_target_pane(dMenu_Collect2D_c* collect2D, u8 x, u8 y);

// Menu hooks
DEFINE_HOOK(&dMenu_Collect2D_c::getItemTag, GetItemTagHook);
HookAction on_get_item_tag_pre(ModContext*, void* args, void* ret, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::cursorPosSet, CursorPosSetHook);
HookAction on_cursor_pos_set_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::cursorMove, CursorMoveHook);
HookAction on_cursor_move_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMenu_Collect2D_c::setItemNameString, SetItemNameStringHook);
HookAction on_set_item_name_string_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMeter2Info_c::getStringKanji, GetStringKanjiHook);
HookAction on_get_string_kanji_pre(ModContext*, void* args, void*, void*);

DEFINE_HOOK(&dMsgStringBase_c::getStringLocal, MsgStringGetStringLocalHook);
HookAction on_get_string_local_pre(ModContext*, void* args, void* ret, void*);
