#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "voxscape.h"
#include "debug.h"

#define XLERP(a, b, t, fp) \
	((((a) << (fp)) + ((b) - (a)) * (t)) >> fp)

enum {
	SLICELEN	= 1
};

struct voxscape {
	int xsz, ysz;
	unsigned char *height;
	unsigned char *color;
	int xshift, xmask, ymask;

	int hfilt, cfilt;

	/* framebuffer */
	uint16_t *fb;
	int fbwidth, fbheight;
	int *coltop;
	int horizon;

	/* view */
	int32_t x, y, angle;
	int vheight;

	/* projection */
	int fov, znear, zfar;
	int nslices;
	int32_t *slicelen;
	int proj_dist;

	int zfog;	/* fog start Z (0: no fog) */
	uint8_t fogcolor;

	unsigned int valid;
};

struct voxscape *vox_create(int xsz, int ysz, uint8_t *himg, uint8_t *cimg)
{
	struct voxscape *vox;

	if(!(vox = calloc(1, sizeof *vox))) {
		return 0;
	}
	vox->height = himg;
	vox->color = cimg;
	vox->xsz = xsz;
	vox->ysz = ysz;

	vox->xmask = vox->xsz - 1;
	vox->ymask = vox->ysz - 1;

	vox->xshift = -1;
	while(xsz) {
		xsz >>= 1;
		vox->xshift++;
	}

	vox->vheight = 80;
	vox->proj_dist = 4;	/* TODO */

	return vox;
}

void vox_free(struct voxscape *vox)
{
	if(!vox) return;

	free(vox->color);
	free(vox->height);
	free(vox->coltop);
	free(vox->slicelen);
	free(vox);
}

uint8_t *vox_texture(struct voxscape *vox, uint8_t *data)
{
	if(data) {
		memcpy(vox->color, data, vox->xsz * vox->ysz);
	}
	return vox->color;
}

uint8_t *vox_heightmap(struct voxscape *vox, uint8_t *data)
{
	if(data) {
		memcpy(vox->height, data, vox->xsz * vox->ysz);
	}
	return vox->height;
}

void vox_fog(struct voxscape *vox, int zstart, uint8_t color)
{
	vox->zfog = zstart;
	vox->fogcolor = color;
}

#define H(x, y)	\
	vox->height[((((y) >> 16) & vox->ymask) << vox->xshift) + (((x) >> 16) & vox->xmask)]
#define C(x, y) \
	vox->color[((((y) >> 16) & vox->ymask) << vox->xshift) + (((x) >> 16) & vox->xmask)]


int vox_height(struct voxscape *vox, int32_t x, int32_t y)
{
	int32_t u, v;
	int h00, h01, h10, h11, h0, h1;

	if(!vox->hfilt) {
		return H(x, y);
	}

	h00 = H(x, y);
	h01 = H(x, y + 0x10000);
	h10 = H(x + 0x10000, y);
	h11 = H(x + 0x10000, y + 0x10000);

	u = x & 0xffff;
	v = y & 0xffff;

	h0 = XLERP(h00, h01, v, 16);
	h1 = XLERP(h10, h11, v, 16);
	return XLERP(h0, h1, u, 16);
}

int vox_color(struct voxscape *vox, int32_t x, int32_t y)
{
	int32_t u, v;
	int c00, c01, c10, c11, c0, c1;

	if(!vox->cfilt) {
		return C(x, y);
	}

	c00 = C(x, y);
	c01 = C(x, y + 0x10000);
	c10 = C(x + 0x10000, y);
	c11 = C(x + 0x10000, y + 0x10000);

	u = x & 0xffff;
	v = y & 0xffff;

	c0 = XLERP(c00, c01, v, 16);
	c1 = XLERP(c10, c11, v, 16);
	return XLERP(c0, c1, u, 16);
}


void vox_filter(struct voxscape *vox, int hfilt, int cfilt)
{
	vox->hfilt = hfilt;
	vox->cfilt = cfilt;
}

void vox_framebuf(struct voxscape *vox, int xres, int yres, void *fb, int horizon)
{
	if(xres != vox->fbwidth) {
		free(vox->coltop);
		if(!(vox->coltop = malloc(xres * sizeof *vox->coltop))) {
			fprintf(stderr, "vox_framebuf: failed to allocate column table (%d)\n", xres);
			return;
		}
	}
	vox->fb = fb;
	vox->fbwidth = xres;
	vox->fbheight = yres;
	vox->horizon = horizon >= 0 ? horizon : (vox->fbheight >> 1);
}

