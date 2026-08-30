#include "visible_equipment.hpp"

#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_horse.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_kankyo.h"
#include "f_pc/f_pc_name.h"
#include "f_pc/f_pc_profile_lst.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_mtx.h"
#include "mods/api.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.h"
#include "../z_button/z_button.hpp"
#include <cstdio>
#include <cstring>

static const LogService *s_logSvc = nullptr;
static ModContext *s_modCtx = nullptr;

bool g_configVisibleEquipmentEnabled = false;
int g_configVisibleEquipDisplayMode = 1;
bool g_configVisibleEquipMirrorBow = true;

bool g_configVisibleEquipShowBow = true;
bool g_configVisibleEquipShowLantern = true;
bool g_configVisibleEquipShowHorseCall = false;
bool g_configVisibleEquipQuiverOnBelt = false;

static J3DModel *s_customBowModel = nullptr;
static J3DModel *s_customQuiverModel = nullptr;
static int s_loadedQuiverType = 0;
static J3DModel *s_customHorseCallModel = nullptr;

DEFINE_HOOK(&daAlink_c::draw, AlinkDrawHook);

inline s16 degToS16(f32 deg) {
  return static_cast<s16>(deg * (65536.0f / 360.0f));
}

extern bool isNativeZButtonEngine();

static u32 getLinkShadowId(daAlink_c *alink) {
  if (alink == nullptr) {
    return 0;
  }

  if (alink->checkHorseRide()) {
    daHorse_c *horse = reinterpret_cast<daHorse_c *>(dComIfGp_getHorseActor());
    if (horse != nullptr) {
      return horse->getShadowID();
    }
  }

  const u8 *basePtr = reinterpret_cast<const u8 *>(&alink->field_0x31a4);

  return static_cast<u32>(alink->field_0x31a4);
}

static void addModelShadow(daAlink_c *alink, J3DModel *model) {
  if (alink != nullptr && model != nullptr) {
    u32 shadowId = getLinkShadowId(alink);
    if (shadowId != 0) {
      dComIfGd_addRealShadow(shadowId, model);
    }
  }
}

static bool isWolfOrTransforming(daAlink_c *alink) {
  if (alink == nullptr) {
    return true;
  }
  if (alink->checkWolf()) {
    return true;
  }
  if (alink->checkWolfShapeReverse()) {
    return true;
  }
  if (alink->checkMetamorphose()) {
    return true;
  }
  return false;
}

static MtxP getBoneMtx(daAlink_c *alink, const char *boneName) {
  if (alink == nullptr || alink->mpLinkModel == nullptr ||
      boneName == nullptr || isWolfOrTransforming(alink)) {
    return nullptr;
  }

  J3DModelData *modelData = alink->mpLinkModel->getModelData();
  if (modelData != nullptr) {
    JUTNameTab *jointNames = modelData->getJointName();
    if (jointNames != nullptr && jointNames->getResNameTable() != nullptr) {
      s16 idx = jointNames->getIndex(boneName);
      if (idx >= 0 && idx < modelData->getJointNum()) {
        return alink->mpLinkModel->getAnmMtx(static_cast<u16>(idx));
      }
    }
  }
  return nullptr;
}

static bool getBlendedBoneMtx(daAlink_c *alink, const char *boneName1,
                              const char *boneName2, f32 weight, Mtx outMtx) {
  MtxP mtx1 = getBoneMtx(alink, boneName1);
  MtxP mtx2 = getBoneMtx(alink, boneName2);

  if (mtx1 != nullptr && mtx2 != nullptr) {
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 4; c++) {
        outMtx[r][c] = (1.0f - weight) * mtx1[r][c] + weight * mtx2[r][c];
      }
    }
    return true;
  } else if (mtx1 != nullptr) {
    mDoMtx_copy(mtx1, outMtx);
    return true;
  } else if (mtx2 != nullptr) {
    mDoMtx_copy(mtx2, outMtx);
    return true;
  }
  return false;
}

static bool isBowUnlocked() {
  u8 bowItem = dComIfGs_getItem(SLOT_4, false);
  return (bowItem != dItemNo_NONE_e && bowItem != 0x00 && bowItem != 0xFF);
}

