#include "collection_nav.hpp"

#include "../z_button/z_button.hpp"
#include "f_pc/f_pc_profile_lst.h"

// another helper function, works a little bit differently this time though
template <typename T>
const T &shiftHIO(const T &member) {
    // Return the amount of bytes to shift by only if on lazy tweaks, otherwise return 0 (shift by nothing)
    int shiftByThis = isNativeZButtonEngine() ? 4 * static_cast<int>(sizeof(f32)) : 0;
    return *reinterpret_cast<const T *>(reinterpret_cast<const u8 *>(&member) + shiftByThis);   // return the value to shift by after casting
}

HookAction on_get_item_tag_pre(ModContext*, void* args, void* ret, void*) {
    if (!is_collection_menu_enabled() || !args || !ret) return HOOK_CONTINUE;
    int i_tag1 = mods::arg<int>(args, 1);
    int i_tag2 = mods::arg<int>(args, 2);
    bool param_3 = mods::arg<bool>(args, 3);

    if (i_tag2 == 5 && !param_3) {
        *(u64*)ret = 0;
        return HOOK_SKIP_ORIGINAL;
    }

    if (i_tag2 == 0) {
        if (i_tag1 == 3) { *(u64*)ret = MULTI_CHAR('ken_n0'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 4) { *(u64*)ret = MULTI_CHAR('ken_mid'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 5) { *(u64*)ret = MULTI_CHAR('ken_n1'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 6) { *(u64*)ret = MULTI_CHAR('heart_n'); return HOOK_SKIP_ORIGINAL; }
    } else if (i_tag2 == 1) {
        if (i_tag1 == 3) { *(u64*)ret = MULTI_CHAR('tate_n0'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 4) { *(u64*)ret = MULTI_CHAR('tate_mid'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 5) { *(u64*)ret = MULTI_CHAR('tate_n1'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 6) { *(u64*)ret = 0; return HOOK_SKIP_ORIGINAL; }
    } else if (i_tag2 == 2) {
        if (i_tag1 == 3) { *(u64*)ret = MULTI_CHAR('fuku_ord'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 4) { *(u64*)ret = MULTI_CHAR('fuku_n0'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 5) { *(u64*)ret = MULTI_CHAR('fuku_n1'); return HOOK_SKIP_ORIGINAL; }
        if (i_tag1 == 6) { *(u64*)ret = MULTI_CHAR('fuku_n2'); return HOOK_SKIP_ORIGINAL; }
    }
    return HOOK_CONTINUE;
}

J2DPane* get_target_pane(dMenu_Collect2D_c* collect2D, u8 x, u8 y) {
    if (!collect2D || !collect2D->mpScreen) return nullptr;
    if (y == 0) {
        if (x == 3) return collect2D->mpScreen->search(MULTI_CHAR('ken_n0'));
        if (x == 4) return s_paneKenMid;
        if (x == 5) return collect2D->mpScreen->search(MULTI_CHAR('ken_n1'));
        if (x == 6) return collect2D->mpScreen->search(MULTI_CHAR('heart_n'));
    } else if (y == 1) {
        if (x == 3) return collect2D->mpScreen->search(MULTI_CHAR('tate_n0'));
        if (x == 4) return s_paneTateMid;
        if (x == 5) return collect2D->mpScreen->search(MULTI_CHAR('tate_n1'));
    } else if (y == 2) {
        if (x == 3) return s_paneFukuStart;
        if (x == 4) return collect2D->mpScreen->search(MULTI_CHAR('fuku_n0'));
        if (x == 5) return collect2D->mpScreen->search(MULTI_CHAR('fuku_n1'));
        if (x == 6) return collect2D->mpScreen->search(MULTI_CHAR('fuku_n2'));
    }
    if (x < 7 && y < 6 && collect2D->mpSelPm[x][y]) {
        return collect2D->mpSelPm[x][y]->getPanePtr();
    }
    return nullptr;
}

HookAction on_cursor_pos_set_pre(ModContext*, void* args, void*, void*) {
    if (!is_collection_menu_enabled() || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D || !collect2D->mpScreen || !collect2D->mpDrawCursor) return HOOK_CONTINUE;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    const dMeter_drawCollectHIO_c &collectHIO = shiftHIO(g_drawHIO.mCollectScreen);

    // Scale all panes
    // renamed usage of g_drawHIO to collectHIO pointer, will work regardless of branch
    for (u8 y = 0; y < 6; y++) {
        for (u8 x = 0; x < 7; x++) {
            J2DPane* pane = get_target_pane(collect2D, x, y);
            if (pane) {
                if ((x != 0 || y != 0) && (x != 6 || y != 0)) {
                    if (y == 5) {
                        if (x == curX && y == curY) {
                            pane->scale(collectHIO.mSelectSaveOptionScale,
                                       collectHIO.mSelectSaveOptionScale);
                        } else {
                            pane->scale(collectHIO.mUnselectSaveOptionScale,
                                       collectHIO.mUnselectSaveOptionScale);
                        }
                    } else if (x == curX && y == curY) {
                        pane->scale(collectHIO.mSelectItemScale,
                                   collectHIO.mSelectItemScale);
                    } else {
                        pane->scale(collectHIO.mUnselectItemScale,
                                   collectHIO.mUnselectItemScale);
                    }
                }
            }
        }
    }

    collect2D->mpDrawCursor->setAlphaRate(1.0f);

    J2DPane* curPane = get_target_pane(collect2D, curX, curY);
    if (curPane) {
        Vec pos;
        if (curX == 6 && curY == 0) {
            J2DPane* heart_n = collect2D->mpScreen->search(MULTI_CHAR('heart_n'));
            pos.x = heart_n ? heart_n->getTranslateX() : curPane->getTranslateX();
            pos.y = heart_n ? heart_n->getTranslateY() : curPane->getTranslateY();
            pos.z = 0.0f;
        } else {
            pos.x = curPane->getTranslateX();
            pos.y = curPane->getTranslateY();
            pos.z = 0.0f;
        }
        collect2D->mpDrawCursor->setPos(pos.x, pos.y, curPane, false);
    }

    if (curY == 5) {
        collect2D->mpDrawCursor->setParam(1.1f, 0.85f, 0.05f, 0.5f, 0.5f);
    } else if (curX == 6 && curY == 0) {
        collect2D->mpDrawCursor->setParam(0.6f, 0.85f, 0.03f, 0.6f, 0.6f);
    } else {
        collect2D->mpDrawCursor->setParam(1.0f, 1.0f, 0.1f, 0.7f, 0.7f);
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction on_cursor_move_pre(ModContext*, void* args, void*, void*) {
    if (!is_collection_menu_enabled() || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D || !collect2D->mpScreen || !collect2D->mpStick) return HOOK_CONTINUE;

    u8 curX = collect2D->mCursorX;
    u8 curY = collect2D->mCursorY;

    if ((curX >= 3 && curX <= 6 && curY <= 2) || (curX == 6 && curY == 0)) {
        collect2D->mpStick->checkTrigger();

        u8 targetX = curX;
        u8 targetY = curY;
        bool moved = false;

        if (collect2D->mpStick->checkRightTrigger()) {
            for (int tx = curX + 1; tx <= 6; tx++) {
                if (collect2D->field_0x22d[tx][curY] != 0) {
                    targetX = tx;
                    moved = true;
                    break;
                }
            }
        } else if (collect2D->mpStick->checkLeftTrigger()) {
            for (int tx = curX - 1; tx >= 3; tx--) {
                if (collect2D->field_0x22d[tx][curY] != 0) {
                    targetX = tx;
                    moved = true;
                    break;
                }
            }
            if (!moved) {
                targetX = 2;
                targetY = (curY == 0) ? 3 : 4;
                moved = true;
            }
        } else if (collect2D->mpStick->checkDownTrigger()) {
            for (int ty = curY + 1; ty <= 2; ty++) {
                if (curX <= 6 && collect2D->field_0x22d[curX][ty] != 0) {
                    targetX = curX;
                    targetY = ty;
                    moved = true;
                    break;
                }
                int bestX = -1;
                int bestDist = 999;
                for (int tx = 3; tx <= 6; tx++) {
                    if (collect2D->field_0x22d[tx][ty] != 0) {
                        int dist = std::abs((int)tx - (int)curX);
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestX = tx;
                        }
                    }
                }
                if (bestX != -1) {
                    targetX = bestX;
                    targetY = ty;
                    moved = true;
                    break;
                }
            }
            if (!moved) {
                targetX = 3;
                targetY = 3;
                moved = true;
            }
        } else if (collect2D->mpStick->checkUpTrigger()) {
            for (int ty = curY - 1; ty >= 0; ty--) {
                if (curX <= 6 && collect2D->field_0x22d[curX][ty] != 0) {
                    targetX = curX;
                    targetY = ty;
                    moved = true;
                    break;
                }
                int bestX = -1;
                int bestDist = 999;
                for (int tx = 3; tx <= 6; tx++) {
                    if (collect2D->field_0x22d[tx][ty] != 0) {
                        int dist = std::abs((int)tx - (int)curX);
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestX = tx;
                        }
                    }
                }
                if (bestX != -1) {
                    targetX = bestX;
                    targetY = ty;
                    moved = true;
                    break;
                }
            }
        }

        if (moved) {
            collect2D->mCursorX = targetX;
            collect2D->mCursorY = targetY;
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            collect2D->cursorPosSet();
            collect2D->setItemNameString(collect2D->mCursorX, collect2D->mCursorY);
            return HOOK_SKIP_ORIGINAL;
        }
    }
    return HOOK_CONTINUE;
}

HookAction on_set_item_name_string_pre(ModContext*, void* args, void*, void*) {
    if (!is_collection_menu_enabled() || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    u8 x = mods::arg<u8>(args, 1);
    u8 y = mods::arg<u8>(args, 2);
    if (!collect2D || !collect2D->mpScreen) return HOOK_CONTINUE;

    if (x >= 3 && x <= 6 && y <= 2) {
        if (!is_collect_item_unlocked(x, y)) {
            collect2D->setItemNameStringNull();
            return HOOK_SKIP_ORIGINAL;
        }

        if (y == 0) {
            if (x == 3) {
                collect2D->field_0x180 = 0x1a4; // Wooden Sword
                collect2D->mItemNameString = 0x2a4;
            } else if (x == 4) {
                collect2D->field_0x180 = 0x18d; // Ordon Sword
                collect2D->mItemNameString = 0x28d;
            } else if (x == 5) {
                collect2D->field_0x180 = dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e) ? 0x1ae : 0x18e; // Master Sword
                collect2D->mItemNameString = collect2D->field_0x180 + 0x100;
            } else if (x == 6) {
                collect2D->field_0x180 = 0x186; // Heart Container
                collect2D->mItemNameString = 0x286;
            }
        } else if (y == 1) {
            if (x == 3) {
                collect2D->field_0x180 = 0x18f; // Wooden Shield
                collect2D->mItemNameString = 0x28f;
            } else if (x == 4) {
                collect2D->field_0x180 = 0x190; // Ordon Shield
                collect2D->mItemNameString = 0x290;
            } else if (x == 5) {
                collect2D->field_0x180 = 0x191; // Hylian Shield
                collect2D->mItemNameString = 0x291;
            }
        } else if (y == 2) {
            if (x == 3) {
                collect2D->field_0x180 = 0x193; // Ordon Clothes
                collect2D->mItemNameString = 0x293;
            } else if (x == 4) {
                collect2D->field_0x180 = 0x194; // Kokiri Clothes
                collect2D->mItemNameString = 0x294;
            } else if (x == 5) {
                collect2D->field_0x180 = 0x196; // Zora Armor
                collect2D->mItemNameString = 0x296;
            } else if (x == 6) {
                collect2D->field_0x180 = 0x195; // Magic Armor
                collect2D->mItemNameString = 0x295;
            }
        }

        collect2D->field_0x184[x][y] = collect2D->field_0x180;
        collect2D->field_0x1d8[x][y] = collect2D->mItemNameString;
        return HOOK_CONTINUE;
    }
    return HOOK_CONTINUE;
}

HookAction on_get_string_kanji_pre(ModContext*, void* args, void*, void*) {
    if (!is_collection_menu_enabled() || !args) return HOOK_CONTINUE;
    u32 msgID = mods::arg<u32>(args, 1);
    if (msgID == 0x193) {
        TEXT_SPAN o_str = mods::arg<TEXT_SPAN>(args, 2);
        if (o_str) {
            SAFE_STRCPY(o_str, "Ordon Clothes");
        }
        return HOOK_SKIP_ORIGINAL;
    }
    if (msgID == 0x437) {
        TEXT_SPAN o_str = mods::arg<TEXT_SPAN>(args, 2);
        if (o_str) {
            SAFE_STRCPY(o_str, "Unequip");
        }
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction on_get_string_local_pre(ModContext*, void* args, void* ret, void*) {
    if (!is_collection_menu_enabled() || !args) return HOOK_CONTINUE;
    u32 msgID = mods::arg<u32>(args, 1);
    if (msgID == 0x293) {
        dMsgStringBase_c* msgStr = mods::arg<dMsgStringBase_c*>(args, 0);
        J2DTextBox* tb0 = mods::arg<J2DTextBox*>(args, 2);
        J2DTextBox* tb1 = mods::arg<J2DTextBox*>(args, 3);
        COutFont_c* outFont = mods::arg<COutFont_c*>(args, 5);

        if (tb0) {
            if (msgStr) msgStr->resetStringLocal(tb0);
            if (outFont) outFont->reset(tb0);
            if (tb0->getStringPtr()) {
                SAFE_STRCPY(tb0->getStringPtr(), "The clothes Link wore at the beginning of his journey in Ordon Village.");
            }
        }
        if (tb1) {
            if (msgStr) msgStr->resetStringLocal(tb1);
            if (outFont) outFont->reset(tb1);
            if (tb1->getStringPtr()) {
                SAFE_STRCPY(tb1->getStringPtr(), "The clothes Link wore at the beginning of his journey in Ordon Village.");
            }
        }
        if (ret) *(f32*)ret = 0.0f;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
