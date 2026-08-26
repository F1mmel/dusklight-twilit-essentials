#pragma once

#include "z_draw.hpp"
#include "midna_location.hpp"

DEFINE_HOOK(&dMeter2Draw_c::draw, Meter2DrawDrawHook);
DEFINE_HOOK(&dMeter2Draw_c::drawButtonZ, DrawButtonZHook);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconMidonaAlpha, MidonaAlphaHook);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconAlpha, ButtonIconAlphaHook);
DEFINE_HOOK(&dMeter2Draw_c::changeTextureItemXY, ChangeTextureItemXYHook);
DEFINE_HOOK(&dMeter2_c::moveButtonXY, MoveButtonXYHook);
DEFINE_HOOK(&dMeter2_c::_execute, Meter2ExecuteHook);
DEFINE_HOOK(&dMeter2Draw_c::setAlphaButtonChange, SetAlphaButtonChangeHook);

void update_z_item_texture(dMeter2Draw_c* draw) {
    if (draw == nullptr) {
        if (g_meter2_info.getMeterClass() != nullptr) {
            draw = g_meter2_info.getMeterClass()->getMeterDrawPtr();
        }
    }

    if (draw == nullptr || isTitleOrMainMenu() || draw->getMainScreenPtr() == nullptr) {
        return;
    }

    J2DScreen* screen = draw->getMainScreenPtr();
    J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
    if (zbtn == nullptr) {
        return;
    }

    static J2DPane* s_lastZbtn = nullptr;
    if (zbtn != s_lastZbtn) {
        s_lastZbtn = zbtn;
        g_cachedZMainPic = nullptr;
        g_lastLoadedZItem = 0xFF;
        g_drawDigitPic[0] = nullptr;
        g_drawDigitPic[1] = nullptr;
        g_drawDigitPic[2] = nullptr;
        g_zKanteraIcon = nullptr;
    }

    update_midna_pane(draw);

    CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
    if (isWolfPlayer() || is_pause_menu_open(draw)) {
        safe_pane_hide(itemR);
        g_cachedZMainPic = nullptr;
        return;
    }

    if (itemR && itemR->getPanePtr()) {
        J2DPane* itemPane = itemR->getPanePtr();
        if (itemPane && itemPane->getParentPane() != zbtn) {
            J2DPane* oldParent = itemPane->getParentPane();
            if (oldParent != nullptr) {
                oldParent->mPaneTree.removeChild(&itemPane->mPaneTree);
            }
            zbtn->appendChild(itemPane);
            set_pane_influenced_alpha_recursive(itemPane, true);
        }
    }

    ensure_z_buffers();
    ensure_z_slot_initialized();

    u8 zItem = dComIfGp_getSelectItem(2);
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        if (g_zInventorySlot != 0xFF && g_zInventorySlot < 24) {
            zItem = dComIfGs_getItem(g_zInventorySlot, false);
        }
    }

    if (!itemR) {
        return;
    }

    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        safe_pane_hide(itemR);
        g_cachedZMainPic = nullptr;
        return;
    }

    J2DPane* pane = itemR->getPanePtr();
    if (pane == nullptr) {
        return;
    }

    J2DPicture* mainPic = static_cast<J2DPicture*>(pane);
    J2DPicture* shinePic = (draw != nullptr) ? draw->mpItemXYPane[2] : nullptr;
    if (shinePic == nullptr && pane != nullptr && pane->getFirstChild() != nullptr && pane->getFirstChild()->getObject() != nullptr) {
        shinePic = static_cast<J2DPicture*>(pane->getFirstChild()->getObject());
    }

    bool isComboItem = (zItem == dItemNo_BOMB_ARROW_e || zItem == dItemNo_HAWK_ARROW_e || zItem == 0x59 || zItem == 0x5A);

    ResTIMG* mainBuf  = reinterpret_cast<ResTIMG*>(g_zTexBufMain[g_zTexBufIdx]);
    ResTIMG* shineBuf = reinterpret_cast<ResTIMG*>(g_zTexBufShine[g_zTexBufIdx]);

    bool forceReload = (g_lastLoadedZItem != zItem || g_cachedZMainPic != mainPic);
    if (!forceReload && mainPic != nullptr) {
        JUTTexture* currentTex = mainPic->getTexture(0);
        if (currentTex == nullptr || currentTex->getTexInfo() != mainBuf) {
            forceReload = true;
        }
    }
    g_cachedZMainPic = mainPic;

    if (forceReload) {
        g_lastLoadedZItem = zItem;

        g_zTexBufIdx = (g_zTexBufIdx == 0) ? 1 : 0;

        mainBuf = reinterpret_cast<ResTIMG*>(g_zTexBufMain[g_zTexBufIdx]);
        shineBuf = reinterpret_cast<ResTIMG*>(g_zTexBufShine[g_zTexBufIdx]);

        int readResult = dMeter2Info_readItemTexture(
            zItem,
            mainBuf,
            nullptr,
            shineBuf,
            nullptr,
            nullptr, nullptr, nullptr, nullptr, -1);

        if (readResult == 0) {
            g_lastLoadedZItem = 0xFF;
            g_cachedZMainPic = nullptr;
        } else {
            if (mainPic) {
                mainPic->changeTexture(mainBuf, 0);
            }
            if (shinePic) {
                if (readResult > 1 || isComboItem) {
                    shinePic->changeTexture(shineBuf, 0);
                    shinePic->show();
                    shinePic->setAlpha(255);
                } else {
                    shinePic->hide();
                }
            }
            dMeter2Info_setItemColor(zItem, mainPic, shinePic, nullptr, nullptr);
        }

        g_zHasSecondLayer = (readResult > 1 || isComboItem);
    } else {
        if (shinePic) {
            if (g_zHasSecondLayer) {
                shinePic->show();
                shinePic->setAlpha(255);
            } else {
                shinePic->hide();
            }
        }
    }

    f32 texScale = g_drawHIO.mItemScaleAdjustON
        ? (g_drawHIO.mItemScalePercent / 100.0f)
        : (dItem_data::getTexScale(zItem) / 100.0f);

    f32 baseSize = 42.0f;
    f32 w = texScale * ((mainBuf->width  * baseSize) / 48.0f);
    f32 h = texScale * ((mainBuf->height * baseSize) / 48.0f);

    if (shinePic) {
        shinePic->resize(w, h);
    }

    f32 widthShift = (w / 42.0f) * 20.0f;
    f32 offsetX = (baseSize - w) * 0.5f + widthShift - 5;
    f32 offsetY = (baseSize - h) * 0.5f - 10;

    if (draw != nullptr) {
        draw->field_0x6ac[2] = offsetX;
        draw->field_0x6b8[2] = offsetY;
        draw->field_0x6c4[2] = w;
        draw->field_0x6d0[2] = h;

        update_midna_pane(draw);
    }

    safe_pane_resize(itemR, w, h);
    if (shinePic) shinePic->resize(w, h);

    safe_pane_trans(itemR, offsetX, offsetY);

    pane->show();
    pane->setAlpha(255);
    mainPic->setAlpha(255);
    if (shinePic) {
        if (g_zHasSecondLayer) {
            shinePic->show();
            shinePic->setAlpha(255);
        } else {
            shinePic->hide();
        }
    }

    g_cachedZMainPic = mainPic;
    g_cachedZW = w;
    g_cachedZH = h;

    J2DScreen* mainScreen = (draw != nullptr) ? draw->getMainScreenPtr() : nullptr;
    if (mainScreen != nullptr) {
        J2DPane* itemRChild = mainScreen->search(MULTI_CHAR('r_itm_pp'));
        if (itemRChild != nullptr) {
            if (g_zHasSecondLayer) {
                itemRChild->show();
                itemRChild->setAlpha(255);
            } else {
                itemRChild->hide();
            }
        }
    }
}

