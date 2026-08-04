#ifndef VISIBLE_EQUIPMENT_HPP
#define VISIBLE_EQUIPMENT_HPP

#include <cstdint>
#include "mods/api.h"

struct HookService;
struct LogService;
struct ModContext;

extern bool g_configVisibleEquipmentEnabled;
extern int g_configVisibleEquipDisplayMode; // 0 = When Equipped, 1 = Permanently (When Unlocked)
extern bool g_configVisibleEquipMirrorBow;  // false = Top-Right to Bottom-Left, true = Top-Left to Bottom-Right

ModResult init_visible_equipment(const HookService* hook_svc, ModError* err);
void update_visible_equipment(const LogService* log_svc, ModContext* mod_ctx);
void draw_visible_equipment(const LogService* log_svc, ModContext* mod_ctx);
void shutdown_visible_equipment();

#endif // VISIBLE_EQUIPMENT_HPP
