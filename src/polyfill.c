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
#include "polyfill.h"
#include "debug.h"

static unsigned char *fb;
static int fbwidth, fbheight;
static short scantab[2][160] __attribute__((section(".iwram")));

void polyfill_framebuffer(unsigned char *ptr, int w, int h)
{
	fb = ptr;
	fbwidth = w;
	fbheight = h;
}

#define VNEXT(p)	(((p) == vlast) ? varr : (p) + 1)
#define VPREV(p)	((p) == varr ? vlast : (p) - 1)
#define VSUCC(p, side)	((side) == 0 ? VNEXT(p) : VPREV(p))

void polyfill_flat(struct pvertex *varr, int vnum, unsigned char col)
{
	int i, line, top, bot;
	struct pvertex *vlast, *v, *vn;
	int32_t x, y0, y1, dx, dy, slope, fx, fy;
	short *tab, start, len;
	unsigned char *fbptr;
	uint16_t *pptr, pcol = ((uint16_t)col << 8) | (uint16_t)col;

	vlast = varr + vnum - 1;
	top = fbheight;
	bot = 0;

	for(i=0; i<vnum; i++) {
		v = varr + i;
		vn = VNEXT(v);

		if(vn->y == v->y) continue;

		if(vn->y > v->y) {
			tab = scantab[0];
		} else {
			tab = scantab[1];
			v = vn;
			vn = varr + i;
		}

		dx = vn->x - v->x;
		dy = vn->y - v->y;
		slope = (dx << 8) / dy;

		y0 = (v->y + 0x100) & 0xffffff00;	/* start from the next scanline */
		fy = y0 - v->y;						/* fractional part before the next scanline */
		fx = (fy * slope) >> 8;				/* X adjust for the step to the next scanline */
		x = v->x + fx;						/* adjust X */
		y1 = vn->y & 0xffffff00;			/* last scanline of the edge <= vn->y */

		line = y0 >> 8;
		if(line < top) top = line;
		if((y1 >> 8) > bot) bot = y1 >> 8;

		if(line > 0) tab += line;

		while(line <= (y1 >> 8) && line < fbheight) {
			if(line >= 0) {
				int val = x < 0 ? 0 : x >> 8;
				*tab++ = val < fbwidth ? val : fbwidth - 1;
			}
			x += slope;
			line++;
		}
	}

	fbptr = fb + top * fbwidth;
	for(i=top; i<=bot; i++) {
		start = scantab[0][i];
		len = scantab[1][i] - start;

		if(len > 0) {
			if(start & 1) {
				pptr = (uint16_t*)(fbptr + (start & 0xfffe));
				*pptr = (*pptr & 0xff) | ((uint16_t)col << 8);
				len--;
				start++;
			}
			pptr = (uint16_t*)(fbptr + start);
			while(len > 1) {
				*pptr++ = pcol;
				len -= 2;
			}
			if(len) {
				*pptr = (*pptr & 0xff00) | col;
			}
		}
		fbptr += fbwidth;
	}
}


/* ----- line drawing and clipping ------ */
enum {
	IN		= 0,
	LEFT	= 1,
	RIGHT	= 2,
	TOP		= 4,
	BOTTOM	= 8
};

static int outcode(int x, int y, int xmin, int ymin, int xmax, int ymax)
{
	int code = 0;

	if(x < xmin) {
		code |= LEFT;
	} else if(x > xmax) {
		code |= RIGHT;
	}
	if(y < ymin) {
		code |= TOP;
	} else if(y > ymax) {
		code |= BOTTOM;
	}
	return code;
}

#define FIXMUL(a, b)	(((a) * (b)) >> 8)
#define FIXDIV(a, b)	(((a) << 8) / (b))

