#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <png.h>
#include "image.h"

int alloc_image(struct image *img, int x, int y, int bpp)
{
	memset(img, 0, sizeof *img);
	img->width = x;
	img->height = y;
	img->bpp = bpp;
	img->scansz = img->pitch = x * (bpp == 15 ? 16 : bpp) / 8;

	if(!(img->pixels = malloc(y * img->scansz))) {
		fprintf(stderr, "failed to allocate %dx%d (%dbpp) pixel buffer\n", x, y, bpp);
		return -1;
	}

	/* just a guess, assume the user will fill the details, but set up reasonable
	 * defaults just in case...
	 */
	if(bpp <= 8) {
		img->nchan = 1;
		img->cmap_ncolors = 1 << bpp;
	} else if(bpp <= 24) {
		img->nchan = 3;
	} else {
		img->nchan = 4;
	}
	return 0;
}

int load_image(struct image *img, const char *fname)
{
	int i;
	FILE *fp;
	png_struct *png;
	png_info *info;
	int chan_bits, color_type;
	png_uint_32 xsz, ysz;
	png_color *palette;
	unsigned char **scanline;
	unsigned char *dptr;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open: %s: %s\n", fname, strerror(errno));
		return -1;
	}

	if(!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))) {
		fclose(fp);
		return -1;
	}
	if(!(info = png_create_info_struct(png))) {
		fclose(fp);
		png_destroy_read_struct(&png, 0, 0);
		return -1;
	}
	if(setjmp(png_jmpbuf(png))) {
		fclose(fp);
		png_destroy_read_struct(&png, &info, 0);
		return -1;
	}

	png_init_io(png, fp);
	png_read_png(png, info, 0, 0);

	png_get_IHDR(png, info, &xsz, &ysz, &chan_bits, &color_type, 0, 0, 0);
	img->width = xsz;
	img->height = ysz;
	img->nchan = png_get_channels(png, info);
	img->bpp = img->nchan * chan_bits;
	img->scansz = img->pitch = xsz * img->bpp / 8;
	img->cmap_ncolors = 0;

	if(color_type == PNG_COLOR_TYPE_PALETTE) {
		png_get_PLTE(png, info, &palette, &img->cmap_ncolors);
		memcpy(img->cmap, palette, img->cmap_ncolors * sizeof *img->cmap);
	}

	if(!(img->pixels = malloc(ysz * img->scansz))) {
		perror("failed to allocate pixel buffer");
		fclose(fp);
		png_destroy_read_struct(&png, &info, 0);
		return -1;
	}
	dptr = img->pixels;

	scanline = (unsigned char**)png_get_rows(png, info);
	for(i=0; i<ysz; i++) {
		memcpy(dptr, scanline[i], img->scansz);
		dptr += img->pitch;
	}

	fclose(fp);
	png_destroy_read_struct(&png, &info, 0);
	return 0;
}

int save_image(struct image *img, const char *fname)
{
	FILE *fp;
	int res;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "save_image: failed to open: %s: %s\n", fname, strerror(errno));
		return -1;
	}
	res = save_image_file(img, fp);
	fclose(fp);
	return res;
}

int save_image_file(struct image *img, FILE *fp)
{
	int i, chan_bits, coltype;
	png_struct *png;
	png_info *info;
	png_text txt;
	unsigned char **scanline = 0;
	unsigned char *pptr;

	if(!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))) {
		fclose(fp);
		return -1;
	}
	if(!(info = png_create_info_struct(png))) {
		png_destroy_write_struct(&png, 0);
		fclose(fp);
		return -1;
	}

	txt.compression = PNG_TEXT_COMPRESSION_NONE;
	txt.key = "Software";
	txt.text = "pngdump";
	txt.text_length = 0;

	if(setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		free(scanline);
		fclose(fp);
		return -1;
	}

	switch(img->nchan) {
	case 1:
		if(img->cmap_ncolors > 0) {
			coltype = PNG_COLOR_TYPE_PALETTE;
		} else {
			coltype = PNG_COLOR_TYPE_GRAY;
		}
		break;
	case 2:
		coltype = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;
	case 3:
		coltype = PNG_COLOR_TYPE_RGB;
		break;
	case 4:
		coltype = PNG_COLOR_TYPE_RGB_ALPHA;
		break;
	}

	chan_bits = img->bpp / img->nchan;
	png_set_IHDR(png, info, img->width, img->height, chan_bits, coltype, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_text(png, info, &txt, 1);

	if(img->cmap_ncolors > 0) {
		png_set_PLTE(png, info, (png_color*)img->cmap, img->cmap_ncolors);
	}

	if(!(scanline = malloc(img->height * sizeof *scanline))) {
		png_destroy_write_struct(&png, &info);
		fclose(fp);
		return -1;
	}

	pptr = img->pixels;
	for(i=0; i<img->height; i++) {
		scanline[i] = pptr;
		pptr += img->pitch;
	}
	png_set_rows(png, info, scanline);

	png_init_io(png, fp);
	png_write_png(png, info, 0, 0);
	png_destroy_write_struct(&png, &info);
	free(scanline);
	return 0;
}


