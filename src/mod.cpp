#include "hp_bars/hp_bars.hpp"
#include "visible_equipment/visible_equipment.hpp"
#include "damage_numbers/damage_numbers.hpp"
#include "custom_z_button/custom_z_button.hpp"

#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/config.h"
#include "mods/svc/ui.h"

#include "d/actor/d_a_title.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DTextBox.h"

DEFINE_HOOK(&dDlst_daTitle_c::draw, DlstTitleDrawHook);

static HookAction on_title_draw_pre(ModContext*, void* args, void*, void*) {
    if (!args) return HOOK_CONTINUE;
    dDlst_daTitle_c* titleDraw = mods::arg<dDlst_daTitle_c*>(args, 0);
    if (!titleDraw || !titleDraw->Scr) return HOOK_CONTINUE;

    J2DTextBox* modShadow = (J2DTextBox*)titleDraw->Scr->search(MULTI_CHAR('m_mshd'));
    J2DTextBox* modText = (J2DTextBox*)titleDraw->Scr->search(MULTI_CHAR('m_mod'));

    if (modText == nullptr) {
        JUTFont* font = mDoExt_getSubFont();
        if (font == nullptr) {
            font = mDoExt_getMesgFont();
        }
        ResFONT* resFont = (font != nullptr) ? font->getResFont() : nullptr;

        JGeometry::TBox2<f32> shadowBox(-18.5f, 411.5f, 621.5f, 451.5f);
        modShadow = JKR_NEW J2DTextBox(
            MULTI_CHAR('m_mshd'),
            shadowBox,
            resFont,
            "Twilit Essentials v1.0.1",
            64,
            HBIND_CENTER,
            VBIND_CENTER
        );
        if (modShadow != nullptr) {
            if (font != nullptr) modShadow->setFont(font);
            modShadow->setFontSize(16.0f, 16.0f);
            modShadow->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 220));
            modShadow->setFontColor(JUtility::TColor(0, 0, 0, 220), JUtility::TColor(0, 0, 0, 220));
            titleDraw->Scr->appendChild(modShadow);
        }

        JGeometry::TBox2<f32> box(-20.0f, 410.0f, 620.0f, 450.0f);
        modText = JKR_NEW J2DTextBox(
            MULTI_CHAR('m_mod'),
            box,
            resFont,
            "Twilit Essentials v1.0.1",
            64,
            HBIND_CENTER,
            VBIND_CENTER
        );
        if (modText != nullptr) {
            if (font != nullptr) {
                modText->setFont(font);
            }
            modText->setFontSize(16.0f, 16.0f);
            modText->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(210, 120, 255, 255));
            modText->setFontColor(JUtility::TColor(210, 120, 255, 255), JUtility::TColor(170, 70, 220, 255));
            titleDraw->Scr->appendChild(modText);
        }
    }

    // Sync alpha with n_all so the watermark fades in together with the Press Start logo
    J2DPane* nAll = titleDraw->Scr->search(MULTI_CHAR('n_all'));
    if (nAll != nullptr) {
        u8 alpha = nAll->getAlpha();
        if (modText != nullptr) {
            modText->setAlpha(alpha);
        }
        if (modShadow != nullptr) {
            modShadow->setAlpha((u8)((u32)alpha * 220 / 255));
        }
    }
    return HOOK_CONTINUE;
}

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_OPTIONAL_SERVICE(ConfigService, svc_config);
IMPORT_OPTIONAL_SERVICE(UiService, svc_ui);

// Retain all metadata records so MSVC cl.exe does not eliminate them
extern "C" MOD_EXPORT const void* const g_keep_mod_records[] = {
    &mod_meta_header_record,
    &mod_meta_import_svc_log,
    &mod_meta_import_svc_hook,
    &mod_meta_import_svc_config,
    &mod_meta_import_svc_ui,
};

