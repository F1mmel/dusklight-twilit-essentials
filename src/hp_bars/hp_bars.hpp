#pragma once
#include <cstdint>
#include "mods/api.h"

struct HookService;
struct LogService;

extern bool g_configHpBarsEnabled;
extern bool g_configHpBarsShowNumbers;
extern bool g_configDamageNumbersEnabled;

ModResult init_hp_bars(const HookService* hook_svc, ModError* error);
void update_hp_bars(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_hp_bars();
