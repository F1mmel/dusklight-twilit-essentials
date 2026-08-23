#pragma once

#include "midna_location.hpp"

void update_midna_pane(dMeter2Draw_c* draw) {
    if (draw == nullptr) return;
    J2DScreen* screen = draw->getMainScreenPtr();
    if (screen == nullptr) return;

    J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
    if (midnaPane == nullptr) return;

    if (is_pause_menu_open(draw) || !isMidnaUnlocked()) {
        midnaPane->hide();
    } else {
        J2DPane* juji = screen->search(MULTI_CHAR('juji_n'));
        if (midnaPane != nullptr && juji != nullptr) {
            if (midnaPane->getParentPane() != juji) {
                J2DPane* oldParent = midnaPane->getParentPane();
                if (oldParent != nullptr) {
                    oldParent->mPaneTree.removeChild(&midnaPane->mPaneTree);
                }
                juji->appendChild(midnaPane);
                set_pane_influenced_alpha_recursive(midnaPane, true);
            }
            midnaPane->move(-18.0f, 0.0f);
            midnaPane->show();
        }
    }
}

void reset_midna_pane() {
    if (isTitleOrMainMenu()) return;

    dMeter2Draw_c* draw = nullptr;
    if (g_meter2_info.getMeterClass() != nullptr) {
        draw = g_meter2_info.getMeterClass()->getMeterDrawPtr();
    }
    if (draw != nullptr && draw->getMainScreenPtr() != nullptr) {
        J2DScreen* screen = draw->getMainScreenPtr();
        if (screen != nullptr) {
            J2DPane* midnaPane = screen->search(MULTI_CHAR('midona_n'));
            J2DPane* contPane = screen->search(MULTI_CHAR('cont_n'));
            if (midnaPane != nullptr) {
                if (contPane != nullptr && midnaPane->getParentPane() != nullptr && midnaPane->getParentPane() != contPane) {
                    J2DPane* oldParent = midnaPane->getParentPane();
                    if (oldParent != nullptr) {
                        oldParent->mPaneTree.removeChild(&midnaPane->mPaneTree);
                    }
                    contPane->appendChild(midnaPane);
                }
                midnaPane->translate(-88.0f, -24.5f);
                midnaPane->show();
            }
        }
    }
}
