#pragma once

// ===========================================================================
//  z_mobile.cpp  --  Android / iOS only Z-button support
// ---------------------------------------------------------------------------
//  Compiled everywhere, but every routine collapses to a no-op unless
//  Z_MOBILE_BUILD is set (see z_mobile.hpp), so it only ever runs on mobile.
//
//  How the touch buttons work in Dusklight
//  ---------------------------------------
//  The on-screen buttons are an RML overlay (`dusk::ui::TouchControls`), not
//  the J2D HUD. Each frame `TouchControls::sync_control_displays()` asks for a
//  per-button state and pushes it into the DOM:
//
//    X / Y : icon   <- item_icon_source_for_button()  ->  "item://item/<hex>"
//            count  <- item_count_label_for_button()  ->  <count class="item-count">
//            oil    <- item_oil_fill_for_button()     ->  <oil-meter><oil-fill>
//    B     : icon only
//    Z     : icon   <- midna_icon_source()            ->  "meter://midna"
//            (no count / oil elements exist in the Z button's RML at all)
//
//  The Z button is hard-wired to Midna, so this mod's Z item never shows up.
//  Worse, `sync_equip_target(2, ...)` -- the screen rect that
//  `dMenu_Ring_c::drawSelectItem` animates the item towards -- is only filled
//  in when the Z button reports `showIcon`, so the item-wheel animation flew
//  to a stale HUD position instead of the real button.
//
//  What this file does
//  -------------------
//  1. Post-hooks `dusk::ui::midna_icon_source()` / `midna_icon_revision()` and
//     returns the Z item's own "item://item/<hex>" source. That is the exact
//     same path X/Y use, so the icon is rendered from the item texture at full
//     opacity (the old approach re-rendered the `midona_n` HUD pane, which
//     baked that pane's fade alpha into the icon -- hence the see-through
//     icon that only appeared while the item wheel forced the pane visible).
//     Dusklight then also syncs the Z equip target on its own, which fixes the
//     wheel animation.
//
//  2. Adds the missing count / oil-meter to the Z button. The Z button's RML is
//     `<button id="button-z"><img class="midna-icon"/><span>Z</span></button>`,
//     so child 1 (the label span) is repurposed as a full-size container and
//     its inner RML is rewritten with the "Z" label plus a `<count>` or
//     `<oil-meter>` using dusklight's own CSS classes.
//     To know *which* DOM element is the Z button we watch for the
//     `SetClass("has-icon", ...)` call that sync_control_displays only ever
//     makes on that button.
//
//  3. Blocks the involuntary iron-boots un-equip (see the boots section below).
//
//  `src/dusk` symbols are excluded from the Android/iOS export list, so they
//  can't be linked or dlsym'd -- but the SDK hook service resolves them from
//  the symbol manifest embedded in the game image, which is what
//  DEFINE_HOOK_SYMBOL and `svc_hook->resolve()` use here. Approach follows the
//  user's own dawnlight mod (src/item_slot_hooks.cpp).
//
//  Fallback for builds with no embedded manifest
//  --------------------------------------------
//  Not every Dusklight build embeds one ("no symbol manifest for this build" in
//  the log). Then none of the above resolves and we fall back to the only
//  bridge that plain member-function hooks can reach: `midona_n`.
//  `setButtonIconMidonaAlpha()` ends with
//  `update_midna_icon_texture(mpButtonMidona->getPanePtr())`, which renders that
//  pane into the touch Z button's texture -- so parenting the Z item pane into
//  `midona_n` puts the item on the touch button. Two things must be forced for
//  that to actually produce a visible, opaque icon:
//    * `dMeter2Info_onUseButton(METER2_USEBUTTON_Z)`, otherwise the function
//      takes its zero / "dim" alpha branch and CPaneMgr::setAlpha cascades that
//      alpha down into our item pane -- which is what made the icon see-through
//      and invisible outside the item wheel (the wheel sets that flag itself).
//    * `mButtonZAlpha = 1`, to skip the fade-in ramp.
//  This has no on-screen side effect: with touch controls, dMeter2Draw_c::draw()
//  hides the whole J2D button parent every frame, so `midona_n` is only ever
//  seen through the render-to-texture.
//  The fallback cannot supply the count / oil meter (that needs the RML DOM,
//  hence the manifest).
// ===========================================================================

#include "z_mobile.hpp"

#if Z_MOBILE_BUILD

#include <algorithm>
#include <cstdio>
#include <string>

// Itanium-mangled names, valid for the Android/iOS (libc++) builds only.
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

// Rml::Element / TouchControls are opaque here: the mod SDK ships no RmlUi or
// dusk headers, and we only ever pass the pointers straight back.
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

// Set when the icon hooks could not be installed (no symbol manifest in this
// game build); the `midona_n` render-to-texture fallback takes over.
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

// ---------------------------------------------------------------- Z item ----

// Read-only: safe to call from the RML display-sync path.
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

// dusklight's icon_provider serves "item://item/<hex>"; the query string is only
// a cache key (it is stripped before the lookup).
std::string z_item_icon_source() {
    const u8 itemNo = current_z_item();
    if (itemNo == dItemNo_NONE_e) {
        return {};
    }
    // No dedicated light-arrow HUD icon; the bow icon is what the game uses.
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
    // Fold in the ammo count so the icon is re-fetched when it changes.
    int count = 0;
    int maxCount = 0;
    z_item_ammo(itemNo, count, maxCount);
    // 'Z' tag in the high byte so it can never collide with a real revision.
    *static_cast<uint64_t*>(retval) = 0x5A000000ull | (static_cast<uint64_t>(itemNo) << 12) |
                                      static_cast<uint64_t>(count & 0xFFF);
}

