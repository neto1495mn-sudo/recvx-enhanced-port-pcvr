/* The leather ammo pouch (tools/make_pouch.py, pouch_data.c): the ammo box at the holster in VR (gs_gl.c draw_pouch). */
#ifndef RECVX_POUCH_DATA_H
#define RECVX_POUCH_DATA_H
#include <stdint.h>
typedef struct { int16_t x, y, z; int8_t nx, ny, nz, part; int16_t u, v; } PouchVertex;   /* part 1: the flap */
extern const int pouch_nverts, pouch_nidx;
extern const float pouch_size[3];
extern const float pouch_hinge[2];   /* y, z of the flap's hinge (x axis) */
extern const PouchVertex pouch_verts[];
extern const uint16_t pouch_idx[];
extern const unsigned pouch_jpg_len;
extern const unsigned char pouch_jpg[];
#endif
