#ifndef VOXSCAPE_H_
#define VOXSCAPE_H_

#include <stdint.h>

enum {
	VOX_NEAREST,
	VOX_LINEAR
};

struct vox_object {
	int x, y, px, py;
	int offs;
	int32_t scale;
};

extern int vox_quality;

int vox_init(int xsz, int ysz, uint8_t *himg, uint8_t *cimg);
void vox_destroy(void);

void vox_framebuf(int xres, int yres, void *fb, int horizon);
/* negative height for auto at -h above terrain */
void vox_view(int32_t x, int32_t y, int h, int32_t angle);
void vox_proj(int fov, int znear, int zfar);

void vox_render(void);

void vox_begin(void);
void vox_render_slice(int n);

void vox_sky_solid(uint8_t color);
void vox_sky_grad(uint8_t chor, uint8_t ctop);

void vox_objects(struct vox_object *ptr, int count, int stride);

#endif	/* VOXSCAPE_H_ */