static bool isHorseCallUnlocked() {
  u8 item21 = dComIfGs_getItem(SLOT_21, false);
  u8 item22 = dComIfGs_getItem(SLOT_22, false);
  return (item21 == dItemNo_HORSE_FLUTE_e || item22 == dItemNo_HORSE_FLUTE_e ||
          dComIfGs_isItemFirstBit(dItemNo_HORSE_FLUTE_e));
}

static bool isLanternUnlocked() {
  u8 item = dComIfGs_getItem(SLOT_8, false);
  if (item == dItemNo_KANTERA_e)
    return true;
  return dComIfGs_isItemFirstBit(dItemNo_KANTERA_e);
}

static bool checkShouldShowBow() {
  if (!isBowUnlocked()) {
    return false;
  }

  if (g_configVisibleEquipDisplayMode == 1) {
    return true;
  }

  u8 slot0 = dComIfGp_getSelectItem(0);
  u8 slot1 = dComIfGp_getSelectItem(1);
  u8 zSlot = dComIfGs_getSelectItemIndex(2);
  u8 slot2 = (zSlot != 0xFF && zSlot != 0x00) ? dComIfGs_getItem(zSlot, false)
                                              : dComIfGp_getSelectItem(2);
  bool bowEquippedOnButton =
      (slot0 == dItemNo_BOW_e || slot1 == dItemNo_BOW_e ||
       slot2 == dItemNo_BOW_e || slot0 == 0x43 || slot1 == 0x43 ||
       slot2 == 0x43);
  return bowEquippedOnButton;
}

static bool checkShouldShowLantern() {
  if (!isLanternUnlocked()) {
    return false;
  }

  if (g_configVisibleEquipDisplayMode == 1) {
    return true;
  }

  u8 slot0 = dComIfGp_getSelectItem(0);
  u8 slot1 = dComIfGp_getSelectItem(1);
  u8 zSlot = dComIfGs_getSelectItemIndex(2);
  u8 slot2 = (zSlot != 0xFF && zSlot != 0x00) ? dComIfGs_getItem(zSlot, false)
                                              : dComIfGp_getSelectItem(2);
  return (slot0 == dItemNo_KANTERA_e || slot1 == dItemNo_KANTERA_e ||
          slot2 == dItemNo_KANTERA_e || slot0 == 0x48 || slot1 == 0x48 ||
          slot2 == 0x48);
}

static bool checkShouldShowHorseCall() {
  if (!isHorseCallUnlocked()) {
    return false;
  }

  if (g_configVisibleEquipDisplayMode == 1) {
    return true;
  }

  u8 slot0 = dComIfGp_getSelectItem(0);
  u8 slot1 = dComIfGp_getSelectItem(1);
  u8 zSlot = dComIfGs_getSelectItemIndex(2);
  u8 slot2 = (zSlot != 0xFF && zSlot != 0x00) ? dComIfGs_getItem(zSlot, false)
                                              : dComIfGp_getSelectItem(2);
  return (slot0 == dItemNo_HORSE_FLUTE_e || slot1 == dItemNo_HORSE_FLUTE_e ||
          slot2 == dItemNo_HORSE_FLUTE_e || slot0 == 0x4F || slot1 == 0x4F ||
          slot2 == 0x4F || slot0 == 0x84 || slot1 == 0x84 || slot2 == 0x84);
}

// Names each archive/index the first time we touch it, so a crash while probing
// a resource is attributable from the log (the frame itself shows up as an
// unresolved address inside the mod).
static void logModelLoadOnce(const char *arcName, s16 bmdIndex) {
  static const char *s_seen[8] = {};
  static int s_seenCount = 0;

  for (int i = 0; i < s_seenCount; i++) {
    if (s_seen[i] == arcName) {
      return;
    }
  }
  if (s_seenCount < 8) {
    s_seen[s_seenCount++] = arcName;
  }
  if (s_logSvc != nullptr && s_modCtx != nullptr) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "[VisibleEquip] probing model res '%s' idx 0x%04X", arcName,
                  static_cast<unsigned>(bmdIndex));
    s_logSvc->info(s_modCtx, buf);
  }
}