void draw_item_count_digits(int num, int maxNum, f32 baseX, f32 baseY, f32 iconW, f32 iconH, f32 alphaRate) {
    if (num < 0 || alphaRate <= 0.0f) return;
    if (num > 999) num = 999;
    if (maxNum > 0 && num > maxNum) num = maxNum;

    if (g_drawDigitPic[0] == nullptr) {
        for (int i = 0; i < 3; i++) {
            g_drawDigitPic[i] = JKR_NEW J2DPicture();
        }
    }

    JKRArchive* arc = dComIfGp_getMain2DArchive();
    if (!arc) return;

    JUtility::TColor black(0, 0, 0, 0);
    JUtility::TColor white(255, 255, 255, 255);
    if (maxNum > 0 && num == maxNum) {
        black.set(30, 30, 30, 0);
        white.set(255, 200, 50, 255);
    } else if (num == 0) {
        black.set(30, 30, 30, 0);
        white.set(180, 180, 180, 255);
    }

    auto get_timg = [arc](int d) -> ResTIMG* {
        if (d < 0 || d > 9) d = 0;
        return (ResTIMG*)arc->getResource('TIMG', dMeter2Info_getNumberTextureName(d));
    };

    f32 digitW = 12.0f;
    f32 digitH = 12.0f;
    f32 startX = baseX + (iconW * 0.25f);
    f32 startY = baseY + (iconH * 0.60f);

    u8 a = (u8)(alphaRate * 255.0f);

    for (int i = 0; i < 3; i++) {
        g_drawDigitPic[i]->setBlackWhite(black, white);
        g_drawDigitPic[i]->setAlpha(a);
    }

    if (num < 100) {
        ResTIMG* t1 = get_timg(num / 10);
        ResTIMG* t2 = get_timg(num % 10);
        if (t1 && t2) {
            g_drawDigitPic[0]->changeTexture(t1, 0);
            g_drawDigitPic[0]->draw(startX + iconW - (digitW * 1.8f), startY, digitW, digitH, false, false, false);
            g_drawDigitPic[1]->changeTexture(t2, 0);
            g_drawDigitPic[1]->draw(startX + iconW - (digitW * 0.9f), startY, digitW, digitH, false, false, false);
        }
    } else {
        ResTIMG* t1 = get_timg(num / 100);
        ResTIMG* t2 = get_timg((num / 10) % 10);
        ResTIMG* t3 = get_timg(num % 10);
        if (t1 && t2 && t3) {
            g_drawDigitPic[0]->changeTexture(t1, 0);
            g_drawDigitPic[0]->draw(startX + iconW - (digitW * 2.7f), startY, digitW, digitH, false, false, false);
            g_drawDigitPic[1]->changeTexture(t2, 0);
            g_drawDigitPic[1]->draw(startX + iconW - (digitW * 1.8f), startY, digitW, digitH, false, false, false);
            g_drawDigitPic[2]->changeTexture(t3, 0);
            g_drawDigitPic[2]->draw(startX + iconW - (digitW * 0.9f), startY, digitW, digitH, false, false, false);
        }
    }
}

