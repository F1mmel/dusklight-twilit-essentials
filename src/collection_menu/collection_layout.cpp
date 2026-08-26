#include "collection_layout.hpp"

void update_screen_bases(J2DScreen* screen, JKRExpHeap* heap) {
    if (!screen) return;

    if (screen != s_cachedScreen) {
        s_cachedScreen = screen;
        s_paneKenMid = screen->search(MULTI_CHAR('ken_mid'));
        s_paneTateMid = screen->search(MULTI_CHAR('tate_mid'));
        s_paneFukuStart = screen->search(MULTI_CHAR('fuku_ord'));
        s_picKenMidFrame = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('ken_gm')));
        s_picKenMidIcon = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('ken_im')));
        s_picTateMidFrame = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tate_gm')));
        s_picTateMidIcon = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tate_im')));
        s_picFukuStartFrame = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_go')));
        s_picFukuStartIcon = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_io')));
        s_picTunagiKen2 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tuna_k2')));
        s_picTunagiTate2 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tuna_t2')));
        s_picTunagiFuku3 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tuna_f3')));

        J2DPane* ken_n0 = screen->search(MULTI_CHAR('ken_n0'));
        J2DPane* tate_n0 = screen->search(MULTI_CHAR('tate_n0'));
        J2DPane* fuku_n0 = screen->search(MULTI_CHAR('fuku_n0'));

        J2DPane* ken_g0 = screen->search(MULTI_CHAR('ken_g_0'));
        J2DPane* ken_01 = screen->search(MULTI_CHAR('ken_01'));
        J2DPane* tate_g0 = screen->search(MULTI_CHAR('tate_g_0'));
        J2DPane* tate_00 = screen->search(MULTI_CHAR('tate_00'));
        J2DPane* fuku_g0 = screen->search(MULTI_CHAR('fuku_g_0'));
        J2DPane* fuku_00 = screen->search(MULTI_CHAR('fuku_00'));

        J2DPane* tunagi01 = screen->search(MULTI_CHAR('tunagi01'));
        J2DPane* tunagi03 = screen->search(MULTI_CHAR('tunagi03'));
        J2DPane* tunagi06 = screen->search(MULTI_CHAR('tunagi06'));

        ResTIMG* ordonClothesTex = get_ordon_clothes_texture();

        JKRHeap* oldHeap = nullptr;
        if (heap != nullptr) {
            oldHeap = mDoExt_setCurrentHeap(heap);
        }

        // 1. Ordon Sword Frame (Slot 2, Row 0 - attached to frames parent)
        if (!s_picKenMidFrame && ken_g0 && ken_g0->getParentPane()) {
            const ResTIMG* frameTex = safe_get_tex_info(ken_g0);
            if (frameTex) {
                s_picKenMidFrame = JKR_NEW J2DPicture(MULTI_CHAR('ken_gm'), ken_g0->mBounds, frameTex, nullptr);
                s_picKenMidFrame->mKind = 'PIC1';
                s_picKenMidFrame->setBasePosition((J2DBasePosition)ken_g0->mBasePosition);
                s_picKenMidFrame->mBounds.set(-23.5f, -23.5f, 23.5f, 23.5f);
                J2DPicture* picKenG0 = static_cast<J2DPicture*>(ken_g0);
                s_picKenMidFrame->setCornerColor(
                    picKenG0->corner(0),
                    picKenG0->corner(1),
                    picKenG0->corner(2),
                    picKenG0->corner(3)
                );
                s_picKenMidFrame->setBlackWhite(picKenG0->getBlack(), picKenG0->getWhite());
                ken_g0->getParentPane()->appendChild(s_picKenMidFrame);
            }
        }

        // 1. Ordon Sword Icon (Slot 2, Row 0)
        if (!s_paneKenMid && ken_n0 && ken_n0->getParentPane()) {
            s_paneKenMid = JKR_NEW J2DPane(ken_n0->getParentPane(), true, MULTI_CHAR('ken_mid'), ken_n0->mBounds);
            s_paneKenMid->setBasePosition((J2DBasePosition)ken_n0->mBasePosition);
            s_paneKenMid->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);

            const ResTIMG* iconTex = safe_get_tex_info(ken_01);
            if (iconTex) {
                s_picKenMidIcon = JKR_NEW J2DPicture(MULTI_CHAR('ken_im'), ken_01 ? ken_01->mBounds : ken_n0->mBounds, iconTex, nullptr);
                s_picKenMidIcon->mKind = 'PIC1';
                if (ken_01) s_picKenMidIcon->setBasePosition((J2DBasePosition)ken_01->mBasePosition);
                s_picKenMidIcon->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
                s_picKenMidIcon->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
                s_paneKenMid->appendChild(s_picKenMidIcon);
                s_picKenMidIcon->translate(0.0f, 0.0f);
            }
        }

        // 2. Ordon Shield Frame (Slot 2, Row 1 - attached to frames parent)
        if (!s_picTateMidFrame && tate_g0 && tate_g0->getParentPane()) {
            const ResTIMG* frameTex = safe_get_tex_info(tate_g0);
            if (frameTex) {
                s_picTateMidFrame = JKR_NEW J2DPicture(MULTI_CHAR('tate_gm'), tate_g0->mBounds, frameTex, nullptr);
                s_picTateMidFrame->mKind = 'PIC1';
                s_picTateMidFrame->setBasePosition((J2DBasePosition)tate_g0->mBasePosition);
                s_picTateMidFrame->mBounds.set(-23.5f, -23.5f, 23.5f, 23.5f);
                J2DPicture* picTateG0 = static_cast<J2DPicture*>(tate_g0);
                s_picTateMidFrame->setCornerColor(
                    picTateG0->corner(0),
                    picTateG0->corner(1),
                    picTateG0->corner(2),
                    picTateG0->corner(3)
                );
                s_picTateMidFrame->setBlackWhite(picTateG0->getBlack(), picTateG0->getWhite());
                tate_g0->getParentPane()->appendChild(s_picTateMidFrame);
            }
        }

        // 2. Ordon Shield Icon (Slot 2, Row 1)
        if (!s_paneTateMid && tate_n0 && tate_n0->getParentPane()) {
            s_paneTateMid = JKR_NEW J2DPane(tate_n0->getParentPane(), true, MULTI_CHAR('tate_mid'), tate_n0->mBounds);
            s_paneTateMid->setBasePosition((J2DBasePosition)tate_n0->mBasePosition);
            s_paneTateMid->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);

            const ResTIMG* iconTex = safe_get_tex_info(tate_00);
            if (iconTex) {
                s_picTateMidIcon = JKR_NEW J2DPicture(MULTI_CHAR('tate_im'), tate_00 ? tate_00->mBounds : tate_n0->mBounds, iconTex, nullptr);
                s_picTateMidIcon->mKind = 'PIC1';
                if (tate_00) s_picTateMidIcon->setBasePosition((J2DBasePosition)tate_00->mBasePosition);
                s_picTateMidIcon->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
                s_picTateMidIcon->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
                s_paneTateMid->appendChild(s_picTateMidIcon);
                s_picTateMidIcon->translate(0.0f, 0.0f);
            }
        }

        // 3. Ordon Clothes Frame (Slot 1, Row 2 - attached to frames parent)
        if (!s_picFukuStartFrame && fuku_g0 && fuku_g0->getParentPane()) {
            const ResTIMG* frameTex = safe_get_tex_info(fuku_g0);
            if (frameTex) {
                s_picFukuStartFrame = JKR_NEW J2DPicture(MULTI_CHAR('fuku_go'), fuku_g0->mBounds, frameTex, nullptr);
                s_picFukuStartFrame->mKind = 'PIC1';
                s_picFukuStartFrame->setBasePosition((J2DBasePosition)fuku_g0->mBasePosition);
                s_picFukuStartFrame->mBounds.set(-23.5f, -23.5f, 23.5f, 23.5f);
                J2DPicture* picFukuG0 = static_cast<J2DPicture*>(fuku_g0);
                s_picFukuStartFrame->setCornerColor(
                    picFukuG0->corner(0),
                    picFukuG0->corner(1),
                    picFukuG0->corner(2),
                    picFukuG0->corner(3)
                );
                s_picFukuStartFrame->setBlackWhite(picFukuG0->getBlack(), picFukuG0->getWhite());
                fuku_g0->getParentPane()->appendChild(s_picFukuStartFrame);
            }
        }

        // 3. Ordon Clothes Icon (Slot 1, Row 2)
        if (!s_paneFukuStart && fuku_n0 && fuku_n0->getParentPane()) {
            s_paneFukuStart = JKR_NEW J2DPane(fuku_n0->getParentPane(), true, MULTI_CHAR('fuku_ord'), fuku_n0->mBounds);
            s_paneFukuStart->setBasePosition((J2DBasePosition)fuku_n0->mBasePosition);
            s_paneFukuStart->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);

            if (ordonClothesTex != nullptr) {
                s_picFukuStartIcon = JKR_NEW J2DPicture(MULTI_CHAR('fuku_io'), fuku_00 ? fuku_00->mBounds : fuku_n0->mBounds, ordonClothesTex, nullptr);
                s_picFukuStartIcon->mKind = 'PIC1';
                if (fuku_00) s_picFukuStartIcon->setBasePosition((J2DBasePosition)fuku_00->mBasePosition);
                s_picFukuStartIcon->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
                s_picFukuStartIcon->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
                s_picFukuStartIcon->setAlpha(255);
                s_paneFukuStart->appendChild(s_picFukuStartIcon);
                s_picFukuStartIcon->translate(0.0f, 0.0f);
            }
        }

        // 4. Sword Connector 2 (Slot 2 to Slot 3)
        if (!s_picTunagiKen2 && tunagi01 && tunagi01->getParentPane()) {
            const ResTIMG* tex = safe_get_tex_info(tunagi01);
            if (tex) {
                s_picTunagiKen2 = JKR_NEW J2DPicture(MULTI_CHAR('tuna_k2'), tunagi01->mBounds, tex, nullptr);
                s_picTunagiKen2->setBasePosition((J2DBasePosition)tunagi01->mBasePosition);
                s_picTunagiKen2->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
                static_cast<CustomPicture*>(s_picTunagiKen2)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi01));
                tunagi01->getParentPane()->appendChild(s_picTunagiKen2);
            }
        }

        // 5. Shield Connector 2 (Slot 2 to Slot 3)
        if (!s_picTunagiTate2 && tunagi03 && tunagi03->getParentPane()) {
            const ResTIMG* tex = safe_get_tex_info(tunagi03);
            if (tex) {
                s_picTunagiTate2 = JKR_NEW J2DPicture(MULTI_CHAR('tuna_t2'), tunagi03->mBounds, tex, nullptr);
                s_picTunagiTate2->setBasePosition((J2DBasePosition)tunagi03->mBasePosition);
                s_picTunagiTate2->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
                static_cast<CustomPicture*>(s_picTunagiTate2)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi03));
                tunagi03->getParentPane()->appendChild(s_picTunagiTate2);
            }
        }

        // 6. Clothes Connector 3 (Slot 3 to Slot 4)
        if (!s_picTunagiFuku3 && tunagi06 && tunagi06->getParentPane()) {
            const ResTIMG* tex = safe_get_tex_info(tunagi06);
            if (tex) {
                s_picTunagiFuku3 = JKR_NEW J2DPicture(MULTI_CHAR('tuna_f3'), tunagi06->mBounds, tex, nullptr);
                s_picTunagiFuku3->setBasePosition((J2DBasePosition)tunagi06->mBasePosition);
                s_picTunagiFuku3->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
                static_cast<CustomPicture*>(s_picTunagiFuku3)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi06));
                tunagi06->getParentPane()->appendChild(s_picTunagiFuku3);
            }
        }

        if (oldHeap != nullptr) {
            mDoExt_setCurrentHeap(oldHeap);
        }
    }
}

