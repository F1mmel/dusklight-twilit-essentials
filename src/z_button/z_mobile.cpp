#pragma once

#include "z_mobile.hpp"

#if Z_MOBILE_BUILD

#include <algorithm>
#include <cstdio>
#include <string>

// Itanium-mangled names, valid for the Android/iOS (libc++) builds only. hopefully not needed in the future anymore
#define Z_SYM_MIDNA_SOURCE "_ZN4dusk2ui17midna_icon_sourceEv"
#define Z_SYM_MIDNA_REVISION "_ZN4dusk2ui19midna_icon_revisionEv"
#define Z_SYM_SYNC_DISPLAYS "_ZN4dusk2ui13TouchControls21sync_control_displaysEv"
#define Z_SYM_RML_SET_CLASS                                                                        \
    "_ZN3Rml7Element8SetClassERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb"
#define Z_SYM_RML_SET_INNER_RML                                                                    \
    "_ZN3Rml7Element11SetInnerRMLERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"
#define Z_SYM_RML_GET_CHILD "_ZNK3Rml7Element8GetChildEi"
#define Z_SYM_RML_SET_PROPERTY                                                                     \
    "_ZN3Rml7Element11SetPropertyERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEES9_"
#define Z_SYM_GET_EQUIP_TARGET "_ZN4dusk2ui16get_equip_targetEiRNS0_11EquipTargetE"

DEFINE_HOOK_SYMBOL(Z_SYM_MIDNA_SOURCE, std::string(), ZMidnaIconSourceHook);
DEFINE_HOOK_SYMBOL(Z_SYM_MIDNA_REVISION, uint64_t(), ZMidnaIconRevisionHook);
DEFINE_HOOK_SYMBOL(Z_SYM_SYNC_DISPLAYS, void(void*), ZTouchSyncDisplaysHook);
DEFINE_HOOK_SYMBOL(Z_SYM_RML_SET_CLASS, void(void*, const std::string*, bool), ZRmlSetClassHook);

namespace {

using RmlGetChildFn = void* (*)(const void*, int);
using RmlSetPropertyFn = bool (*)(void*, const std::string*, const std::string*);
using RmlSetInnerRMLFn = void (*)(void*, const std::string*);

// dusklight's `dusk::ui::EquipTarget` (src/dusk/ui/controls.hpp). Layout must match.
struct EquipTargetABI {
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool valid = false;
};
using GetEquipTargetFn = bool (*)(int, EquipTargetABI&);

const HookService* s_hookSvc = nullptr;
RmlGetChildFn s_rmlGetChild = nullptr;
RmlSetPropertyFn s_rmlSetProperty = nullptr;
RmlSetInnerRMLFn s_rmlSetInnerRML = nullptr;
GetEquipTargetFn s_getEquipTarget = nullptr;

bool s_inDisplaySync = false;
void* s_syncZButton = nullptr;

void* s_meterButton = nullptr;
void* s_meterContainer = nullptr;
std::string s_meterRml;

bool s_useCaptureFallback = false;
J2DPane* s_hostedMidona = nullptr;
bool s_hostActive = false;

template <typename Fn>
bool resolve_symbol(const char* name, Fn& out) {
    if (s_hookSvc == nullptr || s_hookSvc->resolve == nullptr) {
        return false;
    }
    void* addr = nullptr;
    if (s_hookSvc->resolve(mod_ctx, name, &addr, nullptr) != MOD_OK || addr == nullptr) {
        log_z_info("[ZButton/mobile] symbol NOT resolved: %s", name);
        return false;
    }
    out = reinterpret_cast<Fn>(addr);
    return true;
}

u8 current_z_item() {
    if (isWolfPlayer()) {
        return dItemNo_NONE_e;
    }
    u8 zItem = resolved_select_item(2);
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        if (g_zInventorySlot != 0xFF && g_zInventorySlot < 24) {
            zItem = dComIfGs_getItem(g_zInventorySlot, false);
        }
    }
    if (zItem == 0xFF || zItem == 0x00) {
        return dItemNo_NONE_e;
    }
    return zItem;
}

// dusklight's icon_provider serves "item://item/<hex>"
std::string z_item_icon_source() {
    const u8 itemNo = current_z_item();
    if (itemNo == dItemNo_NONE_e) {
        return {};
    }
    const u8 textureItem = (itemNo == dItemNo_LIGHT_ARROW_e) ? dItemNo_BOW_e : itemNo;
    char buf[48] = {};
    std::snprintf(buf, sizeof(buf), "item://item/%02x?ztwe=%02x", textureItem, itemNo);
    return buf;
}

