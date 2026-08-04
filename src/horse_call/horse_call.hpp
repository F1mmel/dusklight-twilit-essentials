#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/hook.hpp"

extern bool g_configDpadHorseCallEnabled;
extern bool g_configDpadHorseCallAllowAnytime;

ModResult init_horse_call(const HookService* hook_svc, ModError* error);
void update_horse_call(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_horse_call();
