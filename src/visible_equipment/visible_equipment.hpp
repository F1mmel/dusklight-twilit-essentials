#ifndef VISIBLE_EQUIPMENT_HPP
#define VISIBLE_EQUIPMENT_HPP

#include <cstdint>
#include "mods/api.h"

struct HookService;
struct LogService;
struct ModContext;

extern bool g_configVisibleEquipmentEnabled;
extern int g_configVisibleEquipDisplayMode;
extern bool g_configVisibleEquipMirrorBow;

extern bool g_configVisibleEquipShowBow;
extern bool g_configVisibleEquipShowLantern;
extern bool g_configVisibleEquipShowHorseCall;

ModResult init_visible_equipment(const HookService* hook_svc, ModError* err);
void update_visible_equipment(const LogService* log_svc, ModContext* mod_ctx);
void draw_visible_equipment(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_visible_equipment();

#endif // VISIBLE_EQUIPMENT_HPP
