#pragma once

#include "z_common.hpp"

void update_midna_pane(dMeter2Draw_c* draw);
void reset_midna_pane();
HookAction on_meter_button_execute_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_meter_button_execute_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_meter_button_draw_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
