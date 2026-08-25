#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/hook.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_item_data.h"
#include "d/d_s_play.h"
#include "d/d_msg_object.h"
#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#define private public
#include "d/d_menu_ring.h"
#undef private
#include "d/d_pane_class.h"
#include "d/d_kantera_icon_meter.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "f_op/f_op_actor_mng.h"
#include "Z2AudioLib/Z2SeMgr.h"

#include <cstdio>
#include <cstring>

// Configuration flags
extern bool g_configCustomZButtonEnabled;
extern bool g_configZButtonEnabled;

// Logger & Context
extern const LogService* g_zLogSvc;
extern ModContext* g_zModCtx;

// D-Pad state tracking
extern bool g_dpadLeftHeld;
extern bool g_dpadLeftTrig;

// Physical Z button status
extern bool g_physZHeld;
extern bool g_physZTrig;

// Texture buffers for Z item rendering
extern u8* g_zTexBufMain[2];
extern u8* g_zTexBufShine[2];
extern u8 g_zTexBufIdx;

// Z button inventory tracking
extern u8 g_zInventorySlot;
extern u8 g_zMixSlot;
extern u8 g_zPendingZSlot;
extern void* g_ringArgs;
extern bool g_inSetSelectItemIndex;

// Z item visual cache
extern J2DPicture* g_cachedZMainPic;
extern f32 g_cachedZW;
extern f32 g_cachedZH;
extern u8 g_lastLoadedZItem;
extern bool g_zHasSecondLayer;

// UI elements
extern dKantera_icon_c* g_zKanteraIcon;
extern J2DPicture* g_drawDigitPic[3];

// Utilities
void log_z_info(const char* fmt, ...);
void ensure_z_buffers();
void ensure_z_slot_initialized();
void sync_z_item_state();
u8 find_slot_for_item(u8 itemNo);
bool is_bomb_item(u8 itemNo);
bool isWolfPlayer();
bool isTitleOrMainMenu();
bool isMidnaUnlocked();
bool is_pause_menu_open(dMeter2Draw_c* draw = nullptr);
bool is_z_item_usable();
void set_pane_influenced_alpha_recursive(J2DPane* pane, bool influenced);
u8 combine_select_item(u8 playItem, u8 mixSlot);
u8 resolved_select_item(int index);
void sync_play_select_item(int index);

// Safe CPaneMgr wrappers
inline bool pane_is_ready(CPaneMgr* p) { return p != nullptr && p->getPanePtr() != nullptr; }
inline void safe_pane_hide(CPaneMgr* p) { if (pane_is_ready(p)) p->hide(); }
inline void safe_pane_show(CPaneMgr* p) { if (pane_is_ready(p)) p->show(); }
inline void safe_pane_resize(CPaneMgr* p, f32 w, f32 h) { if (pane_is_ready(p)) p->resize(w, h); }
inline void safe_pane_trans(CPaneMgr* p, f32 x, f32 y) { if (pane_is_ready(p)) p->paneTrans(x, y); }
inline void safe_pane_scale(CPaneMgr* p, f32 s) { if (pane_is_ready(p)) p->scale(s, s); }
inline void safe_pane_alpha_rate(CPaneMgr* p, f32 a) { if (pane_is_ready(p)) p->setAlphaRate(a); }
inline void safe_pane_alpha(CPaneMgr* p, u8 a) { if (pane_is_ready(p)) p->setAlpha(a); }
