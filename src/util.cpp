#include "util.hpp"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_alink.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_mtx.h"
#include "d/d_kankyo.h"

#include <cstring>

static void normalizeArcName(const char* src, char* dst, size_t dstSize) {
    if (src == nullptr || dst == nullptr || dstSize == 0) return;
    std::strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
    char* dot = std::strstr(dst, ".arc");
    if (dot != nullptr) {
        *dot = '\0';
    }
}

bool loadObjectArchive(const char* arcName) {
    if (arcName == nullptr) return false;

    char cleanName[64];
    normalizeArcName(arcName, cleanName, sizeof(cleanName));

    // Check if already mounted
    dRes_info_c* info = dComIfG_getObjectResInfo(cleanName);
    if (info != nullptr && info->getArchive() != nullptr) {
        return true;
    }

    // Request mounting archive
    dComIfG_setObjectRes(cleanName, 0, nullptr);
    if (dComIfG_syncObjectRes(cleanName) == 0) {
        return true;
    }

    return false;
}

J3DModel* loadBmdFromArc(const char* arcName, const char* bmdName, cXyz scale) {
    if (arcName == nullptr || bmdName == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    // Load / sync archive
    if (!loadObjectArchive(cleanArc)) {
        return nullptr;
    }

    // Try finding resource by name
    void* res = dComIfG_getObjectRes(cleanArc, bmdName);

    // Try without .bmd extension
    if (res == nullptr) {
        char cleanBmd[64];
        std::strncpy(cleanBmd, bmdName, sizeof(cleanBmd) - 1);
        cleanBmd[sizeof(cleanBmd) - 1] = '\0';
        char* dot = std::strstr(cleanBmd, ".bmd");
        if (dot != nullptr) {
            *dot = '\0';
            res = dComIfG_getObjectRes(cleanArc, cleanBmd);
        }
    }

    // Try directly querying the JKRArchive
    if (res == nullptr) {
        dRes_info_c* info = dComIfG_getObjectResInfo(cleanArc);
        if (info != nullptr && info->getArchive() != nullptr) {
            res = info->getArchive()->getResource(bmdName);
            if (res == nullptr) {
                res = info->getArchive()->getResource('BMD ', bmdName);
            }
        }
    }

    if (res == nullptr) {
        return nullptr;
    }

    J3DModelData* modelData = static_cast<J3DModelData*>(res);
    if (modelData->getMaterialNum() == 0 || modelData->getShapeTable() == nullptr ||
        modelData->getShapeTable()->getShapeNum() == 0 ||
        modelData->getMaterialNodePointer(0) == nullptr) {
        return nullptr;
    }

    J3DModel* model = mDoExt_J3DModel__create(modelData, 0x80000, 0x11000084);
    if (model != nullptr) {
        model->setBaseScale(scale);
    }
    return model;
}

mDoExt_bckAnm* loadBckFromArc(const char* arcName, const char* bckName, int playMode, f32 rate) {
    if (arcName == nullptr || bckName == nullptr) return nullptr;

    char cleanArc[64];
    normalizeArcName(arcName, cleanArc, sizeof(cleanArc));

    if (!loadObjectArchive(cleanArc)) {
        return nullptr;
    }

    void* res = dComIfG_getObjectRes(cleanArc, bckName);
    if (res == nullptr) {
        char cleanBck[64];
        std::strncpy(cleanBck, bckName, sizeof(cleanBck) - 1);
        cleanBck[sizeof(cleanBck) - 1] = '\0';
        char* dot = std::strstr(cleanBck, ".bck");
        if (dot != nullptr) {
            *dot = '\0';
            res = dComIfG_getObjectRes(cleanArc, cleanBck);
        }
    }

    if (res == nullptr) {
        dRes_info_c* info = dComIfG_getObjectResInfo(cleanArc);
        if (info != nullptr && info->getArchive() != nullptr) {
            res = info->getArchive()->getResource(bckName);
            if (res == nullptr) {
                res = info->getArchive()->getResource('BCK ', bckName);
            }
        }
    }

    if (res == nullptr) {
        return nullptr;
    }

    J3DAnmTransform* pbck = static_cast<J3DAnmTransform*>(res);
    mDoExt_bckAnm* bckAnm = JKR_NEW mDoExt_bckAnm();
    if (bckAnm == nullptr) {
        return nullptr;
    }

    if (!bckAnm->init(pbck, TRUE, playMode, rate, 0, -1, false)) {
        JKR_DELETE(bckAnm);
        return nullptr;
    }

    return bckAnm;
}

void renderModelAt(J3DModel* model, const cXyz& pos, const csXyz& angle, const cXyz& scale, mDoExt_bckAnm* bck) {
    if (model == nullptr) return;

    if (bck != nullptr) {
        bck->play();
        bck->entry(model->getModelData());
    }

    mDoMtx_stack_c::transS(pos.x, pos.y, pos.z);
    mDoMtx_stack_c::ZXYrotM(angle.x, angle.y, angle.z);
    model->setBaseScale(scale);
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    model->calc();

    daAlink_c* alink = daAlink_getAlinkActorClass();
    if (alink != nullptr) {
        g_env_light.settingTevStruct_colget_player(&alink->tevStr);
        g_env_light.setLightTevColorType_MAJI(model, &alink->tevStr);
    }

    mDoExt_modelUpdateDL(model);
}
