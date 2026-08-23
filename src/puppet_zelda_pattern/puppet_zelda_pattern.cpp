#include "puppet_zelda_pattern.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_hzelda.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"
#include "SSystem/SComponent/c_math.h"

bool g_configPuppetZeldaPatternEnabled = false;
bool g_configPuppetZeldaAlwaysShortest = false;

enum {
    ACTION_WAIT = 0,
    ACTION_ATTACK_A = 1,  // Sword Dive Attack
    ACTION_ATTACK_B = 2,  // Triangle Ground Attack
    ACTION_ATTACK_C = 3,  // Energy Ball Throw Attack
    ACTION_DAMAGE = 4,
};

static fpc_ProcID s_lastZeldaId = fpcM_ERROR_PROCESS_ID_e;
static s16 s_prevAction = ACTION_WAIT;
static int s_zeldaPatternStep = 0;

// 7-step deterministic cycle:
// Step 0: Energy Ball
// Step 1: 1 Attack
// Step 2: Energy Ball
// Step 3: Attack (1 of 3)
// Step 4: Attack (2 of 3)
// Step 5: Attack (3 of 3)
// Step 6: Energy Ball
static const bool s_isBallStep[7] = {
    true,   // 0: Energy Ball
    false,  // 1: 1 Attack
    true,   // 2: Energy Ball
    false,  // 3: Attack (1 of 3)
    false,  // 4: Attack (2 of 3)
    false,  // 5: Attack (3 of 3)
    true    // 6: Energy Ball
};

ModResult init_puppet_zelda_pattern(const HookService*, ModError*) {
    s_lastZeldaId = fpcM_ERROR_PROCESS_ID_e;
    s_prevAction = ACTION_WAIT;
    s_zeldaPatternStep = 0;
    return MOD_OK;
}

void update_puppet_zelda_pattern(const LogService* svc_log, ModContext* mod_ctx) {
    if (!g_configPuppetZeldaPatternEnabled) {
        return;
    }

    e_hzelda_class* zelda = (e_hzelda_class*)fopAcM_SearchByName(fpcNm_E_HZELDA_e);
    if (zelda == nullptr) {
        s_lastZeldaId = fpcM_ERROR_PROCESS_ID_e;
        s_prevAction = ACTION_WAIT;
        s_zeldaPatternStep = 0;
        return;
    }

    fpc_ProcID zeldaId = fopAcM_GetID(zelda);
    if (zeldaId != s_lastZeldaId) {
        s_lastZeldaId = zeldaId;
        s_prevAction = ACTION_WAIT;
        s_zeldaPatternStep = 0;
        if (svc_log && mod_ctx) {
            svc_log->info(mod_ctx, "[PuppetZelda] New Puppet Zelda fight detected, resetting 7-cycle pattern to step 0");
        }
    }

    if (s_prevAction == ACTION_WAIT && zelda->mAction != ACTION_WAIT && zelda->mAction != ACTION_DAMAGE) {
        int currentStep = s_zeldaPatternStep;
        if (s_isBallStep[currentStep]) {
            zelda->mAction = ACTION_ATTACK_C;
            if (svc_log && mod_ctx) {
                char msg[128];
                snprintf(msg, sizeof(msg), "[PuppetZelda] Step %d/7: Forced Energy Ball", currentStep + 1);
                svc_log->info(mod_ctx, msg);
            }
        } else {
            if (g_configPuppetZeldaAlwaysShortest) {
                zelda->mAction = ACTION_ATTACK_A; // Always Sword Dive (Shortest Attack)
            } else {
                zelda->mAction = (cM_rndF(1.0f) < 0.5f) ? ACTION_ATTACK_A : ACTION_ATTACK_B;
            }

            if (svc_log && mod_ctx) {
                char msg[128];
                snprintf(msg, sizeof(msg), "[PuppetZelda] Step %d/7: Non-Ball Attack (%s)",
                         currentStep + 1,
                         (zelda->mAction == ACTION_ATTACK_A) ? "Sword Dive" : "Triangle");
                svc_log->info(mod_ctx, msg);
            }
        }
        s_zeldaPatternStep = (s_zeldaPatternStep + 1) % 7;
    }

    s_prevAction = zelda->mAction;
}
