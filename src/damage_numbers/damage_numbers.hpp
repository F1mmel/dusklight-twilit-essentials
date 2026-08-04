#pragma once
#include <cstdint>
#include "mods/api.h"

struct HookService;
struct LogService;

extern bool g_configDamageNumbersEnabled;

ModResult init_damage_numbers(const HookService* hook_svc, ModError* error);
void update_damage_numbers(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_damage_numbers();
