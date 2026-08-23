#include "hp_bars/hp_bars.hpp"
#include "visible_equipment/visible_equipment.hpp"
#include "damage_numbers/damage_numbers.hpp"
#include "z_button/z_button.hpp"
#include "horse_call/horse_call.hpp"
#include "sheathed_spin/sheathed_spin.hpp"
#include "puppet_zelda_pattern/puppet_zelda_pattern.hpp"
#include "update_service.hpp"

#include "mods/svc/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/config.h"
#include "mods/svc/ui.h"
#include "mods/svc/resource.h"
#include "mods/svc/texture.h"

#include "d/actor/d_a_title.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DTextBox.h"

#define MOD_TITLE_VERSION_TEXT "Twilit Essentials v" TWILIT_ESSENTIALS_VERSION

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
            MOD_TITLE_VERSION_TEXT,
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
            MOD_TITLE_VERSION_TEXT,
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

    // Match title screen alpha
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
IMPORT_OPTIONAL_SERVICE(ResourceService, svc_resource);
IMPORT_OPTIONAL_SERVICE(TextureService, svc_texture);

// Prevent dead-stripping of mod exports
extern "C" MOD_EXPORT const void* const g_keep_mod_records[] = {
    &mod_meta_header_record,
    &mod_meta_import_svc_log,
    &mod_meta_import_svc_hook,
    &mod_meta_import_svc_config,
    &mod_meta_import_svc_ui,
    &mod_meta_import_svc_resource,
    &mod_meta_import_svc_texture,
};

static ConfigVarHandle s_varHpBars = 0;
static ConfigVarHandle s_varHpBarsShowNumbers = 0;
static ConfigVarHandle s_varVisibleEquip = 0;
static ConfigVarHandle s_varVisibleEquipMode = 0;
static ConfigVarHandle s_varVisibleEquipMirrorBow = 0;
static ConfigVarHandle s_varVisibleEquipShowBow = 0;
static ConfigVarHandle s_varVisibleEquipShowLantern = 0;
static ConfigVarHandle s_varVisibleEquipShowHorseCall = 0;
static ConfigVarHandle s_varDamageNumbers = 0;
static ConfigVarHandle s_varCustomZButton = 0;
static ConfigVarHandle s_varDpadHorseCall = 0;
static ConfigVarHandle s_varDpadHorseCallRequireEquipped = 0;
static ConfigVarHandle s_varDpadHorseCallAllowAnytime = 0;
static ConfigVarHandle s_varSheathedSpin = 0;
static ConfigVarHandle s_varPuppetZeldaPattern = 0;
static ConfigVarHandle s_varPuppetZeldaAlwaysShortest = 0;
static ConfigVarHandle s_varCheckForUpdates = 0;

static void on_sheathed_spin_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configSheathedSpinEnabled = value->bool_value;
    }
}

static void on_puppet_zelda_pattern_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configPuppetZeldaPatternEnabled = value->bool_value;
    }
}

static void on_puppet_zelda_always_shortest_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configPuppetZeldaAlwaysShortest = value->bool_value;
    }
}

static void on_check_for_updates_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configCheckForUpdatesEnabled = value->bool_value;
    }
}

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

static void on_visible_equip_show_bow_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configVisibleEquipShowBow = value->bool_value;
    }
}

static void on_visible_equip_show_lantern_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configVisibleEquipShowLantern = value->bool_value;
    }
}

static void on_visible_equip_show_horse_call_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configVisibleEquipShowHorseCall = value->bool_value;
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
        g_configZButtonEnabled = value->bool_value;
        if (svc_log) {
            svc_log->info(mod_ctx, value->bool_value ? "[ZButton] Mod enabled by user" : "[ZButton] Mod disabled by user");
        }
    }
}

static void on_dpad_horse_call_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configDpadHorseCallEnabled = value->bool_value;
    }
}

static void on_dpad_horse_call_require_equipped_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configDpadHorseCallRequireEquipped = value->bool_value;
    }
}

static void on_dpad_horse_call_allow_anytime_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) {
    if (value) {
        g_configDpadHorseCallAllowAnytime = value->bool_value;
    }
}