void after_midna_icon_source(ModContext*, void*, void* retval, void*) {
    if (!g_configCustomZButtonEnabled || retval == nullptr) {
        return;
    }
    std::string source = z_item_icon_source();
    if (!source.empty()) {
        *static_cast<std::string*>(retval) = std::move(source);
    }
}

void after_midna_icon_revision(ModContext*, void*, void* retval, void*) {
    if (!g_configCustomZButtonEnabled || retval == nullptr) {
        return;
    }
    const u8 itemNo = current_z_item();
    if (itemNo == dItemNo_NONE_e) {
        return;
    }
    int count = 0;
    int maxCount = 0;
    z_item_ammo(itemNo, count, maxCount);
    *static_cast<uint64_t*>(retval) = 0x5A000000ull | (static_cast<uint64_t>(itemNo) << 12) |
                                      static_cast<uint64_t>(count & 0xFFF);
}

void set_property(void* element, const char* name, const char* value) {
    if (element == nullptr || s_rmlSetProperty == nullptr) {
        return;
    }
    const std::string propertyName = name;
    const std::string propertyValue = value;
    s_rmlSetProperty(element, &propertyName, &propertyValue);
}

// Turn the Z button's label span into a full-size overlay so the count / oil
// meter can be positioned against the button exactly like they are on X/Y.
void configure_meter_container(void* container) {
    set_property(container, "position", "absolute");
    set_property(container, "left", "0dp");
    set_property(container, "top", "0dp");
    set_property(container, "right", "auto");
    set_property(container, "bottom", "auto");
    set_property(container, "width", "100%");
    set_property(container, "height", "100%");
    set_property(container, "font-size", "0dp");
    set_property(container, "overflow", "visible");
    set_property(container, "pointer-events", "none");
}

void sync_z_button_meter(void* button) {
    if (button == nullptr || s_rmlGetChild == nullptr || s_rmlSetProperty == nullptr ||
        s_rmlSetInnerRML == nullptr)
    {
        return;
    }

    void* container = s_rmlGetChild(button, 1);
    if (container == nullptr) {
        return;
    }
    if (button != s_meterButton || container != s_meterContainer) {
        s_meterButton = button;
        s_meterContainer = container;
        s_meterRml.clear();
        configure_meter_container(container);
    }

    std::string rml =
        "<span style=\"position:absolute;right:9dp;bottom:7dp;font-size:13dp;line-height:1;\">Z"
        "</span>";

    const u8 itemNo = g_configCustomZButtonEnabled ? current_z_item() : dItemNo_NONE_e;
    if (itemNo != dItemNo_NONE_e) {
        int count = 0;
        int maxCount = 0;
        if (z_item_ammo(itemNo, count, maxCount) && count >= 0) {
            rml += "<count class=\"item-count visible\">" + std::to_string(count) + "</count>";
        } else if (z_item_is_lantern(itemNo) && dComIfGs_getMaxOil() > 0) {
            const f32 fill = std::clamp(static_cast<f32>(dComIfGs_getOil()) /
                                            static_cast<f32>(dComIfGs_getMaxOil()),
                0.0f, 1.0f);
            char percent[32] = {};
            std::snprintf(percent, sizeof(percent), "%.1f%%", fill * 100.0f);
            rml += "<oil-meter class=\"oil-meter visible\"><oil-fill style=\"width:" +
                   std::string(percent) + ";\" /></oil-meter>";
        }
    }

    if (rml == s_meterRml) {
        return;
    }
    s_rmlSetInnerRML(container, &rml);
    s_meterRml = rml;
}

HookAction before_touch_sync_displays(ModContext*, void*, void*, void*) {
    s_inDisplaySync = true;
    s_syncZButton = nullptr;
    return HOOK_CONTINUE;
}

void after_touch_sync_displays(ModContext*, void*, void*, void*) {
    void* button = s_syncZButton;
    s_inDisplaySync = false;
    s_syncZButton = nullptr;
    sync_z_button_meter(button);
}

HookAction before_rml_set_class(ModContext*, void* args, void*, void*) {
    if (!s_inDisplaySync || args == nullptr) {
        return HOOK_CONTINUE;
    }
    void* element = mods::arg<void*>(args, 0);
    const std::string* className = mods::arg<const std::string*>(args, 1);
    if (element != nullptr && className != nullptr && *className == "has-icon") {
        s_syncZButton = element;
    }
    return HOOK_CONTINUE;
}

void release_hosted_midona() {
    if (!s_hostActive) {
        return;
    }
    if (s_hostedMidona != nullptr) {
        for (J2DPane* child = s_hostedMidona->getFirstChildPane(); child != nullptr;
             child = child->getNextChildPane()) {
            child->show();
        }
    }
    s_hostedMidona = nullptr;
    s_hostActive = false;
}

}

bool z_mobile_active() {
    return true;
}