void on_meter2_draw_draw_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw || !draw->getMainScreenPtr()) {
        return;
    }

    if (isWolfPlayer() || is_pause_menu_open(draw)) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen != nullptr) {
            J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
            if (itemRPane != nullptr) itemRPane->hide();
            J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));
            if (itemRChild != nullptr) itemRChild->hide();
            J2DPane* textPane = screen->search(MULTI_CHAR('r_text_n'));
            if (textPane != nullptr) textPane->hide();
        }
        return;
    }

    J2DScreen* screen = draw->getMainScreenPtr();
    J2DPane* contPane = screen->search(MULTI_CHAR('cont_n'));
    J2DPane* zbtnPane = screen->search(MULTI_CHAR('zbtn_n'));

    if (contPane != nullptr && (!contPane->isVisible() || contPane->getAlpha() == 0)) {
        return;
    }
    if (zbtnPane != nullptr && (!zbtnPane->isVisible() || zbtnPane->getAlpha() == 0)) {
        return;
    }

    f32 alphaRate = 1.0f;
    if (contPane != nullptr) {
        alphaRate *= ((f32)contPane->getAlpha() / 255.0f);
    }
    if (zbtnPane != nullptr) {
        alphaRate *= ((f32)zbtnPane->getAlpha() / 255.0f);
    }

    f32 baseX = 575.0f;
    f32 baseY = 35.0f;
    f32 iconW = 42.0f;
    f32 iconH = 42.0f;

    if (zbtnPane != nullptr) {
        const JGeometry::TBox2<f32>& bounds = zbtnPane->getGlbBounds();
        baseX = bounds.i.x;
        baseY = bounds.i.y;
        iconW = bounds.getWidth();
        iconH = bounds.getHeight();
    }

    draw_z_ammo_digits(draw, baseX, baseY, iconW, iconH, alphaRate);
}

