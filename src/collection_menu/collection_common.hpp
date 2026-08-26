#pragma once

#include "collection_menu.hpp"

#include "d/d_menu_collect.h"
#include "d/d_menu_window.h"
#include "d/d_select_cursor.h"
#include "d/d_pane_class.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_string_base.h"
#include "d/d_msg_string.h"
#include "d/d_msg_out_font.h"
#include "d/d_meter_HIO.h"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_alink.h"
#include "d/d_lib.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DPane.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JUtility/TColor.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "m_Do/m_Do_ext.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "mods/svc/hook.hpp"
#include "mods/svc/resource.h"

extern const ResourceService* get_resource_service();

// Globals & Module Context
extern ModContext* g_modCtx;
extern J2DScreen* s_cachedScreen;
extern J2DScreen* s_capturedScreen;
extern dMenu_Collect2D_c* s_currentCollect2D;
extern bool s_needReloadCollect;

// Custom Panes:
// Swords: Slot 1 is ken_n0, Slot 2 is s_paneKenMid, Slot 3 is ken_n1
extern J2DPane* s_paneKenMid;
extern J2DPicture* s_picKenMidFrame;
extern J2DPicture* s_picKenMidIcon;

// Shields: Slot 1 is tate_n0, Slot 2 is s_paneTateMid, Slot 3 is tate_n1
extern J2DPane* s_paneTateMid;
extern J2DPicture* s_picTateMidFrame;
extern J2DPicture* s_picTateMidIcon;

// Clothes: Slot 1 is s_paneFukuStart, Slot 2 is fuku_n0, Slot 3 is fuku_n1, Slot 4 is fuku_n2
extern J2DPane* s_paneFukuStart;
extern J2DPicture* s_picFukuStartFrame;
extern J2DPicture* s_picFukuStartIcon;

// Connectors (tunagi):
extern J2DPicture* s_picTunagiKen2;
extern J2DPicture* s_picTunagiTate2;
extern J2DPicture* s_picTunagiFuku3;

extern ResourceBuffer s_ordonClothesBtiBuf;

// Original pristine translations of vanilla panes (immutable constants from vanilla .blo)
static constexpr f32 s_ken_n0_origX = -34.0f;
static constexpr f32 s_ken_n0_origY = -96.0f;
static constexpr f32 s_ken_g0_origY = -44.0f;
static constexpr f32 s_ken_g1_origY = -44.0f;
static constexpr f32 s_ken_n1_origX = 20.0f;
static constexpr f32 s_tate_n0_origX = -34.0f;
static constexpr f32 s_tate_n0_origY = -39.0f;
static constexpr f32 s_tate_g0_origY = 13.0f;
static constexpr f32 s_tate_g1_origY = 13.0f;
static constexpr f32 s_tate_n1_origX = 20.0f;
static constexpr f32 s_fuku_n0_origX = -34.0f;
static constexpr f32 s_fuku_n0_origY = 18.0f;
static constexpr f32 s_fuku_g0_origY = 70.0f;
static constexpr f32 s_fuku_n1_origX = 20.0f;
static constexpr f32 s_fuku_n2_origX = 74.0f;
static constexpr f32 s_heart_n_origX = 74.0f;
static constexpr f32 s_heart_n_origY = -72.0f;
static constexpr f32 s_kamen_n_origX = 181.0f;
static constexpr f32 s_kamen_n_origY = -28.0f;
static constexpr f32 s_modelbgn_origX = 189.0f;
static constexpr f32 s_modelbgn_origY = -53.0f;
static constexpr f32 s_col_dx = 54.0f;

// Subclass to access J2DPicture internals safely for texture vertex mapping
class CustomPicture : public J2DPicture {
public:
    void copyVisualsFrom(const J2DPicture* src) {
        if (!src) return;
        const CustomPicture* s = static_cast<const CustomPicture*>(src);
        mKind = s->mKind;
        field_0x109 = s->field_0x109;
        for (int i = 0; i < 4; i++) {
            field_0x10a[i] = s->field_0x10a[i];
            mCornerColor[i] = s->mCornerColor[i];
        }
        mBlack = s->mBlack;
        mWhite = s->mWhite;
        mBlendKonstColor = s->mBlendKonstColor;
        mBlendKonstAlpha = s->mBlendKonstAlpha;
    }
};

ResTIMG* get_ordon_clothes_texture();
const ResTIMG* safe_get_tex_info(J2DPane* pane);
void set_pane_pos(J2DPane* pane, f32 x, f32 y);
Vec get_pane_center(J2DPane* pane);
void safe_delete_custom_pane(J2DPane*& pane);
