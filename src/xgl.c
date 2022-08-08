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
/* near Z = 0.5 */
#define NEAR_Z	0x10000

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
		cidx = 0xff;//varr->cidx;

		xform(xv, varr, mat[mtop]);
		xform_norm(xv, varr, mat[mtop]);

		/* backfacing check */
		if(xv->nz > 0) goto skip_poly;

		/*
		if(opt & XGL_LIGHTING) {
			ndotl = (xv->nx >> 8) * ldir[0] + (xv->ny >> 8) * ldir[1] + (xv->nz >> 8) * ldir[2];
			if(ndotl < 0) ndotl = 0;
			cidx = 128 + (ndotl >> 9);
			if(cidx > 255) cidx = 255;
		}
		*/

		for(i=0; i<prim; i++) {
			if(i > 0) {
				xform(xv + i, varr + i, mat[mtop]);
			}
			xv[i].x = (xv[i].x << 1) / (xv[i].z >> 8);	/* assume aspect: ~2 */
			xv[i].y = (xv[i].y << 2) / (xv[i].z >> 8);	/* the shift is * PROJ_D */
			/* transform result is 24.8 */
		}

		/* clip against near plane */


		for(i=0; i<prim; i++) {
			/* viewport */
			pv[i].x = (((xv[i].x + 0x100) >> 1) * vp[2]) + (vp[0] << 8);
			pv[i].y = (((0x100 - xv[i].y) >> 1) * vp[3]) + (vp[1] << 8);
		}

		polyfill_flat(pv, prim, cidx);
skip_poly:
		varr += prim;
		vcount -= prim;
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
		draw_line(xv[0].x, xv[0].y, xv[1].x, xv[1].y, 0xff);
	}
}

void xgl_xyzzy(void)
{
	mat[mtop][12] = mat[mtop][13] = 0;
}

#define ISECT_NEAR(v0, v1)	((((v0)->z - NEAR_Z) << 8) / (((v0)->z - (v1)->z) >> 8))

#define LERP_VATTR(res, v0, v1, t) \
	do { \
		(res)->x = (v0)->x + (((v1)->x - (v0)->x) >> 8) * (t);	\
		(res)->y = (v0)->y + (((v1)->y - (v0)->y) >> 8) * (t);	\
		(res)->z = (v0)->z + (((v1)->z - (v0)->z) >> 8) * (t);	\
		(res)->nx = (v0)->nx + (((v1)->nx - (v0)->nx) >> 8) * (t);	\
		(res)->ny = (v0)->ny + (((v1)->ny - (v0)->ny) >> 8) * (t);	\
		(res)->nz = (v0)->nz + (((v1)->nz - (v0)->nz) >> 8) * (t);	\
		(res)->tx = (v0)->tx + (((v1)->tx - (v0)->tx) >> 8) * (t);	\
		(res)->ty = (v0)->ty + (((v1)->ty - (v0)->ty) >> 8) * (t);	\
		(res)->lit = (v0)->lit + (((v1)->lit - (v0)->lit) >> 8) * (t); \
	} while(0)

static int clip_edge_near(struct xvertex *poly, int *vnumptr, struct xvertex *v0, struct xvertex *v1)
{
	int vnum = *vnumptr;
	int in0, in1;
	int32_t t;
	struct xvertex *vptr;

	in0 = v0->z >= NEAR_Z ? 1 : 0;
	in1 = v1->z >= NEAR_Z ? 1 : 0;

	if(in0) {
		/* start inside */
		if(in1) {
			/* all inside */
			poly[vnum++] = *v1;	/* append v1 */
			*vnumptr = vnum;
			return 1;
		} else {
			/* going out */
			vptr = poly + vnum;
			t = ISECT_NEAR(v0, v1);	/* 24:8 */
			LERP_VATTR(vptr, v0, v1, t);
			++vnum;		/* append new vertex on the intersection point */
		}
	} else {
		/* start outside */
		if(in1) {
			/* going in */
			vptr = poly + vnum;
			t = ISECT_NEAR(v0, v1);
			LERP_VATTR(vptr, v0, v1, t);
			++vnum;		/* append new vertex ... */
			/* then append v1 */
			poly[vnum++] = *v1;
		} else {
			/* all outside */
			return -1;
		}
	}

	*vnumptr = vnum;
	return 0;
}

/* special case near-plane clipper */
int xgl_clip_near(struct xvertex *vout, int *voutnum, struct xvertex *vin, int vnum)
{
	int i, nextidx, res;
	int edges_clipped = 0;

	*voutnum = 0;

	for(i=0; i<vnum; i++) {
		nextidx = i + 1;
		if(nextidx >= vnum) nextidx = 0;
		res = clip_edge_near(vout, voutnum, vin + i, vin + nextidx);
		if(res == 0) {
			++edges_clipped;
		}
	}

	if(*voutnum <= 0) return -1;
	return edges_clipped > 0 ? 0 : 1;
}
