#include "sheathed_spin.hpp"
#include "m_Do/m_Do_audio.h"

bool g_configSheathedSpinEnabled = false;

DEFINE_HOOK(&daAlink_c::checkItemAction, SheathedSpinItemActionHook);

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

    // Wolf Link
    if (link->checkWolf()) {
        if (link->checkCutTurnInputTrigger()) {
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
            !link->checkCanoeRide() &&
            link->checkCutTurnInputTrigger())
        {
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