void apply_collect_shifts(dMenu_Collect2D_c* collect2D) {
    if (!collect2D || !collect2D->mpScreen) return;
    J2DScreen* screen = collect2D->mpScreen;
    update_screen_bases(screen, collect2D->mpHeap);

    J2DPane* ken_n0 = screen->search(MULTI_CHAR('ken_n0'));
    J2DPane* ken_n1 = screen->search(MULTI_CHAR('ken_n1'));
    J2DPane* tate_n0 = screen->search(MULTI_CHAR('tate_n0'));
    J2DPane* tate_n1 = screen->search(MULTI_CHAR('tate_n1'));
    J2DPane* fuku_n0 = screen->search(MULTI_CHAR('fuku_n0'));
    J2DPane* fuku_n1 = screen->search(MULTI_CHAR('fuku_n1'));
    J2DPane* fuku_n2 = screen->search(MULTI_CHAR('fuku_n2'));
    J2DPane* heart_n = screen->search(MULTI_CHAR('heart_n'));
    J2DPane* kamen_n = screen->search(MULTI_CHAR('kamen_n'));
    J2DPane* modelbgn = screen->search(MULTI_CHAR('modelbgn'));

    J2DPane* ken_g0 = screen->search(MULTI_CHAR('ken_g_0'));
    J2DPane* ken_gm = screen->search(MULTI_CHAR('ken_gm'));
    J2DPane* ken_g1 = screen->search(MULTI_CHAR('ken_g_1'));

    J2DPane* tate_g0 = screen->search(MULTI_CHAR('tate_g_0'));
    J2DPane* tate_gm = screen->search(MULTI_CHAR('tate_gm'));
    J2DPane* tate_g1 = screen->search(MULTI_CHAR('tate_g_1'));

    J2DPane* fuku_go = screen->search(MULTI_CHAR('fuku_go'));
    J2DPane* fuku_g0 = screen->search(MULTI_CHAR('fuku_g_0'));
    J2DPane* fuku_g1 = screen->search(MULTI_CHAR('fuku_g_1'));
    J2DPane* fuku_g2 = screen->search(MULTI_CHAR('fuku_g_2'));

    f32 dx = s_col_dx + 5.0f;
    f32 baseX = s_ken_n0_origX;
    f32 swordFrameBaseX = baseX - 24.5f;
    f32 shieldFrameBaseX = baseX - 24.5f;
    f32 clothesFrameBaseX = baseX - 24.5f;

    // Row 0 (Swords)
    set_pane_pos(ken_n0, baseX, s_ken_n0_origY);
    set_pane_pos(s_paneKenMid, baseX + dx, s_ken_n0_origY);
    set_pane_pos(ken_n1, baseX + 2.0f * dx, s_ken_n0_origY);

    set_pane_pos(ken_g0, swordFrameBaseX, s_ken_g0_origY);
    set_pane_pos(ken_gm, swordFrameBaseX + dx, s_ken_g0_origY);
    set_pane_pos(ken_g1, swordFrameBaseX + 2.0f * dx, s_ken_g0_origY);

    // Row 1 (Shields)
    set_pane_pos(tate_n0, baseX, s_tate_n0_origY);
    set_pane_pos(s_paneTateMid, baseX + dx, s_tate_n0_origY);
    set_pane_pos(tate_n1, baseX + 2.0f * dx, s_tate_n0_origY);

    set_pane_pos(tate_g0, shieldFrameBaseX, s_tate_g0_origY);
    set_pane_pos(tate_gm, shieldFrameBaseX + dx, s_tate_g0_origY);
    set_pane_pos(tate_g1, shieldFrameBaseX + 2.0f * dx, s_tate_g0_origY);

    // Row 2 (Clothes)
    set_pane_pos(s_paneFukuStart, baseX, s_fuku_n0_origY);
    set_pane_pos(fuku_n0, baseX + dx, s_fuku_n0_origY);
    set_pane_pos(fuku_n1, baseX + 2.0f * dx, s_fuku_n0_origY);
    set_pane_pos(fuku_n2, baseX + 3.0f * dx, s_fuku_n0_origY);

    set_pane_pos(fuku_go, clothesFrameBaseX, s_fuku_g0_origY);
    set_pane_pos(fuku_g0, clothesFrameBaseX + dx, s_fuku_g0_origY);
    set_pane_pos(fuku_g1, clothesFrameBaseX + 2.0f * dx, s_fuku_g0_origY);
    set_pane_pos(fuku_g2, clothesFrameBaseX + 3.0f * dx, s_fuku_g0_origY);

    // Shift mirror 40px, background 20px
    f32 mirrorShiftX = 40.0f;
    set_pane_pos(heart_n, baseX + 3.0f * dx, s_heart_n_origY);
    set_pane_pos(kamen_n, s_kamen_n_origX + mirrorShiftX, s_kamen_n_origY);
    set_pane_pos(modelbgn, s_modelbgn_origX + mirrorShiftX - mirrorShiftX / 2.0f, s_modelbgn_origY);

    // Ensure bounds and local translations on custom panes and children
    if (s_paneKenMid) s_paneKenMid->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
    if (s_picKenMidFrame) s_picKenMidFrame->mBounds.set(-23.5f, -23.5f, 23.5f, 23.5f);
    if (s_picKenMidIcon) {
        s_picKenMidIcon->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
        s_picKenMidIcon->translate(0.0f, 0.0f);
    }

    if (s_paneTateMid) s_paneTateMid->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
    if (s_picTateMidFrame) s_picTateMidFrame->mBounds.set(-23.5f, -23.5f, 23.5f, 23.5f);
    if (s_picTateMidIcon) {
        s_picTateMidIcon->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
        s_picTateMidIcon->translate(0.0f, 0.0f);
    }

    if (s_paneFukuStart) s_paneFukuStart->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
    if (s_picFukuStartFrame) s_picFukuStartFrame->mBounds.set(-23.5f, -23.5f, 23.5f, 23.5f);
    if (s_picFukuStartIcon) {
        s_picFukuStartIcon->mBounds.set(-22.5f, -22.5f, 22.5f, 22.5f);
        s_picFukuStartIcon->translate(0.0f, 0.0f);
        ResTIMG* ordonClothesTex = get_ordon_clothes_texture();
        if (ordonClothesTex) {
            s_picFukuStartIcon->changeTexture(ordonClothesTex, 0);
        }
    }

    // Connectors (tunagi)
    J2DPane* tunagi00 = screen->search(MULTI_CHAR('tunagi00'));
    J2DPane* tunagi01 = screen->search(MULTI_CHAR('tunagi01'));
    J2DPane* tunagi_k2 = screen->search(MULTI_CHAR('tuna_k2'));

    J2DPane* tunagi04 = screen->search(MULTI_CHAR('tunagi04'));
    J2DPane* tunagi03 = screen->search(MULTI_CHAR('tunagi03'));
    J2DPane* tunagi_t2 = screen->search(MULTI_CHAR('tuna_t2'));

    J2DPane* tunagi07 = screen->search(MULTI_CHAR('tunagi07'));
    J2DPane* tunagi06 = screen->search(MULTI_CHAR('tunagi06'));
    J2DPane* tunagi08 = screen->search(MULTI_CHAR('tunagi08'));
    J2DPane* tunagi_f3 = screen->search(MULTI_CHAR('tuna_f3'));

    // Position Sword connectors
    set_pane_pos(tunagi00, swordFrameBaseX - 0.5f * dx, -44.0f);
    set_pane_pos(tunagi01, swordFrameBaseX + 0.5f * dx, -44.0f);
    set_pane_pos(tunagi_k2, swordFrameBaseX + 1.5f * dx, -44.0f);

    // Position Shield connectors
    set_pane_pos(tunagi04, shieldFrameBaseX - 0.5f * dx, 13.0f);
    set_pane_pos(tunagi03, shieldFrameBaseX + 0.5f * dx, 13.0f);
    set_pane_pos(tunagi_t2, shieldFrameBaseX + 1.5f * dx, 13.0f);

    // Position Clothes connectors
    set_pane_pos(tunagi07, clothesFrameBaseX - 0.5f * dx, 70.0f);
    set_pane_pos(tunagi06, clothesFrameBaseX + 0.5f * dx, 70.0f);
    if (tunagi08) {
        if (tunagi06) static_cast<CustomPicture*>(tunagi08)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi06));
        tunagi08->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
        set_pane_pos(tunagi08, clothesFrameBaseX + 1.5f * dx, 70.0f);
    }
    set_pane_pos(tunagi_f3, clothesFrameBaseX + 2.5f * dx, 70.0f);

    if (s_picTunagiKen2 && tunagi01) {
        static_cast<CustomPicture*>(s_picTunagiKen2)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi01));
        s_picTunagiKen2->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
    }
    if (s_picTunagiTate2 && tunagi03) {
        static_cast<CustomPicture*>(s_picTunagiTate2)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi03));
        s_picTunagiTate2->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
    }
    if (s_picTunagiFuku3 && tunagi06) {
        static_cast<CustomPicture*>(s_picTunagiFuku3)->copyVisualsFrom(static_cast<J2DPicture*>(tunagi06));
        s_picTunagiFuku3->mBounds.set(-6.0f, -18.0f, 6.0f, 18.0f);
    }

    // Check item possession/unlocked status
    // Link always has Wooden Sword & Ordon Clothes from game start
    bool hasWoodSword = true;

    bool hasOrdonSword = dComIfGs_isItemFirstBit(dItemNo_SWORD_e);
    if (dComIfGs_getSelectEquipSword() == dItemNo_SWORD_e) hasOrdonSword = true;

    bool hasMasterSword = dComIfGs_isItemFirstBit(dItemNo_MASTER_SWORD_e) || dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e);
    if (dComIfGs_getSelectEquipSword() == dItemNo_MASTER_SWORD_e || dComIfGs_getSelectEquipSword() == dItemNo_LIGHT_SWORD_e) hasMasterSword = true;

    bool hasHeart = (dComIfGs_getMaxLife() > 15);

    bool hasWoodShield = dComIfGs_isItemFirstBit(dItemNo_WOOD_SHIELD_e);
    if (dComIfGs_getSelectEquipShield() == dItemNo_WOOD_SHIELD_e) hasWoodShield = true;

    bool hasOrdonShield = dComIfGs_isItemFirstBit(dItemNo_SHIELD_e);
    if (dComIfGs_getSelectEquipShield() == dItemNo_SHIELD_e) hasOrdonShield = true;

    bool hasHylianShield = dComIfGs_isItemFirstBit(dItemNo_HYLIA_SHIELD_e);
    if (dComIfGs_getSelectEquipShield() == dItemNo_HYLIA_SHIELD_e) hasHylianShield = true;

    bool hasKokiriClothes = dComIfGs_isItemFirstBit(dItemNo_WEAR_KOKIRI_e);
    if (dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_KOKIRI_e) hasKokiriClothes = true;

    bool hasZoraArmor = dComIfGs_isItemFirstBit(dItemNo_WEAR_ZORA_e);
    if (dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_ZORA_e) hasZoraArmor = true;

    bool hasMagicArmor = dComIfGs_isItemFirstBit(dItemNo_ARMOR_e);
    if (dComIfGs_getSelectEquipClothes() == dItemNo_ARMOR_e) hasMagicArmor = true;

    // Make all grid positions selectable by the cursor
    collect2D->field_0x22d[3][0] = 1;
    collect2D->field_0x22d[4][0] = 1;
    collect2D->field_0x22d[5][0] = 1;
    collect2D->field_0x22d[6][0] = 1;

    collect2D->field_0x22d[3][1] = 1;
    collect2D->field_0x22d[4][1] = 1;
    collect2D->field_0x22d[5][1] = 1;
    collect2D->field_0x22d[6][1] = 0;

    collect2D->field_0x22d[3][2] = 1;
    collect2D->field_0x22d[4][2] = 1;
    collect2D->field_0x22d[5][2] = 1;
    collect2D->field_0x22d[6][2] = 1;

    // Connectors are ALWAYS drawn
    if (tunagi00) tunagi00->show();
    if (tunagi01) tunagi01->show();
    if (tunagi_k2) tunagi_k2->show();

    if (tunagi04) tunagi04->show();
    if (tunagi03) tunagi03->show();
    if (tunagi_t2) tunagi_t2->show();

    if (tunagi07) tunagi07->show();
    if (tunagi06) tunagi06->show();
    if (tunagi08) tunagi08->show();
    if (tunagi_f3) tunagi_f3->show();

    // Frames (rects) are ALWAYS drawn
    if (ken_g0) ken_g0->show();
    if (ken_gm) ken_gm->show();
    if (ken_g1) ken_g1->show();

    if (tate_g0) tate_g0->show();
    if (tate_gm) tate_gm->show();
    if (tate_g1) tate_g1->show();

    if (fuku_go) fuku_go->show();
    if (fuku_g0) fuku_g0->show();
    if (fuku_g1) fuku_g1->show();
    if (fuku_g2) fuku_g2->show();

    // Synchronize scaling across custom panes
    if (ken_n0 && s_paneKenMid) s_paneKenMid->scale(ken_n0->getScaleX(), ken_n0->getScaleY());
    if (tate_n0 && s_paneTateMid) s_paneTateMid->scale(tate_n0->getScaleX(), tate_n0->getScaleY());
    if (fuku_n0 && s_paneFukuStart) s_paneFukuStart->scale(fuku_n0->getScaleX(), fuku_n0->getScaleY());

    // Inner item icons: only shown if the item is unlocked!
    // 1. Swords
    J2DPane* ken_00 = screen->search(MULTI_CHAR('ken_00'));
    J2DPane* ken_01 = screen->search(MULTI_CHAR('ken_01'));
    if (ken_n0) ken_n0->show();
    if (ken_00) { ken_00->show(); ken_00->translate(0.0f, 0.0f); }
    if (ken_01) ken_01->hide();

    if (s_paneKenMid) { if (hasOrdonSword) s_paneKenMid->show(); else s_paneKenMid->hide(); }
    if (s_picKenMidIcon) { if (hasOrdonSword) s_picKenMidIcon->show(); else s_picKenMidIcon->hide(); }

    if (ken_n1) { if (hasMasterSword) ken_n1->show(); else ken_n1->hide(); }

    // Heart container
    if (heart_n) heart_n->show();

    // 2. Shields
    J2DPane* tate_00 = screen->search(MULTI_CHAR('tate_00'));
    J2DPane* tate_01 = screen->search(MULTI_CHAR('tate_01'));
    if (tate_n0) { if (hasWoodShield) tate_n0->show(); else tate_n0->hide(); }
    if (tate_01) { if (hasWoodShield) { tate_01->show(); tate_01->translate(0.0f, 0.0f); } else tate_01->hide(); }
    if (tate_00) tate_00->hide();

    if (s_paneTateMid) { if (hasOrdonShield) s_paneTateMid->show(); else s_paneTateMid->hide(); }
    if (s_picTateMidIcon) { if (hasOrdonShield) s_picTateMidIcon->show(); else s_picTateMidIcon->hide(); }

    if (tate_n1) { if (hasHylianShield) tate_n1->show(); else tate_n1->hide(); }

    // 3. Clothes
    if (s_paneFukuStart) s_paneFukuStart->show();
    if (s_picFukuStartIcon) s_picFukuStartIcon->show();

    if (fuku_n0) { if (hasKokiriClothes) fuku_n0->show(); else fuku_n0->hide(); }
    J2DPane* p0 = screen->search(MULTI_CHAR('fuku_00'));
    if (p0) { if (hasKokiriClothes) p0->show(); else p0->hide(); }

    if (fuku_n1) { if (hasZoraArmor) fuku_n1->show(); else fuku_n1->hide(); }
    J2DPane* p1 = screen->search(MULTI_CHAR('fuku_01'));
    if (p1) { if (hasZoraArmor) p1->show(); else p1->hide(); }

    if (fuku_n2) { if (hasMagicArmor) fuku_n2->show(); else fuku_n2->hide(); }
    J2DPane* p2 = screen->search(MULTI_CHAR('fuku_02'));
    if (p2) { if (hasMagicArmor) p2->show(); else p2->hide(); }

    // Set correct textures on all icon panes
    if (ken_00) {
        const ResTIMG* tex = (const ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_item_icon_sword_a.bti");
        if (tex) static_cast<J2DPicture*>(ken_00)->changeTexture(tex, 0);
    }
    if (s_picKenMidIcon) {
        const ResTIMG* tex = (const ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_item_icon_sword_b.bti");
        if (tex) s_picKenMidIcon->changeTexture(tex, 0);
    }
    if (ken_n1) {
        J2DPane* p = screen->search(MULTI_CHAR('ken_02'));
        if (p) {
            const ResTIMG* tex = (const ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_item_icon_sword_c.bti");
            if (tex) static_cast<J2DPicture*>(p)->changeTexture(tex, 0);
        }
    }
    if (tate_01) {
        const ResTIMG* tex = (const ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_item_icon_shield_a.bti");
        if (tex) static_cast<J2DPicture*>(tate_01)->changeTexture(tex, 0);
    }
    if (s_picTateMidIcon) {
        const ResTIMG* tex = (const ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_item_icon_shield_b.bti");
        if (tex) s_picTateMidIcon->changeTexture(tex, 0);
    }
    if (tate_n1) {
        J2DPane* p = screen->search(MULTI_CHAR('tate_02'));
        if (p) {
            const ResTIMG* tex = (const ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_item_icon_shield_c.bti");
            if (tex) static_cast<J2DPicture*>(p)->changeTexture(tex, 0);
        }
    }

    update_frame_highlights(collect2D);
}

void update_frame_highlights(dMenu_Collect2D_c* collect2D) {
    if (!collect2D || !collect2D->mpScreen) return;
    J2DScreen* screen = collect2D->mpScreen;

    u8 currentSword = dComIfGs_getSelectEquipSword();
    u8 currentShield = dComIfGs_getSelectEquipShield();
    u8 currentClothes = dComIfGs_getSelectEquipClothes();

    // 1. Swords
    // Slot 1 (Wooden Sword)
    J2DPicture* ken_g0 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('ken_g_0')));
    if (ken_g0) {
        bool eq = (currentSword == dItemNo_WOOD_STICK_e);
        ken_g0->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                              eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 2 (Ordon Sword)
    J2DPicture* picKenMidFrame = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('ken_gm')));
    if (picKenMidFrame) {
        bool eq = (currentSword == dItemNo_SWORD_e);
        picKenMidFrame->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                              eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 3 (Master Sword)
    J2DPicture* ken_g1 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('ken_g_1')));
    if (ken_g1) {
        bool eq = (currentSword == dItemNo_MASTER_SWORD_e || currentSword == dItemNo_LIGHT_SWORD_e);
        ken_g1->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                              eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }

    // 2. Shields
    // Slot 1 (Wooden Shield)
    J2DPicture* tate_g0 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tate_g_0')));
    if (tate_g0) {
        bool eq = (currentShield == dItemNo_WOOD_SHIELD_e);
        tate_g0->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                               eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 2 (Ordon Shield)
    J2DPicture* picTateMidFrame = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tate_gm')));
    if (picTateMidFrame) {
        bool eq = (currentShield == dItemNo_SHIELD_e);
        picTateMidFrame->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                       eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 3 (Hylian Shield)
    J2DPicture* tate_g1 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tate_g_1')));
    if (tate_g1) {
        bool eq = (currentShield == dItemNo_HYLIA_SHIELD_e);
        tate_g1->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                               eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }

    // 3. Clothes
    // Slot 1 (Ordon Clothes)
    J2DPicture* picFukuStartFrame = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_go')));
    if (picFukuStartFrame) {
        bool eq = (currentClothes == dItemNo_WEAR_CASUAL_e);
        picFukuStartFrame->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                         eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 2 (Kokiri Tunic)
    J2DPicture* fuku_g0 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_g_0')));
    if (fuku_g0) {
        bool eq = (currentClothes == dItemNo_WEAR_KOKIRI_e || (currentClothes != dItemNo_WEAR_CASUAL_e && currentClothes != dItemNo_WEAR_ZORA_e && currentClothes != dItemNo_ARMOR_e));
        fuku_g0->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                               eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 3 (Zora Armor)
    J2DPicture* fuku_g1 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_g_1')));
    if (fuku_g1) {
        bool eq = (currentClothes == dItemNo_WEAR_ZORA_e);
        fuku_g1->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                               eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }
    // Slot 4 (Magic Armor)
    J2DPicture* fuku_g2 = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_g_2')));
    if (fuku_g2) {
        bool eq = (currentClothes == dItemNo_ARMOR_e);
        fuku_g2->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                               eq ? JUtility::TColor(255, 255, 0, 255) : JUtility::TColor(107, 107, 107, 255));
    }

    // Keep all icon pictures in pristine, full brightness
    J2DPicture* io = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('fuku_io')));
    if (io) {
        io->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
        io->setAlpha(255);
    }
    J2DPicture* kim = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('ken_im')));
    if (kim) {
        kim->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
        kim->setAlpha(255);
    }
    J2DPicture* tim = static_cast<J2DPicture*>(screen->search(MULTI_CHAR('tate_im')));
    if (tim) {
        tim->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
        tim->setAlpha(255);
    }
}