// ------------------------------------------------- count / oil DOM injection --

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

    // Children of `<button id="button-z">`: 0 = <img class="midna-icon">, 1 = <span>Z</span>.
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

    // The "Z" label keeps the position .button-z.has-icon span would have given it.
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

// sync_control_displays() sets "has-icon" on the Z button and nothing else,
// which is how we identify that element without reaching into TouchControls.
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

// --------------------------------------------- midona_n capture fallback ----

// Restore the Midna prompt's own children once we stop hosting.
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

}  // namespace

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

    // Park `midona_n` on the Z button so the item sits where the capture expects
    // it (this is the layout that empirically produced an icon).
    J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
    if (zbtn != nullptr && midona->getParentPane() != zbtn) {
        J2DPane* oldParent = midona->getParentPane();
        if (oldParent != nullptr) {
            oldParent->mPaneTree.removeChild(&midona->mPaneTree);
        }
        zbtn->appendChild(midona);
        midona->translate(0.0f, 0.0f);
    }

    // setButtonIconMidonaAlpha() computes
    //   alpha = mMidnaIconAlpha*mParentAlpha*mMainHUDButtonsAlpha
    //           * 255 * mButtonZAlpha * mpButtonParent->getAlphaRate()
    // and CPaneMgr::setAlpha cascades that into our item pane. Any factor at 0
    // gives a fully transparent (i.e. invisible) icon, so pin the ones we own.
    // No on-screen effect: with touch controls dMeter2Draw_c::draw() hides the
    // whole J2D button parent every frame.
    g_meter2_info.onUseButton(0x800);
    draw->mButtonZAlpha = 1.0f;
    if (draw->mpButtonParent != nullptr) {
        draw->mpButtonParent->setAlphaRate(1.0f);
    }

    midona->show();
    midona->setAlpha(255);

    // Only the Z item may contribute to the captured icon: hide Midna's own art.
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

    // Z item icon on the touch button (+ dusklight then syncs the equip target,
    // which is what the item-wheel animation follows).
    const bool iconOk =
        mods::hook::add_post<ZMidnaIconSourceHook>(hook_svc, after_midna_icon_source) == MOD_OK &&
        mods::hook::add_post<ZMidnaIconRevisionHook>(hook_svc, after_midna_icon_revision) == MOD_OK;

    // Without the icon hooks (game build ships no embedded symbol manifest) fall
    // back to feeding the item through the `midona_n` render-to-texture instead.
    s_useCaptureFallback = !iconOk;
    log_z_info("[ZButton/mobile] touch Z icon: %s", iconOk ? "direct (item:// icon source)"
                                                           : "midona_n capture fallback "
                                                             "(no symbol manifest in this build)");

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
    log_z_info("[ZButton/mobile] touch Z count/oil hooks: %s", meterOk ? "ok" : "FAILED");

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

    log_z_info(
        "[ZButton/mobile] capture: host=%d item=0x%02X | midona vis=%d a=%d | item vis=%d a=%d "
        "parented=%d %.0fx%.0f | zAlpha=%.2f parentRate=%.2f hio=%.2f/%.2f/%.2f",
        s_hostActive ? 1 : 0, current_z_item(),
        midona ? midona->isVisible() : -1, midona ? midona->getAlpha() : -1,
        itemPane ? itemPane->isVisible() : -1, itemPane ? itemPane->getAlpha() : -1,
        (itemPane != nullptr && midona != nullptr && itemPane->getParentPane() == midona) ? 1 : 0,
        itemW, itemH,
        draw->mButtonZAlpha,
        draw->mpButtonParent ? draw->mpButtonParent->getAlphaRate() : -1.0f,
        g_drawHIO.mMidnaIconAlpha, g_drawHIO.mParentAlpha, g_drawHIO.mMainHUDButtonsAlpha);
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

// ===========================================================================
//  Iron / heavy boots on the Z slot  (ported from dawnlight item_slot_hooks.cpp)
// ---------------------------------------------------------------------------
//  With touch controls a Z press doesn't register as "held" the way a pad press
//  does, so the engine calls setHeavyBoots(0) right after the boots go on and
//  they pop straight back off. This lock lets a deliberate toggle through but
//  blocks the involuntary un-equip.
// ===========================================================================

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

}  // namespace

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

    // Deliberate re-equip right after a manual toggle-off: allow it.
    if (enable != 0 && s_hbGuardLink == link && s_hbManualToggleOff) {
        hb_clear_lock();
        return HOOK_CONTINUE;
    }
    // Same press still being processed: don't let it toggle again.
    if (enable != 0 && z_mobile_hb_locked(link)) {
        if (retval) *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    // Contexts where the boots genuinely must come off (riding, wolf, ...).
    if (enable == 0 && hb_forced_off_context(link)) {
        hb_clear_lock();
        return HOOK_CONTINUE;
    }
    // Otherwise: block the involuntary un-equip.
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

#else  // ------------------------------------------------------------------- desktop

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