static bool is_hp_bars_sub_disabled(ModContext*, void*) {
    return !g_configHpBarsEnabled;
}

static bool is_visible_equip_sub_disabled(ModContext*, void*) {
    return !g_configVisibleEquipmentEnabled;
}

static bool is_dpad_horse_call_sub_disabled(ModContext*, void*) {
    return !g_configDpadHorseCallEnabled;
}

static bool is_puppet_zelda_sub_disabled(ModContext*, void*) {
    return !g_configPuppetZeldaPatternEnabled;
}

static ModResult build_visible_equip_dialog(ModContext* ctx, UiElementHandle pane, void*, ModError*) {
    if (!svc_ui) return MOD_OK;

    if (s_varVisibleEquipShowBow != 0) {
        UiControlDesc ctrl = UI_CONTROL_DESC_INIT;
        ctrl.kind = UI_CONTROL_TOGGLE;
        ctrl.label = "Hero's Bow & Quiver";
        ctrl.binding = UI_BINDING_CONFIG_VAR;
        ctrl.config_var = s_varVisibleEquipShowBow;
        svc_ui->pane_add_control(ctx, pane, &ctrl, nullptr);
    }

    if (s_varVisibleEquipShowLantern != 0) {
        UiControlDesc ctrl = UI_CONTROL_DESC_INIT;
        ctrl.kind = UI_CONTROL_TOGGLE;
        ctrl.label = "Lantern (Belt)";
        ctrl.binding = UI_BINDING_CONFIG_VAR;
        ctrl.config_var = s_varVisibleEquipShowLantern;
        svc_ui->pane_add_control(ctx, pane, &ctrl, nullptr);
    }

    if (s_varVisibleEquipShowHorseCall != 0) {
        UiControlDesc ctrl = UI_CONTROL_DESC_INIT;
        ctrl.kind = UI_CONTROL_TOGGLE;
        ctrl.label = "Horse Call (Neck)";
        ctrl.binding = UI_BINDING_CONFIG_VAR;
        ctrl.config_var = s_varVisibleEquipShowHorseCall;
        svc_ui->pane_add_control(ctx, pane, &ctrl, nullptr);
    }

    return MOD_OK;
}

static void on_open_visible_equip_dialog(ModContext* ctx, void*) {
    if (!svc_ui) return;

    static UiDialogAction s_doneAction;
    s_doneAction.struct_size = sizeof(UiDialogAction);
    s_doneAction.label = "Done";
    s_doneAction.on_pressed = nullptr;
    s_doneAction.user_data = nullptr;
    s_doneAction.keep_open = false;

    UiDialogDesc desc = UI_DIALOG_DESC_INIT;
    desc.title = "Visible Equipment";
    desc.body_rml = "Select which items to display on Link's model:";
    desc.variant = UI_DIALOG_NORMAL;
    desc.actions = &s_doneAction;
    desc.action_count = 1;
    desc.build = build_visible_equip_dialog;

    UiDialogHandle hDialog = 0;
    svc_ui->dialog_push(ctx, &desc, &hDialog);
}