void draw_z_ammo_digits(dMeter2Draw_c* draw, f32 baseX, f32 baseY, f32 iconW, f32 iconH, f32 alphaRate) {
    if (draw == nullptr || isWolfPlayer() || is_pause_menu_open(draw)) {
        return;
    }

    ensure_z_slot_initialized();
    u8 zItem = resolved_select_item(2);
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        if (g_zInventorySlot != 0xFF && g_zInventorySlot < 24) {
            zItem = dComIfGs_getItem(g_zInventorySlot, false);
        }
    }

    if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e) {
        f32 texScale = g_drawHIO.mItemScaleAdjustON
            ? (g_drawHIO.mItemScalePercent / 100.0f)
            : (dItem_data::getTexScale(zItem) / 100.0f);
        f32 baseSize = 42.0f;
        f32 w = texScale * baseSize;
        f32 widthShift = (w / 42.0f) * 24.0f;
        f32 heightShift = 6.0f;
        if (zItem == dItemNo_PACHINKO_e || zItem == 0x4B || zItem == 0x76) {
            widthShift -= 20.0f;
            heightShift -= 10.0f;
        } else if (is_bomb_item(zItem) || zItem == dItemNo_BOMB_ARROW_e || zItem == 0x59) {
            widthShift -= 12.0f;
            heightShift -= 5.0f;
        } else if (zItem == dItemNo_BOW_e || zItem == dItemNo_HAWK_ARROW_e || zItem == 0x43 || zItem == 0x53 || zItem == 0x54 || zItem == 0x55 || zItem == 0x56 || zItem == 0x5A) {
            widthShift -= 10.0f;
            heightShift -= 1.0f;
        }
        baseX += widthShift;
        baseY += heightShift;
    }

    int count    = -1;
    int maxCount = -1;

    // Bomb Arrow
    if (zItem == dItemNo_BOMB_ARROW_e || zItem == 0x59) {
        u8 bombSlot = dComIfGs_getSelectMixItemNoArrowIndex(2);
        u8 bagIdx = (bombSlot >= 15 && bombSlot < 18) ? (bombSlot - 15) : 0;
        u8 bombType = dComIfGs_getItem((u8)(bagIdx + 15), false);
        count = dComIfGs_getBombNum(bagIdx);
        maxCount = dComIfGs_getBombMax(bombType);
    }
    // Bow
    else if (zItem == dItemNo_BOW_e || zItem == dItemNo_HAWK_ARROW_e || zItem == 0x43 || zItem == 0x53 || zItem == 0x54 || zItem == 0x55 || zItem == 0x56 || zItem == 0x5A) {
        count    = dComIfGs_getArrowNum();
        maxCount = dComIfGs_getArrowMax();
    }
    // Slingshot
    else if (zItem == 0x4B || zItem == 0x76) {
        count    = dComIfGs_getPachinkoNum();
        maxCount = dComIfGs_getPachinkoMax();
    }
    // Bombs
    else if (is_bomb_item(zItem)) {
        u8 bombSlot = dComIfGs_getSelectMixItemNoArrowIndex(2);
        u8 bagIdx = (bombSlot >= 15 && bombSlot < 18) ? (bombSlot - 15) : 0;
        u8 bombType = dComIfGs_getItem((u8)(bagIdx + 15), false);
        count    = dComIfGs_getBombNum(bagIdx);
        maxCount = dComIfGs_getBombMax(bombType);
    }
    // Bee larva bottle
    else if (zItem == dItemNo_BEE_CHILD_e || zItem == dItemNo_BEE_ROD_e) {
        u8 bottleSlot = dComIfGs_getSelectItemIndex(2);
        u8 bottleIdx = (bottleSlot >= 11 && bottleSlot < 15) ? (bottleSlot - 11) : 0;
        count = dComIfGs_getBottleNum(bottleIdx);
        maxCount = 10;
    }
    // Lantern
    else if (zItem == 0x48) {
        if (g_zKanteraIcon == nullptr) {
            g_zKanteraIcon = JKR_NEW dKantera_icon_c();
        }
        if (g_zKanteraIcon != nullptr) {
            f32 kanteraX = baseX + iconW * 0.5f - 8.0f;
            f32 kanteraY = baseY + iconH - 4.0f;
            g_zKanteraIcon->setPos(kanteraX, kanteraY);
            g_zKanteraIcon->setScale(0.6f, 0.6f);
            g_zKanteraIcon->setNowGauge(dComIfGs_getMaxOil(), dComIfGs_getOil());
            g_zKanteraIcon->setAlphaRate(alphaRate);
            g_zKanteraIcon->drawSelf();
        }
    }

    if (count >= 0) {
        draw_item_count_digits(count, maxCount, baseX, baseY, iconW, iconH, alphaRate);
    }
}