int cmp_image(struct image *a, struct image *b)
{
	int i;
	unsigned char *aptr = a->pixels;
	unsigned char *bptr = b->pixels;

	if(a->width != b->width || a->height != b->height || a->bpp != b->bpp || a->nchan != b->nchan) {
		return -1;
	}

	for(i=0; i<a->height; i++) {
		if(memcmp(aptr, bptr, a->scansz) != 0) {
			return -1;
		}
		aptr += a->pitch;
		bptr += b->pitch;
	}
	return 0;
}

void blit(struct image *src, int sx, int sy, int w, int h, struct image *dst, int dx, int dy)
{
	int i;
	unsigned char *sptr, *dptr;

	assert(src->bpp == dst->bpp);
	assert(src->nchan == dst->nchan);

	if(sx < 0) { w += sx; sx = 0; }
	if(sy < 0) { h += sy; sy = 0; }
	if(dx < 0) { w += dx; sx -= dx; dx = 0; }
	if(dy < 0) { h += dy; sy -= dy; dy = 0; }
	if(sx + w >= src->width) w = src->width - sx;
	if(sy + h >= src->height) h = src->height - sy;
	if(dx + w >= dst->width) w = dst->width - dx;
	if(dy + h >= dst->height) h = dst->height - dy;

	if(w <= 0 || h <= 0) return;

	sptr = src->pixels + sy * src->pitch + sx * src->bpp / 8;
	dptr = dst->pixels + dy * dst->pitch + dx * dst->bpp / 8;

	for(i=0; i<h; i++) {
		memcpy(dptr, sptr, w * dst->bpp / 8);
		dptr += dst->pitch;
		sptr += src->pitch;
	}
}

unsigned int get_pixel(struct image *img, int x, int y)
{
	unsigned int r, g, b;
	unsigned char *pptr;
	unsigned short *pptr16;
	unsigned int *pptr32;

	switch(img->bpp) {
	case 4:
		pptr = img->pixels + y * img->pitch + x / 2;
		return x & 1 ? *pptr & 0xf : *pptr >> 4;
	case 8:
		pptr = img->pixels + y * img->pitch + x;
		return *pptr;
	case 15:
	case 16:
		pptr16 = (unsigned short*)(img->pixels + y * img->pitch + x * 2);
		return *pptr16;
	case 24:
		pptr = img->pixels + y * img->pitch + x * 3;
		r = pptr[0];
		g = pptr[1];
		b = pptr[2];
		return r | (g << 8) | (b << 16);
	case 32:
		pptr32 = (unsigned int*)(img->pixels + y * img->pitch + x * 4);
		return *pptr32;

	default:
		fprintf(stderr, "get_pixel not implemented for %d bpp\n", img->bpp);
	}

	return 0;
}

unsigned int get_pixel_rgb(struct image *img, int x, int y, unsigned int *rgb)
{
	unsigned int pix = get_pixel(img, x, y);

	switch(img->bpp) {
	case 15:
		rgb[0] = (pix & 0x7c00) >> 7;
		rgb[1] = (pix & 0x03e0) >> 2;
		rgb[2] = (pix & 0x001f) << 3;
		rgb[0] |= ((rgb[0] & 8) >> 1) | ((rgb[0] & 8) >> 2) | ((rgb[0] & 8) >> 3);
		rgb[1] |= ((rgb[1] & 8) >> 1) | ((rgb[1] & 8) >> 2) | ((rgb[1] & 8) >> 3);
		rgb[2] |= ((rgb[2] & 8) >> 1) | ((rgb[2] & 8) >> 2) | ((rgb[2] & 8) >> 3);
		break;

	case 16:
		rgb[0] = (pix & 0xf800) >> 8;
		rgb[1] = (pix & 0x07e0) >> 3;
		rgb[2] = (pix & 0x001f) << 3;
		rgb[0] |= ((rgb[0] & 8) >> 1) | ((rgb[0] & 8) >> 2) | ((rgb[0] & 8) >> 3);
		rgb[1] |= ((rgb[1] & 4) >> 1) | ((rgb[1] & 4) >> 2);
		rgb[2] |= ((rgb[2] & 8) >> 1) | ((rgb[2] & 8) >> 2) | ((rgb[2] & 8) >> 3);
		break;

	case 24:
	case 32:
		rgb[0] = pix & 0xff;
		rgb[1] = (pix >> 8) & 0xff;
		rgb[2] = (pix >> 16) & 0xff;
		break;

	default:
		assert(pix >= 0 && pix < img->cmap_ncolors);
		rgb[0] = img->cmap[pix].r;
		rgb[1] = img->cmap[pix].g;
		rgb[2] = img->cmap[pix].b;
	}

	return pix;
}

