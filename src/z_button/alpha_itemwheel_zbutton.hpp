#pragma once

#include "z_common.hpp"
#include "z_draw.hpp"

struct SetActiveCursorHook;
struct SetSelectItemIndexHook;
struct CheckStatusHook;

u8 get_ring_slot_for_item(dMenu_Ring_c* ring, u8 slotOrItem);
void trigger_ring_item_slide_z(dMenu_Ring_c* ring, u8 itemNo);
void commit_pending_z_slot(dMenu_Ring_c* ring = nullptr);
void update_ring_z_slots(dMenu_Ring_c* ring);

HookAction on_set_active_cursor_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_set_active_cursor_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_set_select_item_index_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_check_status_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