// Helper to instantiate BMD models
static J3DModel *loadBmdModel(J3DModel *&modelPtr, const char *arcName,
                              s16 bmdIndex, cXyz scale,
                              bool isObjectID = false) {
  if (modelPtr != nullptr) {
    return modelPtr;
  }

  if (arcName == nullptr) {
    return nullptr;
  }

  // Link's own archive is already resident and owned by the player actor.
  // Re-requesting it through setObjectRes hands back a resource whose model data
  // is not populated, so every later access -- including any attempt to validate
  // it -- dereferences garbage. Only load archives we don't already have.
  daAlink_c *alink = static_cast<daAlink_c *>(dComIfGp_getPlayer(0));
  const bool isLinkArc = alink != nullptr && alink->mArcName != nullptr &&
                         std::strcmp(arcName, alink->mArcName) == 0;
  if (!isLinkArc) {
    dComIfG_setObjectRes(arcName, 0, nullptr);
    if (dComIfG_syncObjectRes(arcName) != 0) {
      return nullptr;
    }
  }

  void *resPtr = isObjectID ? dComIfG_getObjectIDRes(arcName, bmdIndex)
                            : dComIfG_getObjectRes(arcName, bmdIndex);
  if (resPtr == nullptr) {
    return nullptr;
  }

  logModelLoadOnce(arcName, bmdIndex);

  // The resource can be present but not yet populated. mDoExt_J3DModel__create
  // walks the material table unconditionally, so an empty one crashes inside
  // J3DMaterialTable::getMaterialNodePointer. Check the counts first: they are
  // plain members, whereas getMaterialNodePointer() indexes a pointer array and
  // would fault itself on an empty table. getShapeTable() returns the address of
  // a member and is never null, so it cannot stand in for these checks.
  // Bailing here just means we retry on the next frame.
  J3DModelData *modelData = static_cast<J3DModelData *>(resPtr);
  if (modelData->getMaterialNum() == 0 || modelData->getShapeTable() == nullptr ||
      modelData->getShapeTable()->getShapeNum() == 0 ||
      modelData->getMaterialNodePointer(0) == nullptr) {
    return nullptr;
  }

  modelPtr = mDoExt_J3DModel__create(modelData, 0x80000, 0x11000084);
  if (modelPtr != nullptr) {
    modelPtr->setBaseScale(scale);
  }

  return modelPtr;
}

static void loadBowModel(const LogService *, ModContext *) {
  const char *bowArcName = dItem_data::getArcName(dItemNo_BOW_e);
  s16 bowBmdIdx = dItem_data::getBmdName(dItemNo_BOW_e);
  loadBmdModel(s_customBowModel, bowArcName, bowBmdIdx,
               cXyz(1.25f, 1.25f, 1.25f));
}

static void loadQuiverModel(const LogService *log_svc, ModContext *mod_ctx) {
  u8 arrowMax = dComIfGs_getArrowMax();
  int targetQuiverType = (arrowMax >= 100) ? 3 : ((arrowMax >= 60) ? 2 : 1);
  const char *quiverArcName =
      (arrowMax >= 100) ? "O_gD_quL3"
                        : ((arrowMax >= 60) ? "O_gD_quL2" : "O_gD_quL1");

  if (s_loadedQuiverType != targetQuiverType) {
    s_customQuiverModel = nullptr;
    s_loadedQuiverType = targetQuiverType;
  }

  loadBmdModel(s_customQuiverModel, quiverArcName, 3,
               cXyz(1.25f, 1.25f, 1.25f));
}

static void loadHorseCallModel(const LogService *, ModContext *) {
  if (s_customHorseCallModel == nullptr) {
    loadBmdModel(s_customHorseCallModel, "Demo37_02", 0x0042,
                 cXyz(1.0f, 1.0f, 1.0f), true);
    if (s_customHorseCallModel != nullptr) {
      J3DModelData *mData = s_customHorseCallModel->getModelData();
      if (mData != nullptr && mData->getShapeTable() != nullptr) {
        u16 numShapes = mData->getShapeTable()->getShapeNum();
        if (numShapes > 0 && mData->getShapeNodePointer(0) != nullptr) {
          mData->getShapeNodePointer(0)->hide();
        }
        if (numShapes > 1 && mData->getShapeNodePointer(1) != nullptr) {
          mData->getShapeNodePointer(1)->show();
        }
      }
    }
  }
}

