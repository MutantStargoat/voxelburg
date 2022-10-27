#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "voxscape.h"
#include "debug.h"
#include "data.h"

/* hardcoded dimensions for the GBA */
#define FBWIDTH		240
#define FBHEIGHT	160
#define FBPITCH		240
/* map size */
#define XSZ			512
#define YSZ			512
#define XSHIFT		9
#define XMASK		0x1ff
#define YMASK		0x1ff
#define HSCALE		40

/* XXX */
#define OBJ_STRIDE_SHIFT	5

#define NO_LERP

#define XLERP(a, b, t, fp) \
	((((a) << (fp)) + ((b) - (a)) * (t)) >> fp)

enum {
	SLICELEN	= 1
};

static unsigned char *vox_height;
static unsigned char *vox_color;
/* framebuffer */
static uint16_t *vox_fb;
static int *vox_coltop;
static int vox_horizon;
/* view */
static int32_t vox_x, vox_y, vox_angle;
static int vox_vheight;
/* projection */
static int vox_fov, vox_znear, vox_zfar;
static int vox_nslices;
static int32_t *vox_slicelen;

static unsigned int vox_valid;

static struct vox_object *vox_obj;
static int vox_num_obj, vox_obj_stride;

int *projlut;

int vox_init(int xsz, int ysz, uint8_t *himg, uint8_t *cimg)
{
	assert(xsz == XSZ && ysz == YSZ);

	vox_height = himg;
	vox_color = cimg;

	vox_fb = 0;
	vox_coltop = 0;
	vox_horizon = 0;
	vox_x = vox_y = vox_angle = 0;
	vox_fov = 0;
	vox_znear = vox_zfar = 0;
	vox_nslices = 0;
	vox_slicelen = 0;
	vox_valid = 0;
	projlut = 0;

	vox_vheight = 80;

	return 0;
}

void vox_destroy(void)
{
	/* XXX we rely on the screen to clear up any allocated IWRAM */
}

#define H(x, y)	\
	vox_height[((((y) >> 16) & YMASK) << XSHIFT) + (((x) >> 16) & XMASK)]
#define C(x, y) \
	vox_color[((((y) >> 16) & YMASK) << XSHIFT) + (((x) >> 16) & XMASK)]

void vox_framebuf(int xres, int yres, void *fb, int horizon)
{
	if(!vox_coltop) {
		if(!(vox_coltop = iwram_sbrk(xres * sizeof *vox_coltop))) {
			panic(get_pc(), "vox_framebuf: failed to allocate column table (%d)\n", xres);
		}
	}
	vox_fb = fb;
	vox_horizon = horizon >= 0 ? horizon : (FBHEIGHT >> 1);
}

void vox_view(int32_t x, int32_t y, int h, int32_t angle)
{
	if(h < 0) {
		h = H(x, y) - h;
	}

	vox_x = x;
	vox_y = y;
	vox_vheight = h;
	vox_angle = angle;

	vox_valid &= ~SLICELEN;
}

void vox_proj(int fov, int znear, int zfar)
{
	vox_fov = fov;
	vox_znear = znear;
	vox_zfar = zfar;

	vox_nslices = vox_zfar - vox_znear;
	if(!vox_slicelen) {
		if(!(vox_slicelen = iwram_sbrk(vox_nslices * sizeof *vox_slicelen))) {
			panic(get_pc(), "vox_proj: failed to allocate slice length table (%d)\n", vox_nslices);
		}
		if(!(projlut = iwram_sbrk(vox_nslices * sizeof *projlut))) {
			panic(get_pc(), "vox_framebuf: failed to allocate projection table (%d)\n", vox_nslices);
		}
	}

	vox_valid &= ~SLICELEN;
}

/* algorithm:
 * calculate extents of horizontal equidistant line from the viewer based on fov
 * for each column step along this line and compute height for each pixel
 * fill the visible (top) part of each column
 */
ARM_IWRAM
void vox_render(void)
{
	int i;

	vox_begin();

	for(i=0; i<vox_nslices; i++) {
		vox_render_slice(i);
	}
}

ARM_IWRAM
void vox_begin(void)
{
	int i;

	memset(vox_coltop, 0, FBWIDTH * sizeof *vox_coltop);

	if(!(vox_valid & SLICELEN)) {
		float theta = (float)vox_fov * M_PI / 360.0f;	/* half angle */
		for(i=0; i<vox_nslices; i++) {
			vox_slicelen[i] = (int32_t)((vox_znear + i) * tan(theta) * 4.0f * 65536.0f);
			projlut[i] = (HSCALE << 8) / (vox_znear + i);
		}
		vox_valid |= SLICELEN;
	}
}

