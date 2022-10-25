#ifndef VOXSCAPE_H_
#define VOXSCAPE_H_

#include <stdint.h>

enum {
	VOX_NEAREST,
	VOX_LINEAR
};

struct voxscape;

struct vox_object {
	int x, y, px, py;
	int offs;
	int32_t scale;
};

extern int vox_quality;

struct voxscape *vox_create(int xsz, int ysz, uint8_t *himg, uint8_t *cimg);
void vox_free(struct voxscape *vox);

/* data argument can be null */
uint8_t *vox_texture(struct voxscape *vox, uint8_t *data);
uint8_t *vox_heightmap(struct voxscape *vox, uint8_t *data);

void vox_fog(struct voxscape *vox, int zstart, uint8_t color);
int vox_height(struct voxscape *vox, int32_t x, int32_t y);

void vox_filter(struct voxscape *vox, int hfilt, int cfilt);

void vox_framebuf(struct voxscape *vox, int xres, int yres, void *fb, int horizon);
/* negative height for auto at -h above terrain */
void vox_view(struct voxscape *vox, int32_t x, int32_t y, int h, int32_t angle);
void vox_proj(struct voxscape *vox, int fov, int znear, int zfar);

void vox_render(struct voxscape *vox);

void vox_begin(struct voxscape *vox);
void vox_render_slice(struct voxscape *vox, int n);

void vox_sky_solid(struct voxscape *vox, uint8_t color);
void vox_sky_grad(struct voxscape *vox, uint8_t chor, uint8_t ctop);

void vox_objects(struct voxscape *vox, struct vox_object *ptr, int count, int stride);

#endif	/* VOXSCAPE_H_ */