static void renderLantern(daAlink_c *alink) {
  if (alink == nullptr || isWolfOrTransforming(alink) ||
      alink->mpKanteraModel == nullptr) {
    return;
  }

  // Skip if active lantern is out (vanilla game handles model, light, and physics)
  if (alink->checkNoResetFlg2(static_cast<daPy_py_c::daPy_FLG2>(0x1)) ||
        alink->checkNoResetFlg2(static_cast<daPy_py_c::daPy_FLG2>(0x20000)) ||
        alink->mEquipItem == dItemNo_KANTERA_e) {
      return;
  }

  if (alink->mpLinkModel == nullptr ||
      alink->getClothesChangeWaitTimer() != 0) {
    return;
  }

  J3DModelData *modelData = alink->mpLinkModel->getModelData();
  MtxP beltMtx = (modelData != nullptr && modelData->getJointNum() > 0x10)
                     ? alink->mpLinkModel->getAnmMtx(0x10)
                     : getBoneMtx(alink, "waist");
  if (beltMtx != nullptr) {
    mDoMtx_stack_c::copy(beltMtx);
    mDoMtx_stack_c::transM(-1.0f, 4.5f, 9.0f);
    mDoMtx_stack_c::XYZrotM(cM_deg2s(-75.0f), cM_deg2s(62.0f), cM_deg2s(89.0f));
    alink->mpKanteraModel->setBaseTRMtx(mDoMtx_stack_c::get());

    cXyz &flamePos = alink->mKandelaarFlamePos;
    if (flamePos.abs2() < 1.0f) {
      mDoMtx_multVecZero(mDoMtx_stack_c::get(), &flamePos);
      flamePos.y -= 17.0f;
    }

    alink->mpKanteraModel->calc();

    g_env_light.settingTevStruct_colget_player(&alink->tevStr);
    g_env_light.setLightTevColorType_MAJI(alink->mpKanteraModel, &alink->tevStr);
    mDoExt_modelUpdateDL(alink->mpKanteraModel);
    addModelShadow(alink, alink->mpKanteraModel);
  }
}

static void renderBow(daAlink_c *alink, bool shouldShowEquipment,
                      bool isBowInHand) {
  if (!shouldShowEquipment || isBowInHand || s_customBowModel == nullptr) {
    return;
  }

  MtxP bowMtx = alink->mSheathModel->getBaseTRMtx();

  if (bowMtx != nullptr) {
    mDoMtx_stack_c::copy(bowMtx);
    if (g_configVisibleEquipMirrorBow) {
      mDoMtx_stack_c::transM(35.0f, 5.0f, -15.0f);
      mDoMtx_stack_c::XYZrotM(degToS16(90.0f), degToS16(-68.0f),
                              degToS16(0.0f));
    } else {
      mDoMtx_stack_c::transM(10.0f, 0.0f, -15.0f);
      mDoMtx_stack_c::XYZrotM(degToS16(82.0f), degToS16(50.0f), degToS16(0.0f));
    }

    cXyz scale(1.25f, 1.25f, 1.25f);
    s_customBowModel->setBaseScale(scale);
    s_customBowModel->setBaseTRMtx(mDoMtx_stack_c::get());
    s_customBowModel->calc();

    g_env_light.settingTevStruct_colget_player(&alink->tevStr);
    g_env_light.setLightTevColorType_MAJI(s_customBowModel, &alink->tevStr);
    mDoExt_modelUpdateDL(s_customBowModel);
    addModelShadow(alink, s_customBowModel);
  }
}