void put_pixel(struct image *img, int x, int y, unsigned int pix)
{
	unsigned char *pptr;
	unsigned short *pptr16;

	switch(img->bpp) {
	case 4:
		pptr = img->pixels + y * img->pitch + x / 2;
		if(x & 1) {
			*pptr = (*pptr & 0xf0) | pix;
		} else {
			*pptr = (*pptr & 0xf) | (pix << 4);
		}
		break;

	case 8:
		pptr = img->pixels + y * img->pitch + x;
		*pptr = pix;
		break;

	case 15:
	case 16:
		pptr16 = (unsigned short*)(img->pixels + y * img->pitch + x * 2);
		*pptr16 = pix;
		break;

	default:
		fprintf(stderr, "put_pixel not implemented for %d bpp\n", img->bpp);
	}
}

void overlay_key(struct image *src, unsigned int key, struct image *dst)
{
	int i, j;
	unsigned int pix;

	assert(src->bpp == dst->bpp);
	assert(src->width == dst->width);
	assert(src->height == dst->height);

	for(i=0; i<dst->height; i++) {
		for(j=0; j<dst->width; j++) {
			pix = get_pixel(src, j, i);
			if(pix != key) {
				put_pixel(dst, j, i, pix);
			}
		}
	}
}

#if 0
/* ---- color quantization ---- */
struct octnode;

struct octree {
	struct octnode *root;
	struct octnode *levn[8];
	int ncol, maxcol;
};

struct octnode {
	struct octree *tree;
	int r, g, b, nref;
	int palidx;
	int nsub;
	struct octnode *sub[8];
	struct octnode *next;
};

static void add_color(struct octree *ot, int r, int g, int b);
static void reduce_colors(struct octree *ot);
static int assign_colors(struct octnode *on, int next, struct cmapent *cmap);
static int lookup_color(struct octree *ot, int r, int g, int b);
static struct octnode *new_node(struct octree *ot, int lvl);
static void del_node(struct octnode *on, int lvl);
static void print_tree(struct octnode *n, int lvl);
static int count_leaves(struct octnode *n);

void quantize_image(struct image *img, int maxcol)
{
	int i, j, cidx;
	unsigned int rgb[3];
	struct octree ot = {0};
	struct image newimg = *img;

	if(img->bpp > 8) {
		newimg.bpp = 8;
		newimg.nchan = 1;
		newimg.scansz = newimg.width;
		newimg.pitch = 8 * img->pitch / img->bpp;
	}

	ot.root = new_node(&ot, 0);
	ot.maxcol = maxcol;

	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			get_pixel_rgb(img, j, i, rgb);
			add_color(&ot, rgb[0], rgb[1], rgb[2]);

			while(count_leaves(ot.root) > ot.maxcol) {
			//while(ot.ncol > ot.maxcol) {
				reduce_colors(&ot);
			}
		}
	}

	/* use created octree to generate the palette */
	newimg.cmap_ncolors = assign_colors(ot.root, 0, newimg.cmap);

	/* replace image pixels */
	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			get_pixel_rgb(img, j, i, rgb);
			cidx = lookup_color(&ot, rgb[0], rgb[1], rgb[2]);
			assert(cidx >= 0 && cidx < maxcol);
			put_pixel(&newimg, j, i, cidx);
		}
	}

	*img = newimg;
}

static int subidx(int bit, int r, int g, int b)
{
	assert(bit >= 0 && bit < 8);
	bit = 7 - bit;
	return ((r >> bit) & 1) | ((g >> (bit - 1)) & 2) | ((b >> (bit - 2)) & 4);
}

static int tree_height(struct octnode *on)
{
	int i, subh, max = 0;

	if(!on) return 0;

	for(i=0; i<8; i++) {
		subh = tree_height(on->sub[i]);
		if(subh > max) max = subh;
	}
	return max + 1;
}

