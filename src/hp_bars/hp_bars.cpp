#include "hp_bars.hpp"

#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <cstdint>

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
#include "f_pc/f_pc_name.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_lib.h"
#include "m_Do/m_Do_ext.h"

#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include "JSystem/JUtility/TColor.h"
#include "JSystem/JUtility/JUTFont.h"

bool g_configHpBarsEnabled = false;
bool g_configHpBarsBossesEnabled = false;
bool g_configHpBarsShowNumbers = false;

DEFINE_HOOK(&dMeter2Draw_c::draw, Meter2Draw);

static std::unordered_map<fpc_ProcID, s16> g_maxHealthMap;
static std::unordered_map<fpc_ProcID, f32> g_enemyAlphaMap;

enum EnemyCategory {
    ENEMY_NORMAL,
    ENEMY_MIDBOSS,
    ENEMY_ENDBOSS
};

static EnemyCategory getEnemyCategory(fopAc_ac_c* actor) {
    if (!actor) return ENEMY_NORMAL;

    s16 name = fopAcM_GetName(actor);

    if (name == fpcNm_B_OH_e   || name == fpcNm_B_OH2_e ||
        name == fpcNm_B_DRE_e  || name == fpcNm_B_OB_e   ||
        name == fpcNm_B_GM_e   || name == fpcNm_B_YO_e   ||
        name == fpcNm_B_YOI_e  || name == fpcNm_B_GO_e   ||
        name == fpcNm_B_DR_e   || name == fpcNm_B_ZANT_e ||
        name == fpcNm_B_ZANTM_e|| name == fpcNm_B_ZANTZ_e||
        name == fpcNm_B_ZANTS_e|| name == fpcNm_B_GND_e  ||
        name == fpcNm_B_GG_e   || name == fpcNm_B_GOS_e) {
        return ENEMY_ENDBOSS;
    }

    // Darkhammer (fpcNm_E_TH_e) is listed as a midboss
    if (name == fpcNm_E_TH_e   || name == fpcNm_B_MGN_e  ||
        name == fpcNm_B_DS_e   || name == fpcNm_B_BH_e   ||
        name == fpcNm_B_BQ_e   || name == fpcNm_B_TN_e   ||
        name == fpcNm_E_MB_e   || name == fpcNm_E_TK_e   ||
        name == fpcNm_E_TK2_e  || name == fpcNm_E_KG_e   ||
        name == fpcNm_E_VT_e   || name == fpcNm_E_KK_e   ||
        name == fpcNm_E_MM_e) {
        return ENEMY_MIDBOSS;
    }

    // Fall back to stage flags if name check didn't match
    if (dComIfGs_isStageBossEnemy()) {
        return ENEMY_ENDBOSS;
    }
    if (dComIfGs_isStageMiddleBoss()) {
        return ENEMY_MIDBOSS;
    }

    return ENEMY_NORMAL;
}

static const char* getBossName(fopAc_ac_c* actor) {
    if (!actor) return "BOSS";
    s16 name = fopAcM_GetName(actor);

    switch (name) {
        case fpcNm_E_TH_e:    return "DARKHAMMER";
        case fpcNm_E_MB_e:    return "OOK";
        case fpcNm_E_TK_e:
        case fpcNm_E_TK2_e:   return "DEKU TOAD";
        case fpcNm_B_DS_e:    return "DEATH SWORD";
        case fpcNm_B_MGN_e:   return "DANGORO";
        case fpcNm_E_VT_e:    return "AERALFOS";
        case fpcNm_E_KG_e:    return "DARKHAMMER";
        case fpcNm_E_KK_e:
        case fpcNm_E_WB_e:    return "KING BULBLIN";
        case fpcNm_E_PO_e:    return "POE BOSS";
        case fpcNm_B_BH_e:    return "BABA SERPENT";
        case fpcNm_B_BQ_e:    return "PUPPET ZELDA";
        case fpcNm_B_TN_e:    return "DARKNUT";

        case fpcNm_B_OH_e:
        case fpcNm_B_OH2_e:   return "DIABABA";
        case fpcNm_B_DRE_e:   return "FYRUS";
        case fpcNm_B_OB_e:    return "MORPHEEL";
        case fpcNm_B_GM_e:    return "STALLORD";
        case fpcNm_B_YO_e:
        case fpcNm_B_YOI_e:   return "BLIZZETA";
        case fpcNm_B_GO_e:    return "ARMOGOHMA";
        case fpcNm_B_DR_e:    return "ARGOROK";
        case fpcNm_B_ZANT_e:
        case fpcNm_B_ZANTM_e:
        case fpcNm_B_ZANTZ_e:
        case fpcNm_B_ZANTS_e: return "ZANT";
        case fpcNm_B_GND_e:
        case fpcNm_B_GG_e:
        case fpcNm_B_GOS_e:   return "GANONDORF";

        default:              return "BOSS";
    }
}

