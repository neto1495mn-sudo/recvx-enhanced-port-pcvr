/* libvu0 helper functions (the SDK implements them with VU0 macro code). */
#include "ps2_platform.h"
#include <math.h>

void sceVu0UnitMatrix(sceVu0FMATRIX m)
{
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) m[i][j] = i == j ? 1.0f : 0.0f;
}
void sceVu0AddVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)  { for (int i = 0; i < 4; i++) v0[i] = v1[i] + v2[i]; }
void sceVu0SubVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)  { for (int i = 0; i < 4; i++) v0[i] = v1[i] - v2[i]; }
void sceVu0ScaleVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s)         { for (int i = 0; i < 4; i++) v0[i] = v1[i] * s; }
float sceVu0InnerProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1)                { return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2]; }
void sceVu0OuterProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)
{
    float x = v1[1] * v2[2] - v1[2] * v2[1];
    float y = v1[2] * v2[0] - v1[0] * v2[2];
    float z = v1[0] * v2[1] - v1[1] * v2[0];
    v0[0] = x; v0[1] = y; v0[2] = z;
}
void sceVu0Normalize(sceVu0FVECTOR v0, sceVu0FVECTOR v1)
{
    float l = sqrtf(v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2]);
    float r = l != 0.0f ? 1.0f / l : 0.0f;
    v0[0] = v1[0] * r; v0[1] = v1[1] * r; v0[2] = v1[2] * r; v0[3] = v1[3];
}
void sceVu0FTOI0Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1) { for (int i = 0; i < 4; i++) v0[i] = (int)v1[i]; }
void sceVu0FTOI4Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1) { for (int i = 0; i < 4; i++) v0[i] = (int)(v1[i] * 16.0f); }
void sceVu0ITOF4Vector(sceVu0FVECTOR v0, sceVu0IVECTOR v1) { for (int i = 0; i < 4; i++) v0[i] = (float)v1[i] / 16.0f; }
void sceVpu0Reset(void) {}