static void renderQuiver(daAlink_c* alink, bool shouldShowEquipment) {
  if (!shouldShowEquipment || s_customQuiverModel == nullptr) {
    return;
  }

  static bool s_loggedLinkJointsOnce = false;
  if (!s_loggedLinkJointsOnce && alink != nullptr &&
      alink->mpLinkModel != nullptr && s_logSvc != nullptr &&
      s_modCtx != nullptr) {
    J3DModelData *modelData = alink->mpLinkModel->getModelData();
    if (modelData != nullptr) {
      s_loggedLinkJointsOnce = true;
    }
      }

  MtxP quiverMtx = getBoneMtx(alink, "waist");

  if (quiverMtx != nullptr) {
    mDoMtx_stack_c::copy(quiverMtx);

    if (g_configVisibleEquipQuiverOnBelt) {
      if (s_loadedQuiverType == 3) {
        // Level 3 Giant Quiver (100 Arrows) - Left belt
        mDoMtx_stack_c::transM(25.0f, 5.0f, 23.0f);
        mDoMtx_stack_c::XYZrotM(degToS16(0.0f), degToS16(120.0f), degToS16(135.0f));

        cXyz scale(0.73f, 0.73f, 0.73f);
        s_customQuiverModel->setBaseScale(scale);
      } else if (s_loadedQuiverType == 2) {
        // Level 2 Big Quiver (60 Arrows) - Left belt
        mDoMtx_stack_c::transM(25.0f, 5.0f, 23.0f);
        mDoMtx_stack_c::XYZrotM(degToS16(0.0f), degToS16(-15.0f), degToS16(135.0f));

        cXyz scale(0.73f, 0.73f, 0.73f);
        s_customQuiverModel->setBaseScale(scale);
      } else {
        // Level 1 Standard Quiver (30 Arrows) - Left belt
        mDoMtx_stack_c::transM(25.0f, 5.0f, 24.0f);
        mDoMtx_stack_c::XYZrotM(degToS16(45.0f), degToS16(-95.0f), degToS16(90.0f));

        cXyz scale(0.73f, 0.73f, 0.73f);
        s_customQuiverModel->setBaseScale(scale);
      }
    } else {
      if (s_loadedQuiverType == 3) {
        // Level 3 Giant Quiver (100 Arrows) - Back waist
        mDoMtx_stack_c::transM(25.0f, 15.0f, 5.0f);
        mDoMtx_stack_c::XYZrotM(degToS16(-70.0f), degToS16(0.0f), degToS16(90.0f));

        cXyz scale(1.1f, 1.1f, 1.1f);
        s_customQuiverModel->setBaseScale(scale);
      } else if (s_loadedQuiverType == 2) {
        // Level 2 Big Quiver (60 Arrows) - Back waist
        mDoMtx_stack_c::transM(25.0f, 15.0f, 5.0f);
        mDoMtx_stack_c::XYZrotM(degToS16(-90.0f), degToS16(30.0f), degToS16(0.0f));

        cXyz scale(1.1f, 1.1f, 1.1f);
        s_customQuiverModel->setBaseScale(scale);
      } else {
        // Level 1 Standard Quiver (30 Arrows) - Back waist
        mDoMtx_stack_c::transM(25.0f, 15.0f, 5.0f);
        mDoMtx_stack_c::XYZrotM(degToS16(-90.0f), degToS16(30.0f), degToS16(0.0f));

        cXyz scale(1.0f, 1.0f, 1.0f);
        s_customQuiverModel->setBaseScale(scale);
      }
    }

    s_customQuiverModel->setBaseTRMtx(mDoMtx_stack_c::get());
    s_customQuiverModel->calc();

    g_env_light.settingTevStruct_colget_player(&alink->tevStr);
    g_env_light.setLightTevColorType_MAJI(s_customQuiverModel, &alink->tevStr);
    mDoExt_modelUpdateDL(s_customQuiverModel);
    addModelShadow(alink, s_customQuiverModel);
  }
}

static void renderHorseCall(daAlink_c *alink) {
  if (s_customHorseCallModel == nullptr) {
    return;
  }

  Mtx blendedMtx;
  bool hasMtx = getBlendedBoneMtx(alink, "backbone1", "neck", 0.5f, blendedMtx);

  if (hasMtx) {
    mDoMtx_stack_c::copy(blendedMtx);

    mDoMtx_stack_c::transM(17.0f, 18.1f, 0.0f);
    mDoMtx_stack_c::XYZrotM(degToS16(0.0f), degToS16(90.0f), degToS16(-180.0f));

    cXyz scale(0.6f, 0.6f, 0.6f);
    s_customHorseCallModel->setBaseScale(scale);
    s_customHorseCallModel->setBaseTRMtx(mDoMtx_stack_c::get());
    s_customHorseCallModel->calc();

    g_env_light.settingTevStruct_colget_player(&alink->tevStr);
    g_env_light.setLightTevColorType_MAJI(s_customHorseCallModel,
                                          &alink->tevStr);
    mDoExt_modelUpdateDL(s_customHorseCallModel);
    addModelShadow(alink, s_customHorseCallModel);
  }
}

static const char *s_cachedArcName = nullptr;
static void *s_cachedLinkInstance = nullptr;
static J3DModel *s_cachedLinkModel = nullptr;
static char s_cachedStageName[16] = {0};
static s32 s_cachedRoomNo = -1;

static void invalidateEquipmentModels() {
  s_customBowModel = nullptr;
  s_customQuiverModel = nullptr;
  s_loadedQuiverType = 0;
  s_customHorseCallModel = nullptr;
}