void on_draw_button_z_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw) {
        return;
    }

    update_midna_pane(draw);

    if (isWolfPlayer()) {
        CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
        safe_pane_hide(itemR);
        J2DPane* textPane = draw->getMainScreenPtr()->search(MULTI_CHAR('r_text_n'));
        if (textPane != nullptr) {
            textPane->hide();
        }
        return;
    }

    ensure_z_slot_initialized();
    u8 zItem = dComIfGp_getSelectItem(2);
    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        if (g_zInventorySlot != 0xFF) {
            zItem = dComIfGs_getItem(g_zInventorySlot, false);
        }
    }

    if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
        CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
        safe_pane_hide(itemR);
        return;
    }

    update_z_item_texture(draw);
}

void on_set_alpha_button_change_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!draw) {
        return;
    }

    update_midna_pane(draw);

    if (isWolfPlayer() || is_pause_menu_open(draw)) {
        return;
    }

    update_z_item_texture(draw);
}

HookAction on_set_button_icon_midona_alpha_pre(ModContext*, void* args, void*, void*) {
    if (!args || isTitleOrMainMenu()) {
        return HOOK_CONTINUE;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    if (!g_configCustomZButtonEnabled) {
        if (draw != nullptr && draw->getMainScreenPtr() != nullptr) {
            J2DScreen* screen = draw->getMainScreenPtr();
            J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
            if (itemRPane != nullptr) itemRPane->hide();
            J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));
            if (itemRChild != nullptr) itemRChild->hide();
        }
        return HOOK_CONTINUE;
    }

    u32& param0 = mods::arg_ref<u32>(args, 1);
    param0 &= ~0x1000000u;

    if (draw != nullptr) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen != nullptr) {
            if (is_pause_menu_open(draw)) {
                J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
                if (itemRPane != nullptr) itemRPane->hide();
                J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));
                if (itemRChild != nullptr) itemRChild->hide();
                J2DPane* textPane = screen->search(MULTI_CHAR('r_text_n'));
                if (textPane != nullptr) textPane->hide();
                J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
                if (midnaPane != nullptr) midnaPane->hide();
                return HOOK_CONTINUE;
            }

            J2DPane* zbtn = screen->search(MULTI_CHAR('zbtn_n'));
            if (zbtn != nullptr) {
                zbtn->show();
                zbtn->setAlpha(255);
            }
            J2DPane* rbtn = screen->search(MULTI_CHAR('rbtn_n'));
            if (rbtn != nullptr) {
                rbtn->show();
                rbtn->setAlpha(255);
            }

            u8 zItem = dComIfGp_getSelectItem(2);
            if (zItem == 0xFF || zItem == 0x00 || zItem == dItemNo_NONE_e) {
                if (g_zInventorySlot != 0xFF) {
                    zItem = dComIfGs_getItem(g_zInventorySlot, false);
                }
            }

            J2DPane* itemRPane = screen->search(MULTI_CHAR('r_itm_p'));
            J2DPane* itemRChild = screen->search(MULTI_CHAR('r_itm_pp'));

            if (isWolfPlayer()) {
                if (itemRPane != nullptr) itemRPane->hide();
                if (itemRChild != nullptr) itemRChild->hide();
                J2DPane* textPane = screen->search(MULTI_CHAR('r_text_n'));
                if (textPane != nullptr) textPane->hide();
            } else if (zItem != 0xFF && zItem != 0x00 && zItem != dItemNo_NONE_e) {
                if (itemRPane != nullptr) {
                    itemRPane->show();
                    itemRPane->setAlpha(255);
                }
                if (itemRChild != nullptr) {
                    if (g_zHasSecondLayer) {
                        itemRChild->show();
                        itemRChild->setAlpha(255);
                    } else {
                        itemRChild->hide();
                    }
                }
            }
        }
    }

    return HOOK_CONTINUE;
}