void on_menu_collect_2d_create_post(ModContext*, void* args, void*, void*) {
    if (!args) return;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    s_currentCollect2D = collect2D;
    if (!g_configCollectionStarterEquip) return;
    apply_collect_shifts(collect2D);
}

HookAction on_menu_collect_2d_delete_pre(ModContext*, void*, void*, void*) {
    s_currentCollect2D = nullptr;
    s_paneKenMid = nullptr;
    s_paneTateMid = nullptr;
    s_paneFukuStart = nullptr;
    s_picKenMidFrame = nullptr;
    s_picKenMidIcon = nullptr;
    s_picTateMidFrame = nullptr;
    s_picTateMidIcon = nullptr;
    s_picFukuStartFrame = nullptr;
    s_picFukuStartIcon = nullptr;
    s_picTunagiKen2 = nullptr;
    s_picTunagiTate2 = nullptr;
    s_picTunagiFuku3 = nullptr;
    s_capturedScreen = nullptr;
    s_cachedScreen = nullptr;
    return HOOK_CONTINUE;
}

HookAction on_screen_set_pre(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return HOOK_CONTINUE;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (collect2D && collect2D->mpScreen) {
        update_screen_bases(collect2D->mpScreen, collect2D->mpHeap);
        apply_collect_shifts(collect2D);
    }
    return HOOK_CONTINUE;
}

