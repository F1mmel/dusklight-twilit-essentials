#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/hook.hpp"

#include "d/actor/d_a_alink.h"

extern bool g_configSheathedSpinEnabled;

ModResult init_sheathed_spin(const HookService* hook_svc);
void update_sheathed_spin(const LogService* log_svc, ModContext* mod_ctx);
