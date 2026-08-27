#pragma once

#include "z_common.hpp"

bool z_mobile_active();
void z_mobile_init(const HookService* hook_svc);
bool z_mobile_wants_midona_host();
J2DPane* z_mobile_sync_touch_z(dMeter2Draw_c* draw);
void z_mobile_report_after_midna_alpha(dMeter2Draw_c* draw);
bool z_mobile_touch_z_rect(f32& x, f32& y, f32& w, f32& h);
HookAction z_mobile_guard_heavy_boots(void* args, void* retval);
bool z_mobile_hb_locked(daAlink_c* link);
void z_mobile_hb_lock(daAlink_c* link, bool manualToggleOff);
void z_mobile_hb_tick(daAlink_c* link);
void z_mobile_shutdown();