static void add_color(struct octree *ot, int r, int g, int b)
{
	int i, idx;
	struct octnode *on;

	on = ot->root;
	for(i=0; i<8; i++) {
		idx = subidx(i, r, g, b);

		if(!on->sub[idx]) {
			on->sub[idx] = new_node(ot, i + 1);
			if(i == 7) {
				/* this only adds a color if the parent node was previously not
				 * a leaf. Otherwise the new one just takes the parent's place
				 */
				ot->ncol++;
			}
			on->nsub++;
		}

		on->r += r;
		on->g += g;
		on->b += b;
		on->nref++;

		on = on->sub[idx];
	}

	on->r += r;
	on->g += g;
	on->b += b;
	on->nref++;
}

static int count_nodes(struct octnode *n)
{
	int count = 0;
	while(n) {
		count++;
		n = n->next;
	}
	return count;
}

static int count_leaves(struct octnode *n)
{
	int i, cnt;

	if(!n) return 0;
	if(n->nsub <= 0) return 1;

	cnt = 0;
	for(i=0; i<8; i++) {
		cnt += count_leaves(n->sub[i]);
	}
	return cnt;
}

static void reduce_colors(struct octree *ot)
{
	int i, lvl, best_nref;
	struct octnode *n, *best;

	lvl = 8;
	while(--lvl >= 0) {
		best_nref = INT_MAX;
		best = 0;
		n = ot->levn[lvl];

		while(n) {
			if(n->nref < best_nref && n->nsub) {
				best = n;
				best_nref = n->nref;
			}
			n = n->next;
		}

		if(best) {
			for(i=0; i<8; i++) {
				if(best->sub[i]) {
					del_node(best->sub[i], lvl + 1);
					best->sub[i] = 0;
				}
			}
			if(best->nsub) {
				/* this wasn't previously a leaf, but now it is */
				ot->ncol++;
				best->nsub = 0;
			}
			break;
		}
	}
}

static int assign_colors(struct octnode *on, int next, struct cmapent *cmap)
{
	int i;

	if(!on) return next;

	if(on->nsub <= 0) {
		assert(next < on->tree->maxcol);
		cmap[next].r = on->r / on->nref;
		cmap[next].g = on->g / on->nref;
		cmap[next].b = on->b / on->nref;
		on->palidx = next++;
	}

	for(i=0; i<8; i++) {
		next = assign_colors(on->sub[i], next, cmap);
	}
	return next;
}

static int lookup_color(struct octree *ot, int r, int g, int b)
{
	int i, idx;
	struct octnode *on = ot->root;

	for(i=0; i<8; i++) {
		idx = subidx(i, r, g, b);
		if(!on->sub[idx]) break;
		on = on->sub[idx];
	}

	return on->palidx;
}

static int have_node(struct octnode *list, struct octnode *n)
{
	while(list) {
		if(list == n) return 1;
		list = list->next;
	}
	return 0;
}

static struct octnode *new_node(struct octree *ot, int lvl)
{
	struct octnode *on;

	if(!(on = calloc(1, sizeof *on))) {
		perror("failed to allocate octree node");
		abort();
	}

	on->tree = ot;
	on->palidx = -1;

	if(lvl < 8) {
		if(have_node(ot->levn[lvl], on)) {
			fprintf(stderr, "double-insertion!\n");
			abort();
		}
		on->next = ot->levn[lvl];
		ot->levn[lvl] = on;
	}
	return on;
}

static void del_node(struct octnode *on, int lvl)
{
	int i;
	struct octree *ot;
	struct octnode dummy, *prev;

	if(!on) return;
	ot = on->tree;

	if(!on->nsub) {
		ot->ncol--;	/* removing a leaf removes a color */
	}

	for(i=0; i<8; i++) {
		del_node(on->sub[i], lvl + 1);
	}

	if(lvl < 8) {
		dummy.next = ot->levn[lvl];
		prev = &dummy;

		while(prev->next) {
			if(prev->next == on) {
				prev->next = on->next;
				break;
			}
			prev = prev->next;
		}
		ot->levn[lvl] = dummy.next;
	}

	free(on);
}

static void print_tree(struct octnode *n, int lvl)
{
	int i;

	if(!n) return;

	for(i=0; i<lvl; i++) {
		fputs("|  ", stdout);
	}

	printf("+-%p: <%d %d %d> #%d", (void*)n, n->r, n->g, n->b, n->nref);
	if(n->palidx >= 0) printf(" [%d]\n", n->palidx);
	putchar('\n');

	for(i=0; i<8; i++) {
		print_tree(n->sub[i], lvl + 1);
	}
}
#endif
