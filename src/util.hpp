#pragma once

#include "global.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "m_Do/m_Do_ext.h"
#include "SSystem/SComponent/c_xyz.h"
#include "SSystem/SComponent/c_sxyz.h"

bool loadObjectArchive(const char* arcName);
J3DModel* loadBmdFromArc(const char* arcName, const char* bmdName, cXyz scale = cXyz(1.0f, 1.0f, 1.0f));
mDoExt_bckAnm* loadBckFromArc(const char* arcName, const char* bckName, int playMode = 2, f32 rate = 1.0f);
void renderModelAt(J3DModel* model, const cXyz& pos, const csXyz& angle = csXyz(0, 0, 0), const cXyz& scale = cXyz(1.0f, 1.0f, 1.0f), mDoExt_bckAnm* bck = nullptr);
