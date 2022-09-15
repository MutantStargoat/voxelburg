/*
gbajam22 entry for the Gameboy Advance
Copyright (C) 2022  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef XGL_H_
#define XGL_H_

#include <stdint.h>

#define X_PI	0x3243f
#define X_2PI	0x6487f
#define X_HPI	0x19220
#define X_QPI	0xc910

enum {
	XGL_LIGHTING	= 1,
	XGL_DEPTH_TEST	= 2,
};

enum {
	XGL_LINES		= 2,
	XGL_TRIANGLES	= 3,
	XGL_QUADS		= 4
};

struct xvertex {
	int32_t x, y, z;
	int32_t nx, ny, nz;
	int32_t tx, ty;
	int32_t lit;
};

void xgl_init(void);

void xgl_enable(unsigned int opt);
void xgl_disable(unsigned int opt);

void xgl_viewport(int x, int y, int w, int h);

void xgl_push_matrix(void);
void xgl_pop_matrix(void);
void xgl_load_identity(void);
void xgl_load_matrix(const int32_t *m);
void xgl_get_matrix(int32_t *m);
void xgl_mult_matrix(const int32_t *m);

void xgl_translate(int32_t x, int32_t y, int32_t z);
void xgl_rotate_x(int32_t angle);
void xgl_rotate_y(int32_t angle);
void xgl_rotate_z(int32_t angle);
void xgl_scale(int32_t x, int32_t y, int32_t z);

void xgl_draw(int prim, const struct xvertex *varr, int vcount);
void xgl_transform(const struct xvertex *vin, int *x, int *y);

void xgl_index(int c);

void xgl_xyzzy(void);

int xgl_clip_near(struct xvertex *vout, int *voutnum, struct xvertex *vin, int vnum);

#endif	/* XGL_H_ */
