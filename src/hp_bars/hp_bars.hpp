#pragma once
#include <cstdint>
#include "mods/api.h"

struct HookService;

extern bool g_configHpBarsEnabled;
extern bool g_configHpBarsShowNumbers;

ModResult init_hp_bars(const HookService* hook_svc, ModError* error);
void shutdown_hp_bars();
