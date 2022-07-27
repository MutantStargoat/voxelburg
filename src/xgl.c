/*
blender for the Gameboy Advance
Copyright (C) 2021  John Tsiombikas <nuclear@member.fsf.org>

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
#include <string.h>
#include <math.h>
#include "xgl.h"
#include "polyfill.h"
#include "debug.h"

#define MAT_STACK_SIZE	4

static int vp[4];
static int32_t mat[MAT_STACK_SIZE][16];
static int mtop;
static unsigned int opt;
static int32_t ldir[3];

static void draw_ptlines(int prim, const struct xvertex *varr, int vcount);


void xgl_init(void)
{
	xgl_viewport(0, 0, 240, 160);
	xgl_load_identity();

	ldir[0] = ldir[1] = 0;
	ldir[2] = -0x100;
}

void xgl_enable(unsigned int o)
{
	opt |= o;
}

void xgl_disable(unsigned int o)
{
	opt &= ~o;
}

void xgl_viewport(int x, int y, int w, int h)
{
	vp[0] = x;
	vp[1] = y;
	vp[2] = w;
	vp[3] = h;
}

void xgl_push_matrix(void)
{
	int prev;

	if(mtop >= MAT_STACK_SIZE - 1) return;

	prev = mtop++;
	memcpy(mat[mtop], mat[prev], sizeof mat[0]);
}

void xgl_pop_matrix(void)
{
	if(mtop > 0) mtop--;
}

static int32_t id[] = {
	0x10000, 0, 0, 0,
	0, 0x10000, 0, 0,
	0, 0, 0x10000, 0,
	0, 0, 0, 0x10000
};

void xgl_load_identity(void)
{
	memcpy(mat[mtop], id, sizeof mat[0]);
}

void xgl_load_matrix(const int32_t *m)
{
	memcpy(mat[mtop], m, sizeof mat[0]);
}

void xgl_get_matrix(int32_t *m)
{
	memcpy(m, mat[mtop], sizeof mat[0]);
}

#define M(i,j)	(((i) << 2) + (j))
#define XMUL(a, b)	(((a) >> 8) * ((b) >> 8))
void xgl_mult_matrix(const int32_t *m2)
{
	int i, j;
	int32_t m1[16];
	int32_t *dest = mat[mtop];

	memcpy(m1, dest, sizeof m1);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			*dest++ = XMUL(m1[M(0, j)], m2[M(i, 0)]) +
				XMUL(m1[M(1, j)], m2[M(i, 1)]) +
				XMUL(m1[M(2, j)], m2[M(i, 2)]) +
				XMUL(m1[M(3, j)], m2[M(i, 3)]);
		}
	}
}

#define XSIN(x)		(int32_t)(sin(x / 65536.0f) * 65536.0f)
#define XCOS(x)		(int32_t)(cos(x / 65536.0f) * 65536.0f)

void xgl_translate(int32_t x, int32_t y, int32_t z)
{
	int32_t m[16] = {0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000};
	m[12] = x;
	m[13] = y;
	m[14] = z;
	xgl_mult_matrix(m);
}

void xgl_rotate_x(int32_t angle)
{
	int32_t m[16] = {0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000};
	int32_t sa = XSIN(angle);
	int32_t ca = XCOS(angle);
	m[5] = ca;
	m[6] = sa;
	m[9] = -sa;
	m[10] = ca;
	xgl_mult_matrix(m);
}

void xgl_rotate_y(int32_t angle)
{
	int32_t m[16] = {0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000};
	int32_t sa = XSIN(angle);
	int32_t ca = XCOS(angle);
	m[0] = ca;
	m[2] = -sa;
	m[8] = sa;
	m[10] = ca;
	xgl_mult_matrix(m);
}

void xgl_rotate_z(int32_t angle)
{
	int32_t m[16] = {0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000, 0, 0, 0, 0, 0x10000};
	int32_t sa = XSIN(angle);
	int32_t ca = XCOS(angle);
	m[0] = ca;
	m[1] = sa;
	m[4] = -sa;
	m[5] = ca;
	xgl_mult_matrix(m);
}

void xgl_scale(int32_t x, int32_t y, int32_t z)
{
	int32_t m[16] = {0};
	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 0x10000;
	xgl_mult_matrix(m);
}

static void xform(struct xvertex *out, const struct xvertex *in, const int32_t *m)
{
	out->x = XMUL(m[0], in->x) + XMUL(m[4], in->y) + XMUL(m[8], in->z) + m[12];
	out->y = XMUL(m[1], in->x) + XMUL(m[5], in->y) + XMUL(m[9], in->z) + m[13];
	out->z = XMUL(m[2], in->x) + XMUL(m[6], in->y) + XMUL(m[10], in->z) + m[14];
}

static void xform_norm(struct xvertex *out, const struct xvertex *in, const int32_t *m)
{
	out->nx = XMUL(m[0], in->nx) + XMUL(m[4], in->ny) + XMUL(m[8], in->nz);
	out->ny = XMUL(m[1], in->nx) + XMUL(m[5], in->ny) + XMUL(m[9], in->nz);
	out->nz = XMUL(m[2], in->nx) + XMUL(m[6], in->ny) + XMUL(m[10], in->nz);
}

/* d = 1.0 / tan(fov/2) */
#define PROJ_D	0x20000

