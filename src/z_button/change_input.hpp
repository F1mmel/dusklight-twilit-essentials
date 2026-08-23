#pragma once

#include "z_common.hpp"

struct PadReadHook;
struct SetStickDataHook;
struct MidnaTalkTriggerHook;
struct OrderTalkHook;
struct CheckItemSetButtonHook;
struct CheckSetItemTriggerHook;
struct CheckItemButtonChangeHook;
struct CheckItemChangeFromButtonHook;

void on_pad_read_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_set_stick_data_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_midna_talk_trigger_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_order_talk_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_check_item_set_button_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_check_set_item_trigger_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_check_item_button_change_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_check_item_change_from_button_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