// Hand-rolled 5x7 bitmap font — no JUT dependency, always works
static void get_glyph_5x7(char ch, uint8_t* rows) {
    for (int i = 0; i < 7; i++) rows[i] = 0;

    if (ch >= '0' && ch <= '9') {
        static const uint8_t digits[10][7] = {
            {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
            {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
            {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
            {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // 3
            {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
            {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
            {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
            {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
            {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
            {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}  // 9
        };
        std::memcpy(rows, digits[ch - '0'], 7);
        return;
    }

    switch (ch) {
        case '/':
            rows[0] = 0x01; rows[1] = 0x02; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x08; rows[5] = 0x10; rows[6] = 0x10;
            break;
        case 'A':
            rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1F; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11;
            break;
        case 'B':
            rows[0] = 0x1E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1E; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x1E;
            break;
        case 'C':
            rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x10; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x11; rows[6] = 0x0E;
            break;
        case 'D':
            rows[0] = 0x1C; rows[1] = 0x12; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x12; rows[6] = 0x1C;
            break;
        case 'E':
            rows[0] = 0x1F; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x1E; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x1F;
            break;
        case 'F':
            rows[0] = 0x1F; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x1E; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x10;
            break;
        case 'G':
            rows[0] = 0x0F; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x17; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E;
            break;
        case 'H':
            rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1F; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11;
            break;
        case 'I':
            rows[0] = 0x0E; rows[1] = 0x04; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x0E;
            break;
        case 'J':
            rows[0] = 0x0F; rows[1] = 0x02; rows[2] = 0x02; rows[3] = 0x02; rows[4] = 0x02; rows[5] = 0x12; rows[6] = 0x0C;
            break;
        case 'K':
            rows[0] = 0x11; rows[1] = 0x12; rows[2] = 0x14; rows[3] = 0x18; rows[4] = 0x14; rows[5] = 0x12; rows[6] = 0x11;
            break;
        case 'L':
            rows[0] = 0x10; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x1F;
            break;
        case 'M':
            rows[0] = 0x11; rows[1] = 0x1B; rows[2] = 0x15; rows[3] = 0x15; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11;
            break;
        case 'N':
            rows[0] = 0x11; rows[1] = 0x19; rows[2] = 0x15; rows[3] = 0x15; rows[4] = 0x13; rows[5] = 0x11; rows[6] = 0x11;
            break;
        case 'O':
            rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E;
            break;
        case 'P':
            rows[0] = 0x1E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1E; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x10;
            break;
        case 'R':
            rows[0] = 0x1E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1E; rows[4] = 0x14; rows[5] = 0x12; rows[6] = 0x11;
            break;
        case 'S':
            rows[0] = 0x0F; rows[1] = 0x10; rows[2] = 0x08; rows[3] = 0x06; rows[4] = 0x01; rows[5] = 0x11; rows[6] = 0x0E;
            break;
        case 'T':
            rows[0] = 0x1F; rows[1] = 0x04; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x04;
            break;
        case 'U':
            rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E;
            break;
        case 'V':
            rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x0A; rows[6] = 0x04;
            break;
        case 'W':
            rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x15; rows[4] = 0x15; rows[5] = 0x1B; rows[6] = 0x11;
            break;
        case 'Y':
            rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x0A; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x04;
            break;
        case 'Z':
            rows[0] = 0x1F; rows[1] = 0x02; rows[2] = 0x04; rows[3] = 0x08; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x1F;
            break;
        default:
            break;
    }
}

static void draw_char_5x7(char ch, f32 x, f32 y, f32 scale, JUtility::TColor color) {
    uint8_t rows[7];
    get_glyph_5x7(ch, rows);

    for (int r = 0; r < 7; r++) {
        uint8_t rowVal = rows[r];
        for (int c = 0; c < 5; c++) {
            int bitIdx = 4 - c;
            if ((rowVal >> bitIdx) & 1) {
                J2DFillBox(x + static_cast<f32>(c) * scale, 
                           y + static_cast<f32>(r) * scale, 
                           scale, scale, color);
            }
        }
    }
}

static void draw_text_5x7(const char* text, f32 x, f32 y, f32 scale, JUtility::TColor color, bool hasShadow = true) {
    f32 curX = x;
    f32 spacing = 6.0f * scale;
    u8 shadowAlpha = static_cast<u8>(static_cast<f32>(color.a) * 0.5f);
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (hasShadow) {
            draw_char_5x7(text[i], curX + 1.0f, y + 1.0f, scale, JUtility::TColor(0, 0, 0, shadowAlpha));
        }
        draw_char_5x7(text[i], curX, y, scale, color);
        curX += spacing;
    }
}

// Renders using the game's resident sub-font. After drawing we call setup2D() to put
// GX state back — without this, 3D actors rendered after us will crash (Array 24 unmapped).
static void draw_text_ingame(const char* text, f32 x, f32 y, f32 charW, f32 charH, JUtility::TColor color, bool hasShadow = true) {
    JUTFont* font = mDoExt_getSubFont();
    if (!font) {
        font = mDoExt_getMesgFont();
    }
    if (font) {
        font->setGX();
        if (hasShadow) {
            u8 shadowAlpha = static_cast<u8>(static_cast<f32>(color.a) * 0.5f);
            font->setCharColor(JUtility::TColor(0, 0, 0, shadowAlpha));
            font->drawString_scale(x + 1.0f, y + 1.0f, charW, charH, text, true);
        }
        font->setCharColor(color);
        font->drawString_scale(x, y, charW, charH, text, true);

        J2DGrafContext* port = dComIfGp_getCurrentGrafPort();
        if (port) {
            port->setup2D();
        }
    } else {
        // sub-font not available, fall back to bitmap
        f32 fallbackScale = charW * 0.1f;
        draw_text_5x7(text, x, y, fallbackScale, color, hasShadow);
    }
}

static f32 get_text_width_ingame(const char* text, f32 charW) {
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

struct DrawHpBarContext {
    fopAc_ac_c* player;
    f32 cursorOffsetY;
    fopAc_ac_c* activeBoss;
    EnemyCategory bossCategory;
};

// Top HUD boss bar — drawn centered slightly left of screen midpoint
static void drawTopHudBossBar(fopAc_ac_c* boss, EnemyCategory category, f32 alpha) {
    if (!boss || alpha < 0.01f) return;

    fpc_ProcID id = fopAcM_GetID(boss);
    s16 maxHp = g_maxHealthMap[id];
    if (maxHp <= 0) maxHp = boss->health;

    f32 screenW = mDoGph_gInf_c::getWidthF();
    if (screenW <= 0.0f) screenW = 640.0f;

    bool isEndboss = (category == ENEMY_ENDBOSS);

    const f32 barWidth = isEndboss ? 260.0f : 190.0f;
    const f32 barHeight = isEndboss ? 10.0f : 8.0f;
    const f32 drawX = (screenW - barWidth) * 0.5f - (barWidth * 0.5f);
    const f32 drawY = isEndboss ? 32.0f : 30.0f;

    f32 hpRatio = static_cast<f32>(boss->health) / static_cast<f32>(maxHp);
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;

    auto getAlpha = [alpha](u8 baseAlpha) -> u8 {
        return static_cast<u8>(static_cast<f32>(baseAlpha) * alpha);
    };

    J2DFillBox(drawX - 3.0f, drawY - 3.0f, barWidth + 6.0f, barHeight + 6.0f, JUtility::TColor(0, 0, 0, getAlpha(180)));

    JUtility::TColor frameColor = isEndboss ? JUtility::TColor(212, 175, 55, getAlpha(240))
                                             : JUtility::TColor(180, 190, 200, getAlpha(240));
    J2DDrawFrame(drawX - 2.0f, drawY - 2.0f, barWidth + 4.0f, barHeight + 4.0f, frameColor, 2);

    J2DFillBox(drawX, drawY, barWidth, barHeight, JUtility::TColor(15, 15, 20, getAlpha(240)));

    // Green -> amber -> red depending on remaining HP
    u8 r, g, b;
    if (hpRatio > 0.5f) {
        f32 factor = (hpRatio - 0.5f) * 2.0f;
        r = static_cast<u8>((1.0f - factor) * 230.0f + factor * 30.0f);
        g = static_cast<u8>(factor * 190.0f + (1.0f - factor) * 170.0f);
        b = static_cast<u8>(factor * 70.0f + (1.0f - factor) * 30.0f);
    } else {
        f32 factor = hpRatio * 2.0f;
        r = static_cast<u8>((1.0f - factor) * 220.0f + factor * 230.0f);
        g = static_cast<u8>(factor * 170.0f);
        b = static_cast<u8>((1.0f - factor) * 40.0f + factor * 30.0f);
    }

    if (hpRatio > 0.0f) {
        f32 fillWidth = barWidth * hpRatio;
        J2DFillBox(drawX, drawY, fillWidth, barHeight, JUtility::TColor(r, g, b, getAlpha(245)));
        J2DFillBox(drawX, drawY, fillWidth, 1.5f, JUtility::TColor(255, 255, 255, getAlpha(100)));
    }

    const char* titleText = getBossName(boss);
    const f32 fontW = isEndboss ? 11.0f : 9.5f;
    const f32 fontH = isEndboss ? 13.0f : 11.5f;
    f32 titleWidth = get_text_width_ingame(titleText, fontW);
    f32 titleX = drawX + (barWidth - titleWidth) * 0.5f;
    f32 titleY = drawY - (isEndboss ? 13.0f : 11.5f);

    draw_text_ingame(titleText, titleX, titleY, fontW, fontH, 
                     isEndboss ? JUtility::TColor(255, 220, 120, getAlpha(255))
                               : JUtility::TColor(220, 230, 240, getAlpha(255)), true);

    char hpText[32];
    std::snprintf(hpText, sizeof(hpText), "%d / %d", boss->health, maxHp);
    const f32 hpFontW = isEndboss ? 8.0f : 6.8f;
    const f32 hpFontH = isEndboss ? 10.0f : 8.5f;
    f32 textWidth = get_text_width_ingame(hpText, hpFontW);
    f32 textX = drawX + (barWidth - textWidth) * 0.5f;
    f32 textY = drawY + (barHeight - hpFontH) * 0.5f;

    draw_text_ingame(hpText, textX, textY, hpFontW, hpFontH, JUtility::TColor(255, 255, 255, getAlpha(255)), true);
}

static int drawEnemyHpBarCallback(void* pActor, void* pData) {
    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(pActor);
    if (!actor) return 0;

    fpc_ProcID id = fopAcM_GetID(actor);

    bool isAlive = (actor->health > 0);
    u8 group = fopAcM_GetGroup(actor);
    bool isEnemyGroup = (group == fopAc_ENEMY_e);

    s16 name = fopAcM_GetName(actor);

    // Skip passive animals — they share the enemy group but aren't hostile
    bool isPassiveAnimal = (name == fpcNm_E_WB_e || name == fpcNm_HORSE_e ||
                            name == fpcNm_COW_e || name == fpcNm_NI_e ||
                            name == fpcNm_DO_e || name == fpcNm_SQ_e);

    // Only draw bars for enemies that are actually fighting
    bool isCombatHostile = (actor->attention_info.flags & (fopAc_AttnFlag_BATTLE_e | fopAc_AttnFlag_LOCK_e)) != 0;

    bool isEnemy = isAlive && isEnemyGroup && !isPassiveAnimal && isCombatHostile;

    DrawHpBarContext* ctx = static_cast<DrawHpBarContext*>(pData);
    EnemyCategory cat = isEnemy ? getEnemyCategory(actor) : ENEMY_NORMAL;

    // Track the highest HP value seen so the bar doesn't grow when HP increases (e.g. Zant)
    auto itMaxHp = g_maxHealthMap.find(id);
    if (itMaxHp == g_maxHealthMap.end() || actor->health > itMaxHp->second) {
        g_maxHealthMap[id] = actor->health;
    }
    s16 maxHp = g_maxHealthMap[id];
    if (maxHp <= 0) maxHp = actor->health;

    // Bosses and midbosses go to the top HUD bar when the setting is on
    if (isEnemy && (cat == ENEMY_ENDBOSS || cat == ENEMY_MIDBOSS)) {
        if (g_configHpBarsBossesEnabled) {
            bool isBattleActive = (actor->attention_info.flags & fopAc_AttnFlag_BATTLE_e) != 0;
            if (isBattleActive) {
                if (!ctx->activeBoss || cat > ctx->bossCategory) {
                    ctx->activeBoss = actor;
                    ctx->bossCategory = cat;
                }
            }
        }
        return 0;
    }

    if (!g_configHpBarsEnabled) {
        return 0;
    }

    f32 targetAlpha = 0.0f;
    if (isEnemy && ctx && ctx->player) {
        f32 dist = fopAcM_searchActorDistance(ctx->player, actor);
        const f32 maxDist = 3000.0f;
        const f32 fadeDist = 600.0f;

        if (dist < maxDist) {
            if (dist > (maxDist - fadeDist)) {
                targetAlpha = (maxDist - dist) / fadeDist;
            } else {
                targetAlpha = 1.0f;
            }
        }
    }

    f32 currentAlpha = 0.0f;
    auto itAlpha = g_enemyAlphaMap.find(id);
    if (itAlpha != g_enemyAlphaMap.end()) {
        currentAlpha = itAlpha->second;
    }

    currentAlpha += (targetAlpha - currentAlpha) * 0.15f;
    if (currentAlpha < 0.005f) {
        g_enemyAlphaMap.erase(id);
        return 0;
    }
    g_enemyAlphaMap[id] = currentAlpha;

    // Project enemy position to screen space
    cXyz pos = actor->attention_info.position;
    if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f) {
        pos = actor->current.pos;
    }
    pos.y += ctx->cursorOffsetY;

    Vec screenPos;
    mDoLib_project(&pos, &screenPos);

    if (screenPos.z >= 400000.0f) {
        return 0;
    }

    f32 screenW = mDoGph_gInf_c::getWidthF();
    f32 screenH = mDoGph_gInf_c::getHeightF();
    if (screenW <= 0.0f) screenW = 640.0f;
    if (screenH <= 0.0f) screenH = 480.0f;

    if (screenPos.x < -100.0f || screenPos.x > screenW + 100.0f ||
        screenPos.y < -100.0f || screenPos.y > screenH + 100.0f) {
        return 0;
    }

    f32 hpRatio = static_cast<f32>(actor->health) / static_cast<f32>(maxHp);
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;

    u8 r, g, b;
    if (hpRatio > 0.5f) {
        f32 factor = (hpRatio - 0.5f) * 2.0f;
        r = static_cast<u8>((1.0f - factor) * 230.0f + factor * 30.0f);
        g = static_cast<u8>(factor * 190.0f + (1.0f - factor) * 170.0f);
        b = static_cast<u8>(factor * 70.0f + (1.0f - factor) * 30.0f);
    } else {
        f32 factor = hpRatio * 2.0f;
        r = static_cast<u8>((1.0f - factor) * 220.0f + factor * 230.0f);
        g = static_cast<u8>(factor * 170.0f);
        b = static_cast<u8>((1.0f - factor) * 40.0f + factor * 30.0f);
    }

    const f32 barWidth = 25.0f;
    const f32 barHeight = 1.0f;
    const f32 drawX = screenPos.x - (barWidth * 0.5f);
    const f32 drawY = screenPos.y - (barHeight * 0.5f);

    auto getAlpha = [currentAlpha](u8 baseAlpha) -> u8 {
        return static_cast<u8>(static_cast<f32>(baseAlpha) * currentAlpha);
    };

    J2DFillBox(drawX - 1.5f, drawY - 1.5f, barWidth + 3.0f, barHeight + 3.0f, JUtility::TColor(0, 0, 0, getAlpha(160)));
    J2DDrawFrame(drawX - 1.0f, drawY - 1.0f, barWidth + 2.0f, barHeight + 2.0f, JUtility::TColor(170, 135, 55, getAlpha(230)), 1);
    J2DFillBox(drawX, drawY, barWidth, barHeight, JUtility::TColor(12, 12, 16, getAlpha(230)));

    if (hpRatio > 0.0f) {
        f32 fillWidth = barWidth * hpRatio;
        J2DFillBox(drawX, drawY, fillWidth, barHeight, JUtility::TColor(r, g, b, getAlpha(240)));
        J2DFillBox(drawX, drawY, fillWidth, 1.0f, JUtility::TColor(255, 255, 255, getAlpha(80)));
    }

    // Tick marks to show individual HP segments (only when count is small enough to be readable)
    if (maxHp > 1 && maxHp <= 20) {
        f32 step = barWidth / static_cast<f32>(maxHp);
        for (int i = 1; i < maxHp; i++) {
            f32 tickX = drawX + static_cast<f32>(i) * step;
            J2DFillBox(tickX - 0.5f, drawY, 1.0f, barHeight, JUtility::TColor(0, 0, 0, getAlpha(140)));
        }
    }

    if (g_configHpBarsShowNumbers) {
        char hpText[32];
        std::snprintf(hpText, sizeof(hpText), "%d/%d", actor->health, maxHp);
        const f32 fontW = 5.2f;
        const f32 fontH = 6.2f;
        f32 textWidth = get_text_width_ingame(hpText, fontW);
        f32 textX = drawX + (barWidth - textWidth) * 0.5f;
        f32 textY = drawY - 4.5f;

        draw_text_ingame(hpText, textX, textY, fontW, fontH, JUtility::TColor(255, 245, 210, getAlpha(255)), true);
    }

    return 0;
}

static void on_meter2_draw_post(ModContext*, void*, void*, void*) {
    if (!g_configHpBarsEnabled && !g_configHpBarsBossesEnabled) {
        return;
    }

    if (dComIfGp_isPauseFlag() || dScnPly_c::isPause()) {
        return;
    }

    if (dComIfGp_getEvent() && dComIfGp_getEvent()->runCheck()) {
        return;
    }

    fopAc_ac_c* player = dComIfGp_getPlayer(0);
    if (!player) {
        return;
    }

    DrawHpBarContext ctx;
    ctx.player = player;
    ctx.cursorOffsetY = 10.0f;
    ctx.activeBoss = nullptr;
    ctx.bossCategory = ENEMY_NORMAL;

    fopAcIt_Executor(reinterpret_cast<fopAcIt_ExecutorFunc>(drawEnemyHpBarCallback), &ctx);

    static f32 s_bossAlpha = 0.0f;

    f32 targetBossAlpha = 0.0f;
    if (g_configHpBarsBossesEnabled && ctx.activeBoss && ctx.activeBoss->health > 0) {
        targetBossAlpha = 1.0f;
    }

    s_bossAlpha += (targetBossAlpha - s_bossAlpha) * 0.15f;

    if (g_configHpBarsBossesEnabled && s_bossAlpha > 0.005f && ctx.activeBoss) {
        drawTopHudBossBar(ctx.activeBoss, ctx.bossCategory, s_bossAlpha);
    }
}

ModResult init_hp_bars(const HookService* hook_svc, ModError* error) {
    if (!hook_svc) {
        return MOD_OK;
    }
    return mods::hook_add_post<Meter2Draw>(hook_svc, on_meter2_draw_post);
}

void shutdown_hp_bars() {
    g_maxHealthMap.clear();
    g_enemyAlphaMap.clear();
}
