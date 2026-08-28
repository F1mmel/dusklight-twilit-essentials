#pragma once

#include "z_common.hpp"

struct Meter2DrawDrawHook;
struct DrawButtonZHook;
struct MidonaAlphaHook;
struct ButtonIconAlphaHook;
struct ChangeTextureItemXYHook;
struct MoveButtonXYHook;
struct Meter2ExecuteHook;
struct SetAlphaButtonChangeHook;

void update_z_item_texture(dMeter2Draw_c* draw = nullptr);
void draw_z_ammo_digits(dMeter2Draw_c* draw, f32 baseX, f32 baseY, f32 iconW, f32 iconH, f32 alphaRate);
void draw_item_count_digits(int num, int maxNum, f32 baseX, f32 baseY, f32 iconW, f32 iconH, f32 alphaRate);

void on_meter2_draw_draw_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_draw_button_z_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_set_button_icon_midona_alpha_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_set_button_icon_midona_alpha_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_set_button_icon_alpha_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
HookAction on_change_texture_item_xy_pre(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_move_button_xy_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_meter2_execute_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
void on_set_alpha_button_change_post(ModContext* mod_ctx, void* args, void* ret, void* user_data);