static ModResult build_mod_ui_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    if (!svc_ui) return MOD_OK;

    if (s_varCheckForUpdates != 0) {
        UiControlDesc ctrlUpdate = UI_CONTROL_DESC_INIT;
        ctrlUpdate.kind = UI_CONTROL_TOGGLE;
        ctrlUpdate.label = "Auto-Check for Updates";
        ctrlUpdate.binding = UI_BINDING_CONFIG_VAR;
        ctrlUpdate.config_var = s_varCheckForUpdates;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlUpdate, nullptr);
    }

    // HP Bars
    svc_ui->pane_add_section(mod_ctx, panel, "Enemy HP Bars");
    svc_ui->pane_add_text(mod_ctx, panel, "Shows health bars above enemies during combat.", nullptr);
    if (s_varHpBars != 0) {
        UiControlDesc ctrlHp = UI_CONTROL_DESC_INIT;
        ctrlHp.kind = UI_CONTROL_TOGGLE;
        ctrlHp.label = "Enabled";
        ctrlHp.binding = UI_BINDING_CONFIG_VAR;
        ctrlHp.config_var = s_varHpBars;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHp, nullptr);
    }

    if (s_varHpBarsShowNumbers != 0) {
        UiControlDesc ctrlHpNum = UI_CONTROL_DESC_INIT;
        ctrlHpNum.kind = UI_CONTROL_TOGGLE;
        ctrlHpNum.label = "Show exact HP numbers";
        ctrlHpNum.binding = UI_BINDING_CONFIG_VAR;
        ctrlHpNum.config_var = s_varHpBarsShowNumbers;
        ctrlHpNum.is_disabled = is_hp_bars_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHpNum, nullptr);
    }

    // Damage Numbers
    svc_ui->pane_add_section(mod_ctx, panel, "Damage Numbers");
    svc_ui->pane_add_text(mod_ctx, panel, "Pop-up numbers showing damage dealt to enemies.", nullptr);
    if (s_varDamageNumbers != 0) {
        UiControlDesc ctrlDmg = UI_CONTROL_DESC_INIT;
        ctrlDmg.kind = UI_CONTROL_TOGGLE;
        ctrlDmg.label = "Enabled";
        ctrlDmg.binding = UI_BINDING_CONFIG_VAR;
        ctrlDmg.config_var = s_varDamageNumbers;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlDmg, nullptr);
    }

    // Visible Equipment
    svc_ui->pane_add_section(mod_ctx, panel, "Visible Equipment");
    svc_ui->pane_add_text(mod_ctx, panel, "Shows Link's gear (bow, quiver, lantern, etc.) on his back.", nullptr);
    if (s_varVisibleEquip != 0) {
        UiControlDesc ctrlEquip = UI_CONTROL_DESC_INIT;
        ctrlEquip.kind = UI_CONTROL_TOGGLE;
        ctrlEquip.label = "Enabled";
        ctrlEquip.binding = UI_BINDING_CONFIG_VAR;
        ctrlEquip.config_var = s_varVisibleEquip;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlEquip, nullptr);
    }

    {
        UiControlDesc ctrlCustomGear = UI_CONTROL_DESC_INIT;
        ctrlCustomGear.kind = UI_CONTROL_BUTTON;
        ctrlCustomGear.label = "Choose Visible Items...";
        ctrlCustomGear.on_pressed = on_open_visible_equip_dialog;
        ctrlCustomGear.is_disabled = is_visible_equip_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlCustomGear, nullptr);
    }

    if (s_varVisibleEquipMode != 0) {
        UiControlDesc ctrlEquipMode = UI_CONTROL_DESC_INIT;
        ctrlEquipMode.kind = UI_CONTROL_TOGGLE;
        ctrlEquipMode.label = "Show all unlocked gear (even if unequipped)";
        ctrlEquipMode.binding = UI_BINDING_CONFIG_VAR;
        ctrlEquipMode.config_var = s_varVisibleEquipMode;
        ctrlEquipMode.is_disabled = is_visible_equip_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlEquipMode, nullptr);
    }

    if (s_varVisibleEquipMirrorBow != 0) {
        UiControlDesc ctrlEquipMirror = UI_CONTROL_DESC_INIT;
        ctrlEquipMirror.kind = UI_CONTROL_TOGGLE;
        ctrlEquipMirror.label = "Mirror bow angle";
        ctrlEquipMirror.binding = UI_BINDING_CONFIG_VAR;
        ctrlEquipMirror.config_var = s_varVisibleEquipMirrorBow;
        ctrlEquipMirror.is_disabled = is_visible_equip_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlEquipMirror, nullptr);
    }

    // Z Button
    svc_ui->pane_add_section(mod_ctx, panel, "Z Button Item Slot");
    svc_ui->pane_add_text(mod_ctx, panel, "Assign items to Z with a 3rd wheel slot. Midna calls move to D-Pad Left.\n(Note: Restart game after toggling this).", nullptr);
    if (s_varCustomZButton != 0) {
        UiControlDesc ctrlZ = UI_CONTROL_DESC_INIT;
        ctrlZ.kind = UI_CONTROL_TOGGLE;
        ctrlZ.label = "Enabled";
        ctrlZ.binding = UI_BINDING_CONFIG_VAR;
        ctrlZ.config_var = s_varCustomZButton;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlZ, nullptr);
    }

    // Horse Call
    svc_ui->pane_add_section(mod_ctx, panel, "Horse Call (D-Pad Down)");
    svc_ui->pane_add_text(mod_ctx, panel, "Call Epona quickly with D-Pad Down.", nullptr);
    if (s_varDpadHorseCall != 0) {
        UiControlDesc ctrlHorse = UI_CONTROL_DESC_INIT;
        ctrlHorse.kind = UI_CONTROL_TOGGLE;
        ctrlHorse.label = "Enabled";
        ctrlHorse.binding = UI_BINDING_CONFIG_VAR;
        ctrlHorse.config_var = s_varDpadHorseCall;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHorse, nullptr);
    }

    if (s_varDpadHorseCallRequireEquipped != 0) {
        UiControlDesc ctrlHorseEquipped = UI_CONTROL_DESC_INIT;
        ctrlHorseEquipped.kind = UI_CONTROL_TOGGLE;
        ctrlHorseEquipped.label = "Only when equipped on X, Y, or Z";
        ctrlHorseEquipped.binding = UI_BINDING_CONFIG_VAR;
        ctrlHorseEquipped.config_var = s_varDpadHorseCallRequireEquipped;
        ctrlHorseEquipped.is_disabled = is_dpad_horse_call_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHorseEquipped, nullptr);
    }

    if (s_varDpadHorseCallAllowAnytime != 0) {
        UiControlDesc ctrlHorseAnytime = UI_CONTROL_DESC_INIT;
        ctrlHorseAnytime.kind = UI_CONTROL_TOGGLE;
        ctrlHorseAnytime.label = "Usable without having Horse Call item";
        ctrlHorseAnytime.binding = UI_BINDING_CONFIG_VAR;
        ctrlHorseAnytime.config_var = s_varDpadHorseCallAllowAnytime;
        ctrlHorseAnytime.is_disabled = is_dpad_horse_call_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlHorseAnytime, nullptr);
    }

    // Sheathed Spin
    svc_ui->pane_add_section(mod_ctx, panel, "Sheathed Spin Attack");
    svc_ui->pane_add_text(mod_ctx, panel, "Perform spin attacks directly while your sword is sheathed.", nullptr);
    if (s_varSheathedSpin != 0) {
        UiControlDesc ctrlSpin = UI_CONTROL_DESC_INIT;
        ctrlSpin.kind = UI_CONTROL_TOGGLE;
        ctrlSpin.label = "Enabled";
        ctrlSpin.binding = UI_BINDING_CONFIG_VAR;
        ctrlSpin.config_var = s_varSheathedSpin;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlSpin, nullptr);
    }

    // Puppet Zelda
    svc_ui->pane_add_section(mod_ctx, panel, "Puppet Zelda Fixed Pattern");
    svc_ui->pane_add_text(mod_ctx, panel, "Removes RNG from Puppet Zelda's attacks with a consistent 7-step pattern.", nullptr);
    if (s_varPuppetZeldaPattern != 0) {
        UiControlDesc ctrlZelda = UI_CONTROL_DESC_INIT;
        ctrlZelda.kind = UI_CONTROL_TOGGLE;
        ctrlZelda.label = "Enabled";
        ctrlZelda.binding = UI_BINDING_CONFIG_VAR;
        ctrlZelda.config_var = s_varPuppetZeldaPattern;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlZelda, nullptr);
    }

    if (s_varPuppetZeldaAlwaysShortest != 0) {
        UiControlDesc ctrlShortest = UI_CONTROL_DESC_INIT;
        ctrlShortest.kind = UI_CONTROL_TOGGLE;
        ctrlShortest.label = "Always shortest attacks (Sword Dive only)";
        ctrlShortest.binding = UI_BINDING_CONFIG_VAR;
        ctrlShortest.config_var = s_varPuppetZeldaAlwaysShortest;
        ctrlShortest.is_disabled = is_puppet_zelda_sub_disabled;
        svc_ui->pane_add_control(mod_ctx, panel, &ctrlShortest, nullptr);
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

        ConfigVarDesc descEquipBow = CONFIG_VAR_DESC_INIT;
        descEquipBow.name = "visibleEquipmentShowBow";
        descEquipBow.type = CONFIG_VAR_BOOL;
        descEquipBow.default_bool = true;
        if (svc_config->register_var(mod_ctx, &descEquipBow, &s_varVisibleEquipShowBow) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varVisibleEquipShowBow, &g_configVisibleEquipShowBow);
            svc_config->subscribe(mod_ctx, s_varVisibleEquipShowBow, on_visible_equip_show_bow_changed, nullptr, nullptr);
        }

        ConfigVarDesc descEquipLantern = CONFIG_VAR_DESC_INIT;
        descEquipLantern.name = "visibleEquipmentShowLantern";
        descEquipLantern.type = CONFIG_VAR_BOOL;
        descEquipLantern.default_bool = true;
        if (svc_config->register_var(mod_ctx, &descEquipLantern, &s_varVisibleEquipShowLantern) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varVisibleEquipShowLantern, &g_configVisibleEquipShowLantern);
            svc_config->subscribe(mod_ctx, s_varVisibleEquipShowLantern, on_visible_equip_show_lantern_changed, nullptr, nullptr);
        }

        ConfigVarDesc descEquipHorseCall = CONFIG_VAR_DESC_INIT;
        descEquipHorseCall.name = "visibleEquipmentShowHorseCall";
        descEquipHorseCall.type = CONFIG_VAR_BOOL;
        descEquipHorseCall.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descEquipHorseCall, &s_varVisibleEquipShowHorseCall) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varVisibleEquipShowHorseCall, &g_configVisibleEquipShowHorseCall);
            svc_config->subscribe(mod_ctx, s_varVisibleEquipShowHorseCall, on_visible_equip_show_horse_call_changed, nullptr, nullptr);
        }

        ConfigVarDesc descZ = CONFIG_VAR_DESC_INIT;
        descZ.name = "customZButtonEnabled";
        descZ.type = CONFIG_VAR_BOOL;
        descZ.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descZ, &s_varCustomZButton) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varCustomZButton, &g_configCustomZButtonEnabled);
            g_configZButtonEnabled = g_configCustomZButtonEnabled;
            svc_config->subscribe(mod_ctx, s_varCustomZButton, on_custom_z_button_changed, nullptr, nullptr);
        }

        ConfigVarDesc descHorse = CONFIG_VAR_DESC_INIT;
        descHorse.name = "dpadHorseCallEnabled";
        descHorse.type = CONFIG_VAR_BOOL;
        descHorse.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descHorse, &s_varDpadHorseCall) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varDpadHorseCall, &g_configDpadHorseCallEnabled);
            svc_config->subscribe(mod_ctx, s_varDpadHorseCall, on_dpad_horse_call_changed, nullptr, nullptr);
        }

        ConfigVarDesc descHorseEquipped = CONFIG_VAR_DESC_INIT;
        descHorseEquipped.name = "dpadHorseCallRequireEquipped";
        descHorseEquipped.type = CONFIG_VAR_BOOL;
        descHorseEquipped.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descHorseEquipped, &s_varDpadHorseCallRequireEquipped) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varDpadHorseCallRequireEquipped, &g_configDpadHorseCallRequireEquipped);
            svc_config->subscribe(mod_ctx, s_varDpadHorseCallRequireEquipped, on_dpad_horse_call_require_equipped_changed, nullptr, nullptr);
        }

        ConfigVarDesc descHorseAnytime = CONFIG_VAR_DESC_INIT;
        descHorseAnytime.name = "dpadHorseCallAllowAnytime";
        descHorseAnytime.type = CONFIG_VAR_BOOL;
        descHorseAnytime.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descHorseAnytime, &s_varDpadHorseCallAllowAnytime) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varDpadHorseCallAllowAnytime, &g_configDpadHorseCallAllowAnytime);
            svc_config->subscribe(mod_ctx, s_varDpadHorseCallAllowAnytime, on_dpad_horse_call_allow_anytime_changed, nullptr, nullptr);
        }

        ConfigVarDesc descSpin = CONFIG_VAR_DESC_INIT;
        descSpin.name = "sheathedSpinEnabled";
        descSpin.type = CONFIG_VAR_BOOL;
        descSpin.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descSpin, &s_varSheathedSpin) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varSheathedSpin, &g_configSheathedSpinEnabled);
            svc_config->subscribe(mod_ctx, s_varSheathedSpin, on_sheathed_spin_changed, nullptr, nullptr);
        }

        ConfigVarDesc descZelda = CONFIG_VAR_DESC_INIT;
        descZelda.name = "puppetZeldaPatternEnabled";
        descZelda.type = CONFIG_VAR_BOOL;
        descZelda.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descZelda, &s_varPuppetZeldaPattern) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varPuppetZeldaPattern, &g_configPuppetZeldaPatternEnabled);
            svc_config->subscribe(mod_ctx, s_varPuppetZeldaPattern, on_puppet_zelda_pattern_changed, nullptr, nullptr);
        }

        ConfigVarDesc descZeldaShortest = CONFIG_VAR_DESC_INIT;
        descZeldaShortest.name = "puppetZeldaAlwaysShortest";
        descZeldaShortest.type = CONFIG_VAR_BOOL;
        descZeldaShortest.default_bool = false;
        if (svc_config->register_var(mod_ctx, &descZeldaShortest, &s_varPuppetZeldaAlwaysShortest) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varPuppetZeldaAlwaysShortest, &g_configPuppetZeldaAlwaysShortest);
            svc_config->subscribe(mod_ctx, s_varPuppetZeldaAlwaysShortest, on_puppet_zelda_always_shortest_changed, nullptr, nullptr);
        }

        ConfigVarDesc descUpdate = CONFIG_VAR_DESC_INIT;
        descUpdate.name = "checkForUpdates";
        descUpdate.type = CONFIG_VAR_BOOL;
        descUpdate.default_bool = true;
        if (svc_config->register_var(mod_ctx, &descUpdate, &s_varCheckForUpdates) == MOD_OK) {
            svc_config->get_bool(mod_ctx, s_varCheckForUpdates, &g_configCheckForUpdatesEnabled);
            svc_config->subscribe(mod_ctx, s_varCheckForUpdates, on_check_for_updates_changed, nullptr, nullptr);
        }
    }

    if (svc_ui) {
        UiModsPanelDesc panelDesc = UI_MODS_PANEL_DESC_INIT;
        panelDesc.build = build_mod_ui_panel;
        svc_ui->register_mods_panel(mod_ctx, &panelDesc);
    }

    if (svc_hook) {
        mods::hook::add_pre<DlstTitleDrawHook>(svc_hook, on_title_draw_pre);
    }

    ModResult res = init_hp_bars(svc_hook, error);
    if (res != MOD_OK) {
        if (svc_log) svc_log->error(mod_ctx, "Failed to initialize HP bars");
        return res;
    }

    init_damage_numbers(svc_hook, error);
    init_visible_equipment(svc_hook, error);
    init_z_button(svc_hook, svc_log, mod_ctx, error);
    init_horse_call(svc_hook, error);
    init_sheathed_spin(svc_hook);
    init_puppet_zelda_pattern(svc_hook, error);
    init_update_service(svc_log, mod_ctx, svc_ui, svc_config, s_varCheckForUpdates);

    if (svc_log) svc_log->info(mod_ctx, "Twilit Essentials initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    update_damage_numbers(svc_log, mod_ctx);
    update_visible_equipment(svc_log, mod_ctx);
    update_z_button(svc_log, mod_ctx);
    update_horse_call(svc_log, mod_ctx);
    update_sheathed_spin(svc_log, mod_ctx);
    update_puppet_zelda_pattern(svc_log, mod_ctx);
    update_update_service(svc_log, mod_ctx, svc_ui);
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    shutdown_hp_bars();
    shutdown_damage_numbers();
    shutdown_visible_equipment();
    shutdown_z_button();
    shutdown_horse_call();
    shutdown_update_service();
    return MOD_OK;
}
}

const ResourceService* get_resource_service() {
    return svc_resource;
}

const TextureService* get_texture_service() {
    return svc_texture;
}
