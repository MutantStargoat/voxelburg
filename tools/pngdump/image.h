#ifndef IMAGE_H_
#define IMAGE_H_

#include <stdio.h>

struct cmapent {
	unsigned char r, g, b;
};

struct image {
	int width, height;
	int bpp;
	int nchan;
	int scansz;	/* scanline size in bytes */
	int pitch;	/* bytes from one scanline to the next */
	int cmap_ncolors;
	struct cmapent cmap[256];
	unsigned char *pixels;
};

int alloc_image(struct image *img, int x, int y, int bpp);
int load_image(struct image *img, const char *fname);
int save_image(struct image *img, const char *fname);
int save_image_file(struct image *img, FILE *fp);

int cmp_image(struct image *a, struct image *b);

void blit(struct image *src, int sx, int sy, int w, int h, struct image *dst, int dx, int dy);
void overlay_key(struct image *src, unsigned int key, struct image *dst);

unsigned int get_pixel(struct image *img, int x, int y);
unsigned int get_pixel_rgb(struct image *img, int x, int y, unsigned int *rgb);
void put_pixel(struct image *img, int x, int y, unsigned int pix);

int quantize_image(struct image *img, int maxcol);
int gen_shades(struct image *img, int levels, int maxcol, int *shade_lut);

#endif	/* IMAGE_H_ */
