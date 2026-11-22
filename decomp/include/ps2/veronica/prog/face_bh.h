#ifndef _FACE_BH_H_
#define _FACE_BH_H_

#include "types.h"
#include "macros.h"

void fmSetLipSyncParam(MASK_WORK* fm, PARAM_WORK* base, LIP_WORK* lip, unsigned int mask);
void _fmSetObjP(ML_WORK* mlwP);
NJS_CNK_OBJECT* _fmCnkSearchObject(MASK_WORK* fm, NJS_CNK_OBJECT* root, char id);
void _fmGetVChunkType(MASK_WORK* fm);
void _fmCnkSetFaceObject(MASK_WORK* fm, NJS_CNK_OBJECT* face);
void _fmCnkSetEyeballObject(MASK_WORK* fm, unsigned int id, NJS_CNK_OBJECT* eye, NJS_CNK_OBJECT* root);
void _fmCnkSetTangObject(MASK_WORK* fm, NJS_CNK_OBJECT* tang);
void _fmCnkSetToothObject(MASK_WORK* fm, NJS_CNK_OBJECT* tooth);
void _fmCnkSetJaw(MASK_WORK* fm, unsigned int v0, unsigned int v1);
void fmCnkInitContext(MASK_WORK* fm, void* data, NJS_CNK_OBJECT* root, ML_WORK* mlwP);
void fmCnkSetParam(MASK_WORK* fm, PARAM_WORK* param);
void fmCnkSetParamLip(MASK_WORK* fm, PARAM_WORK* param, unsigned int flag);
void fmCnkSetInterParam(MASK_WORK* fm, PARAM_WORK* p1, PARAM_WORK* p2, float t, float lv, unsigned int mask);
void fmCnkSetInterParamLip(MASK_WORK* fm, PARAM_WORK* p1, PARAM_WORK* p2, float t, float lv, unsigned int mask);
void fmCnkSetCurrentFrame(MASK_WORK* fm, float frame);
unsigned int fmCnkGetLastFrame(MASK_WORK* fm);
void _fmCnkCalcMuscle(MASK_WORK* fm);
void _fmCnkCalcJaw(MASK_WORK* fm);
void _fmCnkCalcEye(MASK_WORK* fm);
void _fmCnkCalcTang(MASK_WORK* fm);
void _fmCnkCalcNormal(MASK_WORK* fm);
void fmCnkCalcFace(MASK_WORK* fm);
void fmCnkSetMode(MASK_WORK* fm, unsigned int mode);

#endif
