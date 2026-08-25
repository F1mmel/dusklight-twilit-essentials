#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/hook.hpp"

extern bool g_configCustomZButtonEnabled;
extern bool g_configZButtonEnabled;

bool isNativeZButtonEngine();
ModResult init_z_button(const HookService* hook_svc, const LogService* log_svc, ModContext* mod_ctx, ModError* error);
void update_z_button(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_z_button();