HookAction on_set_button_icon_alpha_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return HOOK_CONTINUE;
    }

    int i_no = mods::arg<int>(args, 1);
    if (i_no != 2) {
        return HOOK_CONTINUE;
    }

    g_meter2_info.onUseButton(0x800);

    return HOOK_CONTINUE;
}

HookAction on_change_texture_item_xy_pre(ModContext*, void* args, void*, void*) {
    if (!args) {
        return HOOK_CONTINUE;
    }

    int i_no = mods::arg<int>(args, 1);
    if (i_no != 2) {
        return HOOK_CONTINUE;
    }

    if (isWolfPlayer()) {
        CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
        safe_pane_hide(itemR);
        return HOOK_SKIP_ORIGINAL;
    }

    dMeter2Draw_c* draw = mods::arg<dMeter2Draw_c*>(args, 0);
    update_z_item_texture(draw);
    return HOOK_SKIP_ORIGINAL;
}

void on_move_button_xy_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args) {
        return;
    }

    dMeter2_c* meter2 = mods::arg<dMeter2_c*>(args, 0);
    if (!meter2) {
        return;
    }

    dMeter2Draw_c* draw = meter2->getMeterDrawPtr();
    if (!draw) {
        return;
    }

    u8 zSlot = g_zInventorySlot;

    if (zSlot == 0xFF) {
        zSlot = find_slot_for_item(dComIfGs_getSelectItemIndex(2));
        g_zInventorySlot = zSlot;
    }

    CPaneMgr* itemR = dMeter2Info_getMeterItemPanePtr(2);
    if (zSlot != 0xFF) {
        safe_pane_show(itemR);
    } else {
        safe_pane_hide(itemR);
    }

    update_z_item_texture(draw);
}

void on_meter2_execute_post(ModContext*, void* args, void*, void*) {
    if (!g_configCustomZButtonEnabled || !args || isTitleOrMainMenu()) {
        return;
    }

    dMeter2_c* meter2 = mods::arg<dMeter2_c*>(args, 0);
    if (!meter2) {
        return;
    }

    dMeter2Draw_c* draw = meter2->getMeterDrawPtr();
    if (draw) {
        update_midna_pane(draw);
        if (!isWolfPlayer() && !is_pause_menu_open(draw)) {
            update_z_item_texture(draw);
        }
    }
}