void vox_view(struct voxscape *vox, int32_t x, int32_t y, int h, int32_t angle)
{
	if(h < 0) {
		h = vox_height(vox, x, y) - h;
	}

	vox->x = x;
	vox->y = y;
	vox->vheight = h;
	vox->angle = angle;

	vox->valid &= ~SLICELEN;
}

void vox_proj(struct voxscape *vox, int fov, int znear, int zfar)
{
	vox->fov = fov;
	vox->znear = znear;
	vox->zfar = zfar;

	vox->nslices = vox->zfar - vox->znear;
	free(vox->slicelen);
	if(!(vox->slicelen = malloc(vox->nslices * sizeof *vox->slicelen))) {
		fprintf(stderr, "vox_proj: failed to allocate slice length table (%d)\n", vox->nslices);
		return;
	}

	vox->valid &= ~SLICELEN;
}

/* algorithm:
 * calculate extents of horizontal equidistant line from the viewer based on fov
 * for each column step along this line and compute height for each pixel
 * fill the visible (top) part of each column
 */

void vox_render(struct voxscape *vox)
{
	int i;

	vox_begin(vox);
	for(i=0; i<vox->nslices; i++) {
		vox_render_slice(vox, i);
	}
}

void vox_begin(struct voxscape *vox)
{
	int i;

	memset(vox->coltop, 0, vox->fbwidth * sizeof *vox->coltop);

	if(!(vox->valid & SLICELEN)) {
		float theta = (float)vox->fov * M_PI / 360.0f;	/* half angle */
		for(i=0; i<vox->nslices; i++) {
			vox->slicelen[i] = (int32_t)((vox->znear + i) * tan(theta) * 4.0f * 65536.0f);
		}
		vox->valid |= SLICELEN;
	}
}

void vox_render_slice(struct voxscape *vox, int n)
{
	int i, j, hval, colstart, colheight, z;
	int32_t x, y, len, xstep, ystep;
	uint8_t color;
	uint16_t *fbptr;

	z = vox->znear + n;

	len = vox->slicelen[n] >> 8;
	xstep = ((COS(vox->angle) >> 8) * len) / vox->fbwidth;
	ystep = ((SIN(vox->angle) >> 8) * len) / vox->fbwidth;

	x = vox->x - SIN(vox->angle) * z - xstep * (vox->fbwidth >> 1);
	y = vox->y + COS(vox->angle) * z - ystep * (vox->fbwidth >> 1);
	/* TODO double column */
	for(i=0; i<vox->fbwidth/2; i++) {
		hval = vox_height(vox, x, y) - vox->vheight;
		hval = hval * 160 / (vox->znear + n) + vox->horizon;
		if(hval > vox->fbheight) hval = vox->fbheight;
		if(hval > vox->coltop[i]) {
			color = vox_color(vox, x, y);
			colstart = vox->fbheight - hval;
			colheight = hval - vox->coltop[i];
			fbptr = vox->fb + colstart * vox->fbwidth / 2 + i / 2;

			for(j=0; j<colheight; j++) {
				*fbptr = color | ((uint16_t)color << 8);
				fbptr += vox->fbwidth >> 1;
			}
			vox->coltop[i] = hval;
		}

		x += xstep;
		y += ystep;
	}
}

void vox_sky_solid(struct voxscape *vox, uint8_t color)
{
	int i, j, colheight;
	uint16_t *fbptr;

	/* TODO double columns */
	for(i=0; i<vox->fbwidth/2; i++) {
		fbptr = vox->fb + i;
		colheight = vox->fbheight - vox->coltop[i];
		for(j=0; j<colheight; j++) {
			*fbptr = color | ((uint16_t)color << 8);
			fbptr += vox->fbwidth >> 1;
		}
	}
}

void vox_sky_grad(struct voxscape *vox, uint8_t chor, uint8_t ctop)
{
	int i, j, colheight, t;
	int d = vox->fbheight - vox->horizon;
	uint8_t *grad;
	uint16_t *fbptr;

	grad = alloca(vox->fbheight * sizeof *grad);

	for(i=0; i<d; i++) {
		t = (i << 8) / d;
		grad[i] = XLERP(ctop, chor, t, 8);
	}
	for(i=d; i<vox->fbheight; i++) {
		grad[i] = chor;
	}

	/* TODO double columns */
	for(i=0; i<vox->fbwidth/2; i++) {
		fbptr = vox->fb + i / 2;
		colheight = vox->fbheight - vox->coltop[i];
		for(j=0; j<colheight; j++) {
			*fbptr = grad[j] | ((uint16_t)grad[j] << 8);
			fbptr += vox->fbwidth >> 1;
		}
	}
}