// Returns false while Link's archive is being torn down or reloaded, in which
// case nothing in the cache may be touched this frame.
static bool syncEquipmentModelCache(daAlink_c *alink) {
  if (alink == nullptr || alink->getClothesChangeWaitTimer() != 0) {
    invalidateEquipmentModels();
    s_cachedLinkInstance = nullptr;
    s_cachedLinkModel = nullptr;
    s_cachedArcName = nullptr;
    s_cachedStageName[0] = '\0';
    s_cachedRoomNo = -1;
    return false;
  }

  const char *stageName = dComIfGp_getStartStageName();
  int roomNo = fopAcM_GetRoomNo(alink);

  if (s_cachedLinkInstance != alink ||
      s_cachedLinkModel != alink->mpLinkModel ||
      s_cachedArcName != alink->mArcName ||
      (stageName != nullptr && std::strcmp(s_cachedStageName, stageName) != 0) ||
      s_cachedRoomNo != roomNo) {
    invalidateEquipmentModels();
    s_cachedLinkInstance = alink;
    s_cachedLinkModel = alink->mpLinkModel;
    s_cachedArcName = alink->mArcName;
    if (stageName != nullptr) {
      std::strncpy(s_cachedStageName, stageName, sizeof(s_cachedStageName) - 1);
      s_cachedStageName[sizeof(s_cachedStageName) - 1] = '\0';
    } else {
      s_cachedStageName[0] = '\0';
    }
    s_cachedRoomNo = roomNo;
  }

  return true;
}

static bool isTitleOrMainMenu() {
  const char *stageName = dComIfGp_getStartStageName();
  if (stageName != nullptr) {
    if (std::strcmp(stageName, "F_SP102") == 0 ||
        std::strcmp(stageName, "title") == 0) {
      return true;
    }
  }
  return false;
}

static void on_alink_draw_post(ModContext *, void *, void *, void *) {
  if (!g_configVisibleEquipmentEnabled || isTitleOrMainMenu()) {
    return;
  }

  daAlink_c *alink = static_cast<daAlink_c *>(dComIfGp_getPlayer(0));
  if (!alink || !alink->mpLinkModel || alink->mpLinkModel->getModelData() == nullptr ||
      isWolfOrTransforming(alink)) {
    return;
  }

  if (!syncEquipmentModelCache(alink)) {
    return;
  }

  bool shouldShowBow = g_configVisibleEquipShowBow && checkShouldShowBow();
  bool isBowInHand = alink->checkBowAndSlingItem(alink->mEquipItem) ||
                   (alink->mEquipItem == dItemNo_BOW_e);

  if (shouldShowBow) {
    renderBow(alink, shouldShowBow, isBowInHand);
    renderQuiver(alink, shouldShowBow);
  }
  if (g_configVisibleEquipShowHorseCall && checkShouldShowHorseCall()) {
    renderHorseCall(alink);
  }
  if (g_configVisibleEquipShowLantern && checkShouldShowLantern()) {
    renderLantern(alink);
  }
}

ModResult init_visible_equipment(const HookService *hook_svc, ModError *) {
  invalidateEquipmentModels();
  s_cachedArcName = nullptr;

  if (hook_svc) {
    mods::hook::add_post<AlinkDrawHook>(hook_svc, on_alink_draw_post);
  }
  return MOD_OK;
}

void update_visible_equipment(const LogService *log_svc, ModContext *mod_ctx) {
  s_logSvc = log_svc;
  s_modCtx = mod_ctx;

  if (!g_configVisibleEquipmentEnabled || isTitleOrMainMenu()) {
    return;
  }

  daAlink_c *alink = static_cast<daAlink_c *>(dComIfGp_getPlayer(0));

  if (!syncEquipmentModelCache(alink)) {
    return;
  }

  if (!alink->mpLinkModel || isWolfOrTransforming(alink)) {
    return;
  }

  if (g_configVisibleEquipShowBow && checkShouldShowBow()) {
    loadBowModel(log_svc, mod_ctx);
    loadQuiverModel(log_svc, mod_ctx);
  }

  if (g_configVisibleEquipShowHorseCall && checkShouldShowHorseCall()) {
    loadHorseCallModel(log_svc, mod_ctx);
  }
}

void draw_visible_equipment(const LogService *, ModContext *) {}

void shutdown_visible_equipment() {
  invalidateEquipmentModels();
  s_cachedArcName = nullptr;
}