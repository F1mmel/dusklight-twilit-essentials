#include "sheathed_spin.hpp"
#include "m_Do/m_Do_audio.h"
#include <cmath>

bool g_configSheathedSpinEnabled = false;

DEFINE_HOOK(&daAlink_c::checkItemAction, SheathedSpinItemActionHook);

// Touch- and Stick-Tracking
static s16 s_prevStickAngle = 0;
static int s_touchSpinAccum = 0;
static int s_touchSpinReadyTimer = 0;
static int s_sheathedHoldChargeTimer = 0;

static bool is_spin_triggered(daAlink_c* link) {
    if (!link) return false;

    // 1. Vanilla Controller 360-degree rotation
    if (link->checkCutTurnInputTrigger()) {
        return true;
    }

    // 2. Dusklight Spin-Flag (e.g. Gestures / Motion / Shake)
    if (link->checkResetFlg0(daAlink_c::RFLG0_UNK_40)) {
        return true;
    }

    // 3. Touch-Stick buffer (B pressed shortly after a circular motion)
    if (s_touchSpinReadyTimer > 0 && link->swordSwingTrigger()) {
        s_touchSpinReadyTimer = 0;
        return true;
    }

    return false;
}

static HookAction on_check_item_action_pre(ModContext*, void* args, void* retval, void*) {
    if (!g_configSheathedSpinEnabled || !args || !retval) {
        return HOOK_CONTINUE;
    }

    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr ||
        link->checkKandelaarSwingAnime() || link->checkCopyRodThrowAnime() ||
        link->checkBoomerangThrowAnime())
    {
        return HOOK_CONTINUE;
    }

    // Update touch-stick rotation tracking
    if (link->mStickValue > 0.35f) {
        s16 angleDiff = link->mStickAngle - s_prevStickAngle;
        if (std::abs(angleDiff) > 0x0600 && std::abs(angleDiff) < 0x7500) {
            if ((s_touchSpinAccum > 0 && angleDiff > 0) || (s_touchSpinAccum < 0 && angleDiff < 0) || s_touchSpinAccum == 0) {
                s_touchSpinAccum += angleDiff;
            } else {
                s_touchSpinAccum = angleDiff;
            }

            // Arm spin trigger after ~180-240 degree circular swipe on touchscreen
            if (std::abs(s_touchSpinAccum) >= 0x7000) {
                s_touchSpinReadyTimer = 8; // 8 frames window to press attack
                s_touchSpinAccum = 0;
            }
        }
        s_prevStickAngle = link->mStickAngle;
    } else {
        s_touchSpinAccum = 0;
        s_prevStickAngle = link->mStickAngle;
    }

    if (s_touchSpinReadyTimer > 0) {
        s_touchSpinReadyTimer--;
    }

    // Wolf Link
    if (link->checkWolf()) {
        if (is_spin_triggered(link)) {
            BOOL result = link->procWolfRollAttackInit(2, 0);
            *static_cast<BOOL*>(retval) = result;
            return HOOK_SKIP_ORIGINAL;
        }
        return HOOK_CONTINUE;
    }

    // Human Link
    if (link->mEquipItem != 0x103 || link->checkEquipAnime()) {
        if (dComIfGs_getSelectEquipSword() != dItemNo_NONE_e &&
            !link->checkNotBattleStage() &&
            !link->checkCanoeRide())
        {
            // Instant spin attack via stick rotation / touch gesture
            if (is_spin_triggered(link)) {
                link->swordEquip(TRUE);
                link->setSwordModel();
                mDoAud_seStart(Z2SE_AL_SWORD_PULLOUT, NULL, 0, 0);

                BOOL result = FALSE;
                if (link->checkBoardRide()) {
                    result = link->procBoardCutTurnInit();
                } else if (link->checkReinRide()) {
                    result = link->procHorseCutTurnInit();
                } else {
                    result = link->procCutTurnInit(1, 2);
                }
                *static_cast<BOOL*>(retval) = result;
                return HOOK_SKIP_ORIGINAL;
            }

            // Charge spin attack by holding B while sheathed
            if (link->swordButton()) {
                s_sheathedHoldChargeTimer++;
                if (s_sheathedHoldChargeTimer >= 3) {
                    link->swordEquip(TRUE);
                    link->setSwordModel();
                    mDoAud_seStart(Z2SE_AL_SWORD_PULLOUT, NULL, 0, 0);

                    BOOL result = FALSE;
                    if (link->checkReinRide()) {
                        result = link->procHorseCutChargeReadyInit();
                    } else if (!link->checkBoardRide()) {
                        result = link->procCutTurnChargeInit();
                    }
                    if (result) {
                        *static_cast<BOOL*>(retval) = result;
                        return HOOK_SKIP_ORIGINAL;
                    }
                }
            } else {
                s_sheathedHoldChargeTimer = 0;
            }
        }
    } else {
        s_sheathedHoldChargeTimer = 0;
    }

    return HOOK_CONTINUE;
}

ModResult init_sheathed_spin(const HookService* hook_svc) {
    if (!hook_svc) return MOD_ERROR;
    mods::hook::add_pre<SheathedSpinItemActionHook>(hook_svc, on_check_item_action_pre);
    return MOD_OK;
}

void update_sheathed_spin(const LogService*, ModContext*) {
}