void xgl_draw(int prim, const struct xvertex *varr, int vcount)
{
	int i, cidx;
	struct xvertex xv[4];
	struct pvertex pv[4];
	int32_t ndotl;

	if(prim < 3) {
		draw_ptlines(prim, varr, vcount);
		return;
	}

	while(vcount >= prim) {
		cidx = varr->cidx;

		xform(xv, varr, mat[mtop]);
		xform_norm(xv, varr, mat[mtop]);

		if(xv->nz > 0) {
			/* backface */
			varr += prim;
			vcount -= prim;
			continue;
		}

		if(opt & XGL_LIGHTING) {
			ndotl = (xv->nx >> 8) * ldir[0] + (xv->ny >> 8) * ldir[1] + (xv->nz >> 8) * ldir[2];
			if(ndotl < 0) ndotl = 0;
			cidx = 128 + (ndotl >> 9);
			if(cidx > 255) cidx = 255;
		}

		xv->x = (xv->x << 1) / (xv->z >> 8);	/* assume aspect: ~2 */
		xv->y = (xv->y << 2) / (xv->z >> 8);	/* the shift is * PROJ_D */
		/* projection result is 24.8 */
		/* viewport */
		pv->x = (((xv->x + 0x100) >> 1) * vp[2]) + (vp[0] << 8);
		pv->y = (((0x100 - xv->y) >> 1) * vp[3]) + (vp[1] << 8);
		varr++;

		for(i=1; i<prim; i++) {
			xform(xv + i, varr, mat[mtop]);

			xv[i].x = (xv[i].x << 1) / (xv[i].z >> 8);	/* assume aspect: ~2 */
			xv[i].y = (xv[i].y << 2) / (xv[i].z >> 8);	/* the shift is * PROJ_D */
			/* projection result is 24.8 */
			/* viewport */
			pv[i].x = (((xv[i].x + 0x100) >> 1) * vp[2]) + (vp[0] << 8);
			pv[i].y = (((0x100 - xv[i].y) >> 1) * vp[3]) + (vp[1] << 8);
			varr++;
		}
		vcount -= prim;

		polyfill_flat(pv, prim, cidx);
	}
}

void xgl_transform(const struct xvertex *vin, int *x, int *y)
{
	struct xvertex v;
	xform(&v, vin, mat[mtop]);

	v.x = (v.x << 1) / (v.z >> 8);	/* assume aspect: ~2 */
	v.y = (v.y << 2) / (v.z >> 8);	/* the shift is * PROJ_D */
	/* projection result is 24.8 */
	/* viewport */
	*x = ((((v.x + 0x100) >> 1) * vp[2]) >> 8) + vp[0];
	*y = ((((0x100 - v.y) >> 1) * vp[3]) >> 8) + vp[1];
}

static void draw_ptlines(int prim, const struct xvertex *varr, int vcount)
{
	int i;
	struct xvertex xv[2];

	while(vcount >= prim) {
		for(i=0; i<prim; i++) {
			xform(xv + i, varr, mat[mtop]);

			xv[i].x = (xv[i].x << 1) / (xv[i].z >> 8);	/* assume aspect: ~2 */
			xv[i].y = (xv[i].y << 2) / (xv[i].z >> 8);	/* the shift is * PROJ_D */
			/* projection result is 24.8 */
			/* viewport */
			xv[i].x = ((((xv[i].x + 0x100) >> 1) * vp[2]) >> 8) + vp[0];
			xv[i].y = ((((0x100 - xv[i].y) >> 1) * vp[3]) >> 8) + vp[1];
			varr++;
		}
		vcount -= prim;

		/* line clipping */
#ifndef ALT_LCLIP
		clip_line((int*)&xv[0].x, (int*)&xv[0].y, (int*)&xv[1].x, (int*)&xv[1].y, vp[0], vp[1], vp[2] - 1, vp[3] - 1);
#endif
		draw_line(xv[0].x, xv[0].y, xv[1].x, xv[1].y, varr[-2].cidx);
	}
}

void xgl_xyzzy(void)
{
	mat[mtop][12] = mat[mtop][13] = 0;
}
