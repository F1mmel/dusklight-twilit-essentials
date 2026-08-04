#include "damage_numbers.hpp"

#include <unordered_map>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>

#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"

#include "d/d_meter2_draw.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_iter.h"
#include "f_op/f_op_actor_mng.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_lib.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/JUtility/TColor.h"
#include "JSystem/JUtility/JUTFont.h"

bool g_configDamageNumbersEnabled = false;

struct DamagePopup {
    fpc_ProcID enemyId;
    s16 damageAmount;
    cXyz worldPos;
    f32 velY;
    f32 velX;
    f32 velZ;
    int currentFrame;
    int maxFrames;
    bool isCritical;
};

static std::unordered_map<fpc_ProcID, s16> s_lastHealthMap;
static std::vector<DamagePopup> s_damagePopups;

DEFINE_HOOK(&dMeter2Draw_c::draw, Meter2DrawDmg);

// Draws using the game's resident font. Calls setup2D() afterwards so 3D rendering doesn't break.
static void draw_damage_text(const char* text, f32 x, f32 y, f32 charW, f32 charH, JUtility::TColor color) {
    JUTFont* font = mDoExt_getSubFont();
    if (!font) {
        font = mDoExt_getMesgFont();
    }

    if (font) {
        font->setGX();

        u8 shadowAlpha = static_cast<u8>(static_cast<f32>(color.a) * 0.5f);
        font->setCharColor(JUtility::TColor(0, 0, 0, shadowAlpha));
        font->drawString_scale(x + 1.0f, y + 1.0f, charW, charH, text, true);

        font->setCharColor(color);
        font->drawString_scale(x, y, charW, charH, text, true);

        J2DGrafContext* port = dComIfGp_getCurrentGrafPort();
        if (port) {
            port->setup2D();
        }
    }
}

static f32 get_damage_text_width(const char* text, f32 charW) {
    JUTFont* font = mDoExt_getSubFont();
    if (!font) {
        font = mDoExt_getMesgFont();
    }
    if (font) {
        f32 totalWidth = 0.0f;
        for (size_t i = 0; text[i] != '\0'; i++) {
            f32 w = static_cast<f32>(font->getWidth(text[i]));
            if (w <= 0.0f) w = static_cast<f32>(font->getWidth());
            totalWidth += w * (charW / static_cast<f32>(font->getWidth()));
        }
        return totalWidth;
    }
    return static_cast<f32>(std::strlen(text)) * charW;
}

static void* trackEnemyDamageCallback(void* pActor, void*) {
    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(pActor);
    if (!actor) return nullptr;

    u8 group = fopAcM_GetGroup(actor);
    if (group != fopAc_ENEMY_e) {
        return nullptr;
    }

    fpc_ProcID id = fopAcM_GetID(actor);
    s16 curHp = actor->health;

    auto it = s_lastHealthMap.find(id);
    if (it != s_lastHealthMap.end()) {
        s16 prevHp = it->second;
        if (curHp < prevHp) {
            s16 damage = prevHp - curHp;
            if (damage > 0) {
                DamagePopup popup;
                popup.enemyId = id;
                popup.damageAmount = damage;

                popup.worldPos = actor->attention_info.position;
                if (popup.worldPos.y == 0.0f) {
                    popup.worldPos = actor->current.pos;
                    popup.worldPos.y += 100.0f;
                }

                // Scatter a bit so stacked numbers don't completely overlap
                f32 randX = (static_cast<f32>(std::rand() % 30) - 15.0f);
                f32 randZ = (static_cast<f32>(std::rand() % 30) - 15.0f);
                popup.worldPos.x += randX;
                popup.worldPos.z += randZ;

                popup.velY = 3.5f;
                popup.velX = randX * 0.05f;
                popup.velZ = randZ * 0.05f;
                popup.currentFrame = 0;
                popup.maxFrames = 45;
                popup.isCritical = (damage >= 60);

                s_damagePopups.push_back(popup);
            }
        }
    }

    s_lastHealthMap[id] = curHp;
    return nullptr;
}

void update_damage_numbers(const LogService*, ModContext*) {
    if (!g_configDamageNumbersEnabled) {
        s_lastHealthMap.clear();
        s_damagePopups.clear();
        return;
    }

    if (dComIfGp_isPauseFlag() || dScnPly_c::isPause()) {
        return;
    }

    fopAcIt_Judge(trackEnemyDamageCallback, nullptr);

    for (auto it = s_damagePopups.begin(); it != s_damagePopups.end();) {
        it->currentFrame++;
        it->worldPos.y += it->velY;
        it->worldPos.x += it->velX;
        it->worldPos.z += it->velZ;
        it->velY *= 0.90f;

        if (it->currentFrame >= it->maxFrames) {
            it = s_damagePopups.erase(it);
        } else {
            ++it;
        }
    }
}

static void on_meter2_draw(ModContext*, void*, void*, void*) {
    if (!g_configDamageNumbersEnabled || s_damagePopups.empty()) {
        return;
    }

    if (dComIfGp_isPauseFlag() || dScnPly_c::isPause()) {
        return;
    }

    if (dComIfGp_getEvent() && dComIfGp_getEvent()->runCheck()) {
        return;
    }

    for (const auto& popup : s_damagePopups) {
        cXyz pos = popup.worldPos;
        Vec screenPos;
        mDoLib_project(&pos, &screenPos);

        if (screenPos.z < 1.0f && screenPos.x > -50.0f && screenPos.x < 700.0f && screenPos.y > -50.0f && screenPos.y < 500.0f) {
            f32 alphaF = 1.0f;
            if (popup.currentFrame > (popup.maxFrames - 15)) {
                alphaF = static_cast<f32>(popup.maxFrames - popup.currentFrame) / 15.0f;
            }

            u8 alpha = static_cast<u8>(alphaF * 255.0f);
            if (alpha == 0) continue;

            // Quick pop-in scale on spawn
            f32 popScale = 1.0f;
            if (popup.currentFrame < 5) {
                popScale = 1.3f - (static_cast<f32>(popup.currentFrame) * 0.06f);
            }

            char numText[32];
            if (popup.isCritical) {
                std::snprintf(numText, sizeof(numText), "-%d!", popup.damageAmount);
            } else {
                std::snprintf(numText, sizeof(numText), "-%d", popup.damageAmount);
            }

            f32 charW = (popup.isCritical ? 14.0f : 11.0f) * popScale;
            f32 charH = (popup.isCritical ? 17.0f : 14.0f) * popScale;

            f32 textW = get_damage_text_width(numText, charW);
            f32 drawX = screenPos.x - (textW * 0.5f);
            f32 drawY = screenPos.y - (charH * 0.5f);

            JUtility::TColor numColor(235, 45, 45, alpha);

            draw_damage_text(numText, drawX, drawY, charW, charH, numColor);
        }
    }
}

ModResult init_damage_numbers(const HookService* hook_svc, ModError*) {
    s_lastHealthMap.clear();
    s_damagePopups.clear();

    if (hook_svc) {
        mods::hook_add_post<Meter2DrawDmg>(hook_svc, on_meter2_draw);
    }
    return MOD_OK;
}

void shutdown_damage_numbers() {
    s_lastHealthMap.clear();
    s_damagePopups.clear();
}