ARM_IWRAM
void vox_render_slice(int n)
{
	int i, j, hval, last_hval, colstart, colheight, col, z, offs, last_offs = -1;
	int32_t x, y, len, xstep, ystep;
	uint8_t color, last_col;
	uint16_t *fbptr;
	/*int proj;*/
	struct vox_object *obj;

	z = vox_znear + n;

	len = vox_slicelen[n] >> 8;
	xstep = (((COS(vox_angle) >> 4) * len) >> 4) / (FBWIDTH / 2);
	ystep = (((SIN(vox_angle) >> 4) * len) >> 4) / (FBWIDTH / 2);

	x = vox_x - SIN(vox_angle) * z - xstep * (FBWIDTH / 4);
	y = vox_y + COS(vox_angle) * z - ystep * (FBWIDTH / 4);

	/*proj = (HSCALE << 8) / (vox_znear + n);*/

	for(i=0; i<FBWIDTH/2; i++) {
		col = i << 1;
		offs = (((y >> 16) & YMASK) << XSHIFT) + ((x >> 16) & XMASK);
		if(offs == last_offs) {
			hval = last_hval;
			color = last_col;
		} else {
			hval = vox_height[offs] - vox_vheight;
			hval = ((hval * projlut[n]) >> 8) + vox_horizon;
			if(hval > FBHEIGHT) hval = FBHEIGHT;
			color = vox_color[offs];
			last_offs = offs;
			last_hval = hval;
			last_col = color;
		}
		if(hval >= vox_coltop[col]) {
			colstart = FBHEIGHT - hval;
			colheight = hval - vox_coltop[col];
			fbptr = vox_fb + colstart * (FBPITCH / 2) + i;

			for(j=0; j<colheight; j++) {
				*fbptr = color | ((uint16_t)color << 8);
				fbptr += FBPITCH / 2;
			}
			vox_coltop[col] = hval;

			/* check to see if there's an object here */
			if(color >= CMAP_SPAWN0) {
				int idx = color - CMAP_SPAWN0;
				obj = (struct vox_object*)((char*)vox_obj + (idx << OBJ_STRIDE_SHIFT));
				obj->px = col;
				obj->py = colstart;
				obj->scale = projlut[n];
			}
		}
		x += xstep;
		y += ystep;
	}
}

ARM_IWRAM
void vox_sky_solid(uint8_t color)
{
	int i, j, colheight;
	uint16_t *fbptr;

	for(i=0; i<FBWIDTH / 2; i++) {
		fbptr = vox_fb + i;
		colheight = FBHEIGHT - vox_coltop[i << 1];

		for(j=0; j<colheight; j++) {
			*fbptr = color | ((uint16_t)color << 8);
			fbptr += FBPITCH / 2;
		}
	}
}

ARM_IWRAM
void vox_sky_grad(uint8_t chor, uint8_t ctop)
{
	int i, j, colheight, t;
	int d = FBHEIGHT - vox_horizon;
	uint8_t grad[FBHEIGHT];
	uint16_t *fbptr;

	for(i=0; i<d; i++) {
		t = (i << 16) / d;
		grad[i] = XLERP(ctop, chor, t, 16);
	}
	for(i=d; i<FBHEIGHT; i++) {
		grad[i] = chor;
	}

	for(i=0; i<FBWIDTH / 2; i++) {
		fbptr = vox_fb + i;
		colheight = FBHEIGHT - vox_coltop[i << 1];

		for(j=0; j<colheight; j++) {
			*fbptr = grad[j] | ((uint16_t)grad[j] << 8);
			fbptr += FBPITCH / 2;
		}
	}
}

void vox_objects(struct vox_object *ptr, int count, int stride)
{
	int i;
	struct vox_object *obj;

	if(stride != 1 << OBJ_STRIDE_SHIFT) {
		panic(get_pc(), "optimization requires %d byte vox obj (got %d)",
				1 << OBJ_STRIDE_SHIFT, stride);
	}

	vox_obj = ptr;
	vox_num_obj = count;
	vox_obj_stride = stride;

	obj = ptr;
	for(i=0; i<count; i++) {
		obj->offs = obj->y * XSZ + obj->x;
		obj = (struct vox_object*)((char*)obj + stride);
	}
}
