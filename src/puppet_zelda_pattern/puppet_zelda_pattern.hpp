#pragma once

#include "mods/svc/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"

extern bool g_configPuppetZeldaPatternEnabled;
extern bool g_configPuppetZeldaAlwaysShortest;

ModResult init_puppet_zelda_pattern(const HookService* svc_hook, ModError* error);
void update_puppet_zelda_pattern(const LogService* svc_log, ModContext* mod_ctx);