bool z_mobile_wants_midona_host() {
    return s_useCaptureFallback && g_configCustomZButtonEnabled && !isWolfPlayer() &&
           !is_pause_menu_open() && current_z_item() != dItemNo_NONE_e;
}

J2DPane* z_mobile_sync_touch_z(dMeter2Draw_c* draw) {
    if (!s_useCaptureFallback || draw == nullptr || isTitleOrMainMenu()) {
        return nullptr;
    }
    J2DScreen* screen = draw->getMainScreenPtr();
    if (screen == nullptr) {
        release_hosted_midona();
        return nullptr;
    }

    J2DPane* midona = screen->search(MULTI_CHAR('midona_n'));
    const bool wantHost = g_configCustomZButtonEnabled && !isWolfPlayer() &&
                          !is_pause_menu_open(draw) && current_z_item() != dItemNo_NONE_e;
    if (!wantHost || midona == nullptr) {
        release_hosted_midona();
        return nullptr;
    }

    J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
    if (zbtn != nullptr && midona->getParentPane() != zbtn) {
        J2DPane* oldParent = midona->getParentPane();
        if (oldParent != nullptr) {
            oldParent->mPaneTree.removeChild(&midona->mPaneTree);
        }
        zbtn->appendChild(midona);
        midona->translate(0.0f, 0.0f);
    }

    g_meter2_info.onUseButton(0x800);
    draw->mButtonZAlpha = 1.0f;
    if (draw->mpButtonParent != nullptr) {
        draw->mpButtonParent->setAlphaRate(1.0f);
    }

    midona->show();
    midona->setAlpha(255);

    J2DPane* itemPane = nullptr;
    if (CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2)) {
        itemPane = itemR->getPanePtr();
    }
    for (J2DPane* child = midona->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane()) {
        if (child == itemPane) {
            child->show();
            continue;
        }
        child->hide();
        child->setAlpha(0);
    }

    s_hostedMidona = midona;
    s_hostActive = true;
    return midona;
}

void z_mobile_init(const HookService* hook_svc) {
    s_hookSvc = hook_svc;
    if (hook_svc == nullptr) {
        return;
    }

    const bool iconOk =
        mods::hook::add_post<ZMidnaIconSourceHook>(hook_svc, after_midna_icon_source) == MOD_OK &&
        mods::hook::add_post<ZMidnaIconRevisionHook>(hook_svc, after_midna_icon_revision) == MOD_OK;

    s_useCaptureFallback = !iconOk;

    // Ammo count / lantern oil bar on the touch button.
    const bool meterOk =
        resolve_symbol(Z_SYM_RML_GET_CHILD, s_rmlGetChild) &&
        resolve_symbol(Z_SYM_RML_SET_PROPERTY, s_rmlSetProperty) &&
        resolve_symbol(Z_SYM_RML_SET_INNER_RML, s_rmlSetInnerRML) &&
        mods::hook::add_pre<ZTouchSyncDisplaysHook>(hook_svc, before_touch_sync_displays) ==
            MOD_OK &&
        mods::hook::add_post<ZTouchSyncDisplaysHook>(hook_svc, after_touch_sync_displays) ==
            MOD_OK &&
        mods::hook::add_pre<ZRmlSetClassHook>(hook_svc, before_rml_set_class) == MOD_OK;

    resolve_symbol(Z_SYM_GET_EQUIP_TARGET, s_getEquipTarget);
}

void z_mobile_report_after_midna_alpha(dMeter2Draw_c* draw) {
    if (!s_useCaptureFallback || draw == nullptr) {
        return;
    }
    static int s_tick = 0;
    if (++s_tick < 120) {
        return;
    }
    s_tick = 0;

    J2DScreen* screen = draw->getMainScreenPtr();
    J2DPane* midona = (screen != nullptr) ? screen->search(MULTI_CHAR('midona_n')) : nullptr;
    J2DPane* itemPane = nullptr;
    if (CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2)) {
        itemPane = itemR->getPanePtr();
    }

    f32 itemW = 0.0f;
    f32 itemH = 0.0f;
    if (itemPane != nullptr) {
        const JGeometry::TBox2<f32>& b = itemPane->getGlbBounds();
        itemW = b.getWidth();
        itemH = b.getHeight();
    }
}

bool z_mobile_touch_z_rect(f32& x, f32& y, f32& w, f32& h) {
    if (s_getEquipTarget == nullptr) {
        return false;
    }
    EquipTargetABI target;
    if (!s_getEquipTarget(2, target) || !target.valid) {
        return false;
    }
    if (!(target.width > 1.0f) || !(target.height > 1.0f)) {
        return false;
    }
    x = target.left;
    y = target.top;
    w = target.width;
    h = target.height;
    return true;
}

