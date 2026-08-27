#include "collection_common.hpp"

ModContext* g_modCtx = nullptr;
const LogService* g_logSvc = nullptr;

void log_collect_info(const char* fmt, ...) {
    if (!g_logSvc || !g_modCtx) return;
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    g_logSvc->info(g_modCtx, buffer);
}

J2DScreen* s_cachedScreen = nullptr;
J2DScreen* s_capturedScreen = nullptr;
dMenu_Collect2D_c* s_currentCollect2D = nullptr;
bool s_needReloadCollect = false;

// Custom Panes
// Swords: Slot 1 is ken_n0, Slot 2 is s_paneKenMid, Slot 3 is ken_n1
J2DPane* s_paneKenMid = nullptr;
J2DPicture* s_picKenMidFrame = nullptr;
J2DPicture* s_picKenMidIcon = nullptr;

// Shields: Slot 1 is tate_n0, Slot 2 is s_paneTateMid, Slot 3 is tate_n1
J2DPane* s_paneTateMid = nullptr;
J2DPicture* s_picTateMidFrame = nullptr;
J2DPicture* s_picTateMidIcon = nullptr;

// Clothes: Slot 1 is s_paneFukuStart, Slot 2 is fuku_n0, Slot 3 is fuku_n1, Slot 4 is fuku_n2
J2DPane* s_paneFukuStart = nullptr;
J2DPicture* s_picFukuStartFrame = nullptr;
J2DPicture* s_picFukuStartIcon = nullptr;

// Connectors (tunagi)
J2DPicture* s_picTunagiKen2 = nullptr;
J2DPicture* s_picTunagiTate2 = nullptr;
J2DPicture* s_picTunagiFuku3 = nullptr;

ResourceBuffer s_ordonClothesBtiBuf = RESOURCE_BUFFER_INIT;

ResTIMG* get_ordon_clothes_texture() {
    static ResTIMG* s_permanentOrdonClothesTex = nullptr;
    if (s_permanentOrdonClothesTex != nullptr) {
        return s_permanentOrdonClothesTex;
    }

    if (s_ordonClothesBtiBuf.data == nullptr) {
        const ResourceService* res_svc = get_resource_service();
        if (res_svc != nullptr && g_modCtx != nullptr) {
            res_svc->load(g_modCtx, "textures/ordon_clothes.bti", &s_ordonClothesBtiBuf);
        }
    }

    if (s_ordonClothesBtiBuf.data != nullptr) {
        JKRHeap* gameHeap = mDoExt_getGameHeap();
        if (gameHeap != nullptr && s_ordonClothesBtiBuf.size > 0) {
            void* persistentBuf = gameHeap->alloc(s_ordonClothesBtiBuf.size, 32);
            if (persistentBuf != nullptr) {
                memcpy(persistentBuf, s_ordonClothesBtiBuf.data, s_ordonClothesBtiBuf.size);
                ResTIMG* mainBuf = reinterpret_cast<ResTIMG*>(persistentBuf);
                mainBuf->alphaEnabled = 1;
                s_permanentOrdonClothesTex = mainBuf;
                return mainBuf;
            }
        }
        ResTIMG* mainBuf = reinterpret_cast<ResTIMG*>(s_ordonClothesBtiBuf.data);
        mainBuf->alphaEnabled = 1;
        s_permanentOrdonClothesTex = mainBuf;
        return mainBuf;
    }
    return nullptr;
}

const ResTIMG* safe_get_tex_info(J2DPane* pane) {
    if (!pane) return nullptr;
    if (pane->getTypeID() == 18 || pane->getTypeID() == 19 || pane->getKind() == 'PIC1' || pane->getKind() == 'PIC2') {
        J2DPicture* pic = reinterpret_cast<J2DPicture*>(pane);
        JUTTexture* tex = pic->getTexture(0);
        if (tex && tex->getTexInfo()) return tex->getTexInfo();
    }
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr; child = child->getNextChildPane()) {
        if (child->getTypeID() == 18 || child->getTypeID() == 19 || child->getKind() == 'PIC1' || child->getKind() == 'PIC2') {
            J2DPicture* pic = reinterpret_cast<J2DPicture*>(child);
            JUTTexture* tex = pic->getTexture(0);
            if (tex && tex->getTexInfo()) return tex->getTexInfo();
        }
    }
    return nullptr;
}

void set_pane_pos(J2DPane* pane, f32 x, f32 y) {
    if (!pane) return;
    pane->translate(x, y);
}

Vec get_pane_center(J2DPane* pane) {
    Vec center = {0.0f, 0.0f, 0.0f};
    if (!pane) return center;

    J2DPane* chain[32];
    int depth = 0;
    for (J2DPane* curr = pane; curr != nullptr && depth < 32; curr = curr->getParentPane()) {
        chain[depth++] = curr;
    }

    Mtx curMtx;
    MTXIdentity(curMtx);
    for (int i = depth - 1; i >= 0; i--) {
        chain[i]->calcMtx();
        Mtx localMtx;
        MTXCopy(*chain[i]->getMtx(), localMtx);
        Mtx next;
        MTXConcat(curMtx, localMtx, next);
        MTXCopy(next, curMtx);
    }

    f32 offsetX = (pane->mBounds.i.x + pane->mBounds.f.x) * 0.5f;
    f32 offsetY = (pane->mBounds.i.y + pane->mBounds.f.y) * 0.5f;

    center.x = curMtx[0][3] + (offsetX * curMtx[0][0] + offsetY * curMtx[0][1]);
    center.y = curMtx[1][3] + (offsetX * curMtx[1][0] + offsetY * curMtx[1][1]);
    center.z = curMtx[2][3];
    return center;
}

void safe_delete_custom_pane(J2DPane*& pane) {
    if (!pane) return;
    if (pane->getParentPane()) {
        pane->getParentPane()->mPaneTree.removeChild(&pane->mPaneTree);
    }
    while (pane->mPaneTree.getFirstChild() != nullptr) {
        JSUTree<J2DPane>* childTree = pane->mPaneTree.getFirstChild();
        J2DPane* child = childTree->getObject();
        pane->mPaneTree.removeChild(childTree);
        delete child;
    }
    delete pane;
    pane = nullptr;
}