void on_screen_set_post(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return;

    // Swords name/description string IDs
    collect2D->field_0x184[3][0] = 0x1a4; // Wooden Sword
    collect2D->field_0x1d8[3][0] = 0x2a4;
    collect2D->field_0x184[4][0] = 0x18d; // Ordon Sword
    collect2D->field_0x1d8[4][0] = 0x28d;
    collect2D->field_0x184[5][0] = 0x18e; // Master Sword
    collect2D->field_0x1d8[5][0] = 0x28e;
    collect2D->field_0x184[6][0] = 0x186; // Heart Container
    collect2D->field_0x1d8[6][0] = 0x286;

    // Shields name/description string IDs
    collect2D->field_0x184[3][1] = 0x18f; // Wooden Shield
    collect2D->field_0x1d8[3][1] = 0x28f;
    collect2D->field_0x184[4][1] = 0x190; // Ordon Shield
    collect2D->field_0x1d8[4][1] = 0x290;
    collect2D->field_0x184[5][1] = 0x191; // Hylian Shield
    collect2D->field_0x1d8[5][1] = 0x291;

    // Clothes name/description string IDs
    collect2D->field_0x184[3][2] = 0x193; // Ordon Clothes
    collect2D->field_0x1d8[3][2] = 0x293;
    collect2D->field_0x184[4][2] = 0x194; // Kokiri Clothes
    collect2D->field_0x1d8[4][2] = 0x294;
    collect2D->field_0x184[5][2] = 0x196; // Zora Armor
    collect2D->field_0x1d8[5][2] = 0x296;
    collect2D->field_0x184[6][2] = 0x195; // Magic Armor
    collect2D->field_0x1d8[6][2] = 0x295;

    apply_collect_shifts(collect2D);
    update_frame_highlights(collect2D);

    if (collect2D->mpScreen) {
        auto setupSelPm = [&](int x, int y, J2DPane* pane) {
            if (!pane) return;
            if (!collect2D->mpSelPm[x][y]) {
                JKRHeap* oldHeap = collect2D->mpHeap ? mDoExt_setCurrentHeap(collect2D->mpHeap) : nullptr;
                CPaneMgr* pm = JKR_NEW CPaneMgr();
                if (pm) {
                    pm->mFlags = 0;
                    pm->initiate(pane, (JKRExpHeap*)collect2D->mpHeap);
                    collect2D->mpSelPm[x][y] = pm;
                }
                if (oldHeap) mDoExt_setCurrentHeap(oldHeap);
            } else {
                collect2D->mpSelPm[x][y]->mPane = pane;
                collect2D->mpSelPm[x][y]->reinit();
            }
        };

        // Swords
        setupSelPm(3, 0, collect2D->mpScreen->search(MULTI_CHAR('ken_n0')));
        setupSelPm(4, 0, s_paneKenMid);
        setupSelPm(5, 0, collect2D->mpScreen->search(MULTI_CHAR('ken_n1')));
        setupSelPm(6, 0, collect2D->mpScreen->search(MULTI_CHAR('heart_n')));

        // Shields
        setupSelPm(3, 1, collect2D->mpScreen->search(MULTI_CHAR('tate_n0')));
        setupSelPm(4, 1, s_paneTateMid);
        setupSelPm(5, 1, collect2D->mpScreen->search(MULTI_CHAR('tate_n1')));
        collect2D->mpSelPm[6][1] = nullptr;

        // Clothes
        setupSelPm(3, 2, s_paneFukuStart);
        setupSelPm(4, 2, collect2D->mpScreen->search(MULTI_CHAR('fuku_n0')));
        setupSelPm(5, 2, collect2D->mpScreen->search(MULTI_CHAR('fuku_n1')));
        setupSelPm(6, 2, collect2D->mpScreen->search(MULTI_CHAR('fuku_n2')));
    }
}

void on_menu_collect_wide_post(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    apply_collect_shifts(collect2D);
    update_frame_highlights(collect2D);
}

void on_menu_collect_2d_move_post(ModContext*, void* args, void*, void*) {
    if (!g_configCollectionStarterEquip || !args) return;
    dMenu_Collect2D_c* collect2D = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!collect2D) return;
    apply_collect_shifts(collect2D);
    update_frame_highlights(collect2D);
}

void on_mw_execute_post(ModContext*, void*, void*, void*) {
    if (s_needReloadCollect) {
        s_needReloadCollect = false;
        dMw_c* mw = dMeter2Info_getMenuWindowClass();
        if (mw && s_currentCollect2D != nullptr && mw->isPauseWindow()) {
            mw->dMw_collect_delete(true);
            mw->dMw_collect_create();
        }
    }
}