static ConfigVarHandle s_varHpBars = 0;
static ConfigVarHandle s_varHpBarsShowNumbers = 0;
static ConfigVarHandle s_varVisibleEquip = 0;
static ConfigVarHandle s_varVisibleEquipMode = 0;
static ConfigVarHandle s_varVisibleEquipMirrorBow = 0;
static ConfigVarHandle s_varDamageNumbers = 0;
static ConfigVarHandle s_varCustomZButton = 0;

static void on_hp_bars_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configHpBarsEnabled = value->bool_value;
    }
}

static void on_hp_bars_show_numbers_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configHpBarsShowNumbers = value->bool_value;
    }
}

static void on_visible_equip_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configVisibleEquipmentEnabled = value->bool_value;
    }
}

static void on_visible_equip_mode_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configVisibleEquipDisplayMode = value->bool_value ? 1 : 0;
    }
}

static void on_visible_equip_mirror_bow_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configVisibleEquipMirrorBow = value->bool_value;
    }
}

static void on_damage_numbers_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configDamageNumbersEnabled = value->bool_value;
    }
}

static void on_custom_z_button_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configCustomZButtonEnabled = value->bool_value;
    }
}

static ModResult build_mod_ui_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    if (!svc_ui) return MOD_OK;

    // --- Section 1: Enemy HP Bars ---
    svc_ui->pane_add_section(mod_ctx, panel, "Enemy HP Bars");
    svc_ui->pane_add_text(mod_ctx, panel, "Displays dynamic overhead health bars for active enemies.", nullptr);
    if (s_varHpBars != 0) {
        UiControlDesc ctrlHp = UI_CONTROL_DESC_INIT;
        ctrlHp.kind = UI_CONTROL_TOGGLE;
        ctrlHp.label = "Normal Enemy HP Bars";
        ctrlHp.help_rml = "Toggle overhead 3D HP bars over active hostile enemies (Off by default).";
        ctrlHp.binding = UI_BINDING_CONFIG_VAR;
        ctrlHp.config_var = s_varHpBars;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHp, nullptr);
    }

    if (s_varHpBarsShowNumbers != 0) {
        UiControlDesc ctrlHpNum = UI_CONTROL_DESC_INIT;
        ctrlHpNum.kind = UI_CONTROL_TOGGLE;
        ctrlHpNum.label = "Show HP Numbers";
        ctrlHpNum.help_rml = "Display numerical health values (e.g. 12/20) above enemy health bars (Off by default).";
        ctrlHpNum.binding = UI_BINDING_CONFIG_VAR;
        ctrlHpNum.config_var = s_varHpBarsShowNumbers;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHpNum, nullptr);
    }

    // --- Section 2: Floating 3D Damage Numbers ---
    svc_ui->pane_add_section(mod_ctx, panel, "Floating 3D Damage Numbers");
    svc_ui->pane_add_text(mod_ctx, panel, "Displays animated floating 3D damage numbers when dealing damage to enemies.", nullptr);
    if (s_varDamageNumbers != 0) {
        UiControlDesc ctrlDmg = UI_CONTROL_DESC_INIT;
        ctrlDmg.kind = UI_CONTROL_TOGGLE;
        ctrlDmg.label = "Status";
        ctrlDmg.help_rml = "Toggle floating animated damage numbers when hitting enemies.";
        ctrlDmg.binding = UI_BINDING_CONFIG_VAR;
        ctrlDmg.config_var = s_varDamageNumbers;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlDmg, nullptr);
    }

    // --- Section 3: Visible Equipment on Link ---
    svc_ui->pane_add_section(mod_ctx, panel, "Visible Equipment on Link");
    svc_ui->pane_add_text(mod_ctx, panel, "Renders Link's equipped weapons, quivers, and gear directly on his back model.", nullptr);
    if (s_varVisibleEquip != 0) {
        UiControlDesc ctrlEquip = UI_CONTROL_DESC_INIT;
        ctrlEquip.kind = UI_CONTROL_TOGGLE;
        ctrlEquip.label = "Status";
        ctrlEquip.help_rml = "Toggle rendering equipped weapons, quivers, and gear on Link's back.";
        ctrlEquip.binding = UI_BINDING_CONFIG_VAR;
        ctrlEquip.config_var = s_varVisibleEquip;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlEquip, nullptr);
    }

    if (s_varVisibleEquipMode != 0) {
        UiControlDesc ctrlEquipMode = UI_CONTROL_DESC_INIT;
        ctrlEquipMode.kind = UI_CONTROL_TOGGLE;
        ctrlEquipMode.label = "Show Always Once Unlocked";
        ctrlEquipMode.help_rml = "Off: Show equipment on back only when assigned to an item button (X/Y/Z).\nOn: Show equipment permanently on back once unlocked in Item Wheel.";
        ctrlEquipMode.binding = UI_BINDING_CONFIG_VAR;
        ctrlEquipMode.config_var = s_varVisibleEquipMode;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlEquipMode, nullptr);
    }

    if (s_varVisibleEquipMirrorBow != 0) {
        UiControlDesc ctrlEquipMirror = UI_CONTROL_DESC_INIT;
        ctrlEquipMirror.kind = UI_CONTROL_TOGGLE;
        ctrlEquipMirror.label = "Mirror Bow Angle on Back";
        ctrlEquipMirror.help_rml = "Off: Bow hangs diagonally from Top-Right to Bottom-Left.\nOn: Bow hangs diagonally from Top-Left to Bottom-Right.";
        ctrlEquipMirror.binding = UI_BINDING_CONFIG_VAR;
        ctrlEquipMirror.config_var = s_varVisibleEquipMirrorBow;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlEquipMirror, nullptr);
    }

    // --- Section 4: Custom Z Button ---
    svc_ui->pane_add_section(mod_ctx, panel, "Custom Z Button");
    svc_ui->pane_add_text(mod_ctx, panel, "Allows assigning inventory items to the Z button and moves Midna calls to D-Pad Left.", nullptr);
    if (s_varCustomZButton != 0) {
        UiControlDesc ctrlZ = UI_CONTROL_DESC_INIT;
        ctrlZ.kind = UI_CONTROL_TOGGLE;
        ctrlZ.label = "Status";
        ctrlZ.help_rml = "Toggle custom Z button item equipping and D-Pad Left Midna call.";
        ctrlZ.binding = UI_BINDING_CONFIG_VAR;
        ctrlZ.config_var = s_varCustomZButton;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlZ, nullptr);
    }

    return MOD_OK;
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if (svc_hook == nullptr) {
        return mods::set_error(error, MOD_ERROR, "HookService unavailable");
    }

    if (svc_config) {
        ConfigVarDesc descHp = CONFIG_VAR_DESC_INIT;
        descHp.name = "hpBarsEnabled";
        descHp.type = CONFIG_VAR_BOOL;
        descHp.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descHp, &s_varHpBars) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varHpBars, &g_configHpBarsEnabled);
            svc_config->subscribe(mod_ctx, s_varHpBars, on_hp_bars_changed, nullptr, nullptr);
        }

        ConfigVarDesc descHpNum = CONFIG_VAR_DESC_INIT;
        descHpNum.name = "hpBarsShowNumbers";
        descHpNum.type = CONFIG_VAR_BOOL;
        descHpNum.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descHpNum, &s_varHpBarsShowNumbers) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varHpBarsShowNumbers, &g_configHpBarsShowNumbers);
            svc_config->subscribe(mod_ctx, s_varHpBarsShowNumbers, on_hp_bars_show_numbers_changed, nullptr, nullptr);
        }

        g_configHpBarsBossesEnabled = false;

        ConfigVarDesc descDmg = CONFIG_VAR_DESC_INIT;
        descDmg.name = "damageNumbersEnabled";
        descDmg.type = CONFIG_VAR_BOOL;
        descDmg.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descDmg, &s_varDamageNumbers) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varDamageNumbers, &g_configDamageNumbersEnabled);
            svc_config->subscribe(mod_ctx, s_varDamageNumbers, on_damage_numbers_changed, nullptr, nullptr);
        }

        ConfigVarDesc descEquip = CONFIG_VAR_DESC_INIT;
        descEquip.name = "visibleEquipmentEnabled";
        descEquip.type = CONFIG_VAR_BOOL;
        descEquip.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descEquip, &s_varVisibleEquip) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varVisibleEquip, &g_configVisibleEquipmentEnabled);
            svc_config->subscribe(mod_ctx, s_varVisibleEquip, on_visible_equip_changed, nullptr, nullptr);
        }

        ConfigVarDesc descEquipMode = CONFIG_VAR_DESC_INIT;
        descEquipMode.name = "visibleEquipmentAlwaysShowUnlocked";
        descEquipMode.type = CONFIG_VAR_BOOL;
        descEquipMode.default_bool = true;
        if (svc_config->register_var(mod_ctx, &descEquipMode, &s_varVisibleEquipMode) == MOD_OK) {
            bool modeBool = true;
            svc_config->get_bool(mod_ctx, s_varVisibleEquipMode, &modeBool);
            g_configVisibleEquipDisplayMode = modeBool ? 1 : 0;
            svc_config->subscribe(mod_ctx, s_varVisibleEquipMode, on_visible_equip_mode_changed, nullptr, nullptr);
        }

        ConfigVarDesc descEquipMirror = CONFIG_VAR_DESC_INIT;
        descEquipMirror.name = "visibleEquipmentMirrorBow";
        descEquipMirror.type = CONFIG_VAR_BOOL;
        descEquipMirror.default_bool = true;
        if (svc_config->register_var(mod_ctx, &descEquipMirror, &s_varVisibleEquipMirrorBow) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varVisibleEquipMirrorBow, &g_configVisibleEquipMirrorBow);
            svc_config->subscribe(mod_ctx, s_varVisibleEquipMirrorBow, on_visible_equip_mirror_bow_changed, nullptr, nullptr);
        }

        ConfigVarDesc descZ = CONFIG_VAR_DESC_INIT;
        descZ.name = "customZButtonEnabled";
        descZ.type = CONFIG_VAR_BOOL;
        descZ.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descZ, &s_varCustomZButton) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varCustomZButton, &g_configCustomZButtonEnabled);
            svc_config->subscribe(mod_ctx, s_varCustomZButton, on_custom_z_button_changed, nullptr, nullptr);
        }
    }

    if (svc_ui) {
        UiModsPanelDesc panelDesc = UI_MODS_PANEL_DESC_INIT;
        panelDesc.build = build_mod_ui_panel;
        svc_ui->register_mods_panel(mod_ctx, &panelDesc);
    }

    if (svc_hook) {
        mods::hook_add_pre<DlstTitleDrawHook>(svc_hook, on_title_draw_pre);
    }

    ModResult res = init_hp_bars(svc_hook, error);
    if (res != MOD_OK) {
        if (svc_log) svc_log->error(mod_ctx, "failed to initialize hp_bars sub-mod");
        return res;
    }

    init_damage_numbers(svc_hook, error);
    init_visible_equipment(svc_hook, error);
    init_custom_z_button(svc_hook, error);

    if (svc_log) svc_log->info(mod_ctx, "dusklight_twilit_essentials main dispatcher initialized successfully");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    update_damage_numbers(svc_log, mod_ctx);
    update_visible_equipment(svc_log, mod_ctx);
    update_custom_z_button(svc_log, mod_ctx);
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    shutdown_hp_bars();
    shutdown_damage_numbers();
    shutdown_visible_equipment();
    shutdown_custom_z_button();
    return MOD_OK;
}
}
