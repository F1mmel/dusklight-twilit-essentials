#pragma once

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/hook.hpp"

extern bool g_configCustomZButtonEnabled;

ModResult init_custom_z_button(const HookService* hook_svc, ModError* error);
void update_custom_z_button(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_custom_z_button();