namespace {

daAlink_c* s_hbGuardLink = nullptr;
bool s_hbManualToggleOff = false;
bool s_hbWaitRelease = false;
u8 s_hbGuardFrames = 0;

constexpr u8 kBtnZ = 0x04;  // daAlink_c::BTN_Z

bool hb_z_selected(daAlink_c* link) {
    return link != nullptr &&
           link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(2));
}

bool hb_z_held(daAlink_c* link) {
    return link != nullptr && (link->mItemButton & kBtnZ) != 0;
}

bool hb_forced_off_context(daAlink_c* link) {
    if (link == nullptr) {
        return true;
    }
    if (link->checkWolf() || link->checkEventRun() || link->checkDeadHP() ||
        link->checkCanoeRide() || link->checkHorseRide() || link->checkBoardRide() ||
        link->checkSpinnerRide())
    {
        return true;
    }
    switch (link->mProcID) {
    case daAlink_c::PROC_DIVE_JUMP:
    case daAlink_c::PROC_SMALL_JUMP:
    case daAlink_c::PROC_CANOE_RIDE:
    case daAlink_c::PROC_CANOE_JUMP_RIDE:
    case daAlink_c::PROC_CANOE_GETOFF:
    case daAlink_c::PROC_HORSE_RIDE:
    case daAlink_c::PROC_HORSE_GETOFF:
    case daAlink_c::PROC_BOARD_RIDE:
    case daAlink_c::PROC_SPINNER_READY:
        return true;
    default:
        return false;
    }
}

void hb_clear_lock() {
    s_hbGuardLink = nullptr;
    s_hbManualToggleOff = false;
    s_hbWaitRelease = false;
    s_hbGuardFrames = 0;
}

}

bool z_mobile_hb_locked(daAlink_c* link) {
    return link != nullptr && s_hbGuardLink == link && s_hbWaitRelease;
}

void z_mobile_hb_lock(daAlink_c* link, bool manualToggleOff) {
    if (link == nullptr) {
        return;
    }
    s_hbGuardLink = link;
    s_hbManualToggleOff = manualToggleOff;
    s_hbWaitRelease = true;
    s_hbGuardFrames = manualToggleOff ? 24 : 0;
}

void z_mobile_hb_tick(daAlink_c* link) {
    if (s_hbGuardLink == nullptr || link == nullptr) {
        s_hbWaitRelease = false;
        s_hbGuardFrames = 0;
        return;
    }
    if (s_hbGuardLink != link || !s_hbWaitRelease) {
        return;
    }
    if (s_hbManualToggleOff) {
        if (s_hbGuardFrames != 0) {
            --s_hbGuardFrames;
        } else {
            hb_clear_lock();
        }
        return;
    }
    if (!hb_z_held(link)) {
        hb_clear_lock();
    }
}

HookAction z_mobile_guard_heavy_boots(void* args, void* retval) {
    if (!g_configCustomZButtonEnabled || args == nullptr) {
        return HOOK_CONTINUE;
    }
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);
    const int enable = mods::arg<int>(args, 1);

    if (link == nullptr || !link->checkEquipHeavyBoots() || link->checkNotHeavyBootsStage() ||
        !hb_z_selected(link))
    {
        return HOOK_CONTINUE;
    }

    if (enable != 0 && s_hbGuardLink == link && s_hbManualToggleOff) {
        hb_clear_lock();
        return HOOK_CONTINUE;
    }
    if (enable != 0 && z_mobile_hb_locked(link)) {
        if (retval) *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    if (enable == 0 && hb_forced_off_context(link)) {
        hb_clear_lock();
        return HOOK_CONTINUE;
    }
    if (retval) *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void z_mobile_shutdown() {
    hb_clear_lock();
    release_hosted_midona();
    s_meterButton = nullptr;
    s_meterContainer = nullptr;
    s_meterRml.clear();
    s_inDisplaySync = false;
    s_syncZButton = nullptr;
}

#else

bool z_mobile_active() {
    return false;
}
void z_mobile_init(const HookService*) {}
bool z_mobile_wants_midona_host() {
    return false;
}
J2DPane* z_mobile_sync_touch_z(dMeter2Draw_c*) {
    return nullptr;
}
void z_mobile_report_after_midna_alpha(dMeter2Draw_c*) {}
bool z_mobile_touch_z_rect(f32&, f32&, f32&, f32&) {
    return false;
}
bool z_mobile_hb_locked(daAlink_c*) {
    return false;
}
void z_mobile_hb_lock(daAlink_c*, bool) {}
void z_mobile_hb_tick(daAlink_c*) {}
HookAction z_mobile_guard_heavy_boots(void*, void*) {
    return HOOK_CONTINUE;
}
void z_mobile_shutdown() {}

#endif