#define LERP(a, b, t)	((a) + FIXMUL((b) - (a), (t)))

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax)
{
	int oc_out;

	int oc0 = outcode(*x0, *y0, xmin, ymin, xmax, ymax);
	int oc1 = outcode(*x1, *y1, xmin, ymin, xmax, ymax);

	long fx0, fy0, fx1, fy1, fxmin, fymin, fxmax, fymax;

	if(!(oc0 | oc1)) return 1;	/* both points are inside */

	fx0 = *x0 << 8;
	fy0 = *y0 << 8;
	fx1 = *x1 << 8;
	fy1 = *y1 << 8;
	fxmin = xmin << 8;
	fymin = ymin << 8;
	fxmax = xmax << 8;
	fymax = ymax << 8;

	for(;;) {
		long x, y, t;

		if(oc0 & oc1) return 0;		/* both have points with the same outbit, not visible */
		if(!(oc0 | oc1)) break;		/* both points are inside */

		oc_out = oc0 ? oc0 : oc1;

		if(oc_out & TOP) {
			t = FIXDIV(fymin - fy0, fy1 - fy0);
			x = LERP(fx0, fx1, t);
			y = fymin;
		} else if(oc_out & BOTTOM) {
			t = FIXDIV(fymax - fy0, fy1 - fy0);
			x = LERP(fx0, fx1, t);
			y = fymax;
		} else if(oc_out & LEFT) {
			t = FIXDIV(fxmin - fx0, fx1 - fx0);
			x = fxmin;
			y = LERP(fy0, fy1, t);
		} else {/*if(oc_out & RIGHT) {*/
			t = FIXDIV(fxmax - fx0, fx1 - fx0);
			x = fxmax;
			y = LERP(fy0, fy1, t);
		}

		if(oc_out == oc0) {
			fx0 = x;
			fy0 = y;
			oc0 = outcode(fx0 >> 8, fy0 >> 8, xmin, ymin, xmax, ymax);
		} else {
			fx1 = x;
			fy1 = y;
			oc1 = outcode(fx1 >> 8, fy1 >> 8, xmin, ymin, xmax, ymax);
		}
	}

	*x0 = fx0 >> 8;
	*y0 = fy0 >> 8;
	*x1 = fx1 >> 8;
	*y1 = fy1 >> 8;
	return 1;
}

#ifdef ALT_LCLIP
#define PUTPIXEL(ptr) \
	do { \
		if(x0 >= 0 && x0 < fbwidth && y0 >= 0 && y0 < fbheight) { \
			uint16_t *pptr = (uint16_t*)((uint32_t)ptr & 0xfffffffe); \
			if((uint32_t)ptr & 1) { \
				*pptr = (*pptr & 0xff) | (color << 8); \
			} else { \
				*pptr = (*pptr & 0xff00) | color; \
			} \
		} \
	} while(0)
#else	/* !ALT_LCLIP */
#define PUTPIXEL(ptr) \
	do { \
		uint16_t *pptr = (uint16_t*)((uint32_t)ptr & 0xfffffffe); \
		if((uint32_t)ptr & 1) { \
			*pptr = (*pptr & 0xff) | (color << 8); \
		} else { \
			*pptr = (*pptr & 0xff00) | color; \
		} \
	} while(0)
#endif

void draw_line(int x0, int y0, int x1, int y1, unsigned short color)
{
	int i, dx, dy, x_inc, y_inc, error;
#ifdef ALT_LCLIP
	int y0inc;
#endif
	unsigned char *fbptr = fb;

	fbptr += y0 * fbwidth + x0;

	dx = x1 - x0;
	dy = y1 - y0;

	if(dx >= 0) {
		x_inc = 1;
	} else {
		x_inc = -1;
		dx = -dx;
	}
	if(dy >= 0) {
		y_inc = fbwidth;
#ifdef ALT_LCLIP
		y0inc = 1;
#endif
	} else {
		y_inc = -fbwidth;
#ifdef ALT_LCLIP
		y0inc = -1;
#endif
		dy = -dy;
	}

	if(dx > dy) {
		error = dy * 2 - dx;
		for(i=0; i<=dx; i++) {
			PUTPIXEL(fbptr);
			if(error >= 0) {
				error -= dx * 2;
				fbptr += y_inc;
#ifdef ALT_LCLIP
				y0 += y0inc;
#endif
			}
			error += dy * 2;
			fbptr += x_inc;
#ifdef ALT_LCLIP
			x0 += x_inc;
#endif
		}
	} else {
		error = dx * 2 - dy;
		for(i=0; i<=dy; i++) {
			PUTPIXEL(fbptr);
			if(error >= 0) {
				error -= dy * 2;
				fbptr += x_inc;
#ifdef ALT_LCLIP
				x0 += x_inc;
#endif
			}
			error += dx * 2;
			fbptr += y_inc;
#ifdef ALT_LCLIP
			y0 += y0inc;
#endif
		}
	}
}

