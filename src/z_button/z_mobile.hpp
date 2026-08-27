#pragma once

#include "z_common.hpp"

// ---------------------------------------------------------------------------
// Mobile-only (Android / iOS) Z-button fixes. See z_mobile.cpp for the full
// rationale. Everything here is a no-op on desktop builds.
// Z_MOBILE_BUILD is defined in z_common.hpp.
// ---------------------------------------------------------------------------

// True only on Android / iOS builds.
bool z_mobile_active();

// Installs the touch-Z-button hooks. Call once from init_z_button().
// Safe on desktop (no-op).
void z_mobile_init(const HookService* hook_svc);

// True while the HUD-capture fallback owns the `midona_n` pane (used when the
// game build ships no symbol manifest, so the icon can't be injected directly).
bool z_mobile_wants_midona_host();

// Prepares `midona_n` so dusklight's render-to-texture of that pane produces
// the Z item icon for the touch Z button, at full opacity. Returns the pane the
// Z item should be parented into, or nullptr when the fallback is inactive.
// Call from the setButtonIconMidonaAlpha pre-hook.
J2DPane* z_mobile_sync_touch_z(dMeter2Draw_c* draw);

// Diagnostics: logs the pane state the icon capture actually saw, throttled to
// roughly once every two seconds. Call from the setButtonIconMidonaAlpha POST
// hook (i.e. right after dusklight rendered the pane into the Z button).
void z_mobile_report_after_midna_alpha(dMeter2Draw_c* draw);

// Fills the on-screen rect (J2DPane global-bounds space) of the real touch Z
// button, resolved via the SDK hook service. Returns false on desktop or before
// dusklight has synced the target.
bool z_mobile_touch_z_rect(f32& x, f32& y, f32& w, f32& h);

// --- Iron / heavy boots on the Z slot (Android/iOS only; no-ops on desktop) ---

// `daAlink_c::setHeavyBoots` pre-hook body: blocks the involuntary un-equip
// while the iron boots are the selected Z item. Wire from on_set_heavy_boots_pre.
HookAction z_mobile_guard_heavy_boots(void* args, void* retval);

// True while a Z boots press is still being processed (debounce). Checked by the
// item-trigger hooks so they don't re-trigger the same press.
bool z_mobile_hb_locked(daAlink_c* link);

// Arm the debounce lock. `manualToggleOff` = boots were already on (toggle-off).
void z_mobile_hb_lock(daAlink_c* link, bool manualToggleOff);

// Per-frame: releases the lock once Z is let go / the toggle window elapses.
// Call from a daAlink per-frame hook (on_set_stick_data_post).
void z_mobile_hb_tick(daAlink_c* link);

// Releases retained state (boots lock). Call from shutdown_z_button().
void z_mobile_shutdown();
