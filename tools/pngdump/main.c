#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include "image.h"

enum {
	MODE_PIXELS,
	MODE_CMAP,
	MODE_PNG,
	MODE_INFO
};

void conv_gba_image(struct image *img);
void dump_colormap(struct image *img, int text, FILE *fp);
void print_usage(const char *argv0);

int main(int argc, char **argv)
{
	int i, j, mode = 0;
	int text = 0;
	int renibble = 0;
	char *outfname = 0;
	char *slut_fname = 0, *cmap_fname = 0;
	char *infiles[256];
	int num_infiles = 0;
	struct image img, tmpimg;
	FILE *out = stdout;
	FILE *aux_out;
	int *shade_lut = 0;
	int *lutptr;
	int shade_levels = 8;
	int maxcol = 0;
	int lvl;
	int conv_555 = 0;
	int gbacolors = 0;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(argv[i][2] == 0) {
				switch(argv[i][1]) {
				case 'P':
					mode = MODE_PNG;
					break;

				case 'p':
					mode = MODE_PIXELS;
					break;

				case 'c':
					mode = MODE_CMAP;
					break;

				case 'i':
					mode = MODE_INFO;
					break;

				case 'C':
					if(!argv[++i] || (maxcol = atoi(argv[i])) < 2 || maxcol > 256) {
						fprintf(stderr, "-C must be followed by the number of colors to reduce down to\n");
						return 1;
					}
					break;

				case 's':
					if(!argv[++i] || (shade_levels = atoi(argv[i])) == 0) {
						fprintf(stderr, "-s must be followed by the number of shade levels\n");
						return 1;
					}
					break;

				case 't':
					text = 1;
					break;

				case 'n':
					renibble = 1;
					break;

				case 'g':
					gbacolors = 1;
					break;

				case 'o':
					if(!argv[++i]) {
						fprintf(stderr, "%s must be followed by a filename\n", argv[i - 1]);
						return 1;
					}
					outfname = argv[i];
					break;

				case 'h':
					print_usage(argv[0]);
					return 0;

				default:
					fprintf(stderr, "invalid option: %s\n", argv[i]);
					print_usage(argv[0]);
					return 1;
				}
			} else {
				if(strcmp(argv[i], "-oc") == 0) {
					if(!argv[++i]) {
						fprintf(stderr, "-oc must be followed by a filename\n");
						return 1;
					}
					cmap_fname = argv[i];

				} else if(strcmp(argv[i], "-os") == 0) {
					if(!argv[++i]) {
						fprintf(stderr, "-os must be followed by a filename\n");
						return 1;
					}
					slut_fname = argv[i];

				} else if(strcmp(argv[i], "-555") == 0) {
					conv_555 = 1;

				} else {
					fprintf(stderr, "invalid option: %s\n", argv[i]);
					print_usage(argv[0]);
					return 1;
				}
			}
		} else {
			infiles[num_infiles++] = argv[i];
		}
	}

	if(!num_infiles) {
		fprintf(stderr, "pass the filename of a PNG file\n");
		return 1;
	}
	if(load_image(&img, infiles[0]) == -1) {
		fprintf(stderr, "failed to load PNG file: %s\n", infiles[0]);
		return 1;
	}

	if(gbacolors) {
		conv_gba_image(&img);
	}

	for(i=1; i<num_infiles; i++) {
		if(load_image(&tmpimg, infiles[i]) == -1) {
			fprintf(stderr, "failed to load PNG file: %s\n", infiles[i]);
			return 1;
		}
		if(tmpimg.width != img.width || tmpimg.height != img.height) {
			fprintf(stderr, "size mismatch: first image (%s) is %dx%d, %s is %dx%d\n",
					infiles[0], img.width, img.height, infiles[i], tmpimg.width, tmpimg.height);
			return 1;
		}
		if(tmpimg.bpp != img.bpp) {
			fprintf(stderr, "bpp mismatch: first image (%s) is %d bpp, %s is %d bpp\n",
					infiles[0], img.bpp, infiles[i], img.bpp);
			return 1;
		}

		overlay_key(&tmpimg, 0, &img);
	}

	/* generate shading LUT and quantize image as necessary */
	if(slut_fname) {
		if(img.bpp > 8) {
			fprintf(stderr, "shading LUT generation is only supported for indexed color images\n");
			return 1;
		}
		if(!(aux_out = fopen(slut_fname, "wb"))) {
			fprintf(stderr, "failed to open shading LUT output file: %s: %s\n", slut_fname, strerror(errno));
			return 1;
		}

		if(!maxcol) maxcol = 256;

		if(!(shade_lut = malloc(maxcol * shade_levels * sizeof *shade_lut))) {
			fprintf(stderr, "failed to allocate shading look-up table\n");
			return 1;
		}

		gen_shades(&img, shade_levels, maxcol, shade_lut);

		lutptr = shade_lut;
		for(i=0; i<maxcol; i++) {
			for(j=0; j<shade_levels; j++) {
				lvl = lutptr[shade_levels - j - 1];
				if(text) {
					fprintf(aux_out, "%d%c", lvl, j < shade_levels - 1 ? ' ' : '\n');
				} else {
					fputc(lvl, aux_out);
				}
			}
			lutptr += shade_levels;
		}
		fclose(aux_out);

	} else if(maxcol) {
		/* perform any color reductions if requested */
		if(img.bpp <= 8 && img.cmap_ncolors <= maxcol) {
			fprintf(stderr, "requested reduction to %d colors, but image has %d colors\n", maxcol, img.cmap_ncolors);
			return 1;
		}
		quantize_image(&img, maxcol);
	}

	if(cmap_fname) {
		if(img.bpp > 8) {
			fprintf(stderr, "colormap output works only for indexed color images\n");
			return 1;
		}
		if(!(aux_out = fopen(cmap_fname, "wb"))) {
			fprintf(stderr, "failed to open colormap output file: %s: %s\n", cmap_fname, strerror(errno));
			return 1;
		}
		dump_colormap(&img, text, aux_out);
		fclose(aux_out);
	}

	if(img.bpp == 4 && renibble) {
		unsigned char *ptr = img.pixels;
		for(i=0; i<img.width * img.height; i++) {
			unsigned char p = *ptr;
			*ptr++ = (p << 4) | (p >> 4);
		}
	}

	if(conv_555) {
		struct image img555;
		unsigned int rgb24[3], rgb15;

		if(alloc_image(&img555, img.width, img.height, 15) == -1) {
			fprintf(stderr, "failed to allocate temporary %dx%d image for 555 conversion\n",
					img.width, img.height);
			return 1;
		}

		for(i=0; i<img.height; i++) {
			for(j=0; j<img.width; j++) {
				get_pixel_rgb(&img, j, i, rgb24);
				rgb15 = ((rgb24[0] >> 3) & 0x1f) | ((rgb24[1] << 2) & 0x3e0) |
					((rgb24[2] << 7) & 0x7c00);
				put_pixel(&img555, j, i, rgb15);
			}
		}
		free(img.pixels);
		img = img555;
	}

	if(outfname) {
		if(!(out = fopen(outfname, "wb"))) {
			fprintf(stderr, "failed to open output file: %s: %s\n", outfname, strerror(errno));
			return 1;
		}
	}

	switch(mode) {
	case MODE_PNG:
		save_image_file(&img, out);
		break;

	case MODE_PIXELS:
		fwrite(img.pixels, 1, img.scansz * img.height, out);
		break;

	case MODE_CMAP:
		dump_colormap(&img, text, out);
		break;

	case MODE_INFO:
		printf("size: %dx%d\n", img.width, img.height);
		printf("bit depth: %d\n", img.bpp);
		printf("scanline size: %d bytes\n", img.scansz);
		if(img.cmap_ncolors > 0) {
			printf("colormap entries: %d\n", img.cmap_ncolors);
		} else {
			printf("color channels: %d\n", img.nchan);
		}
		break;
	}

	fclose(out);
	return 0;
}

#define MIN(a, b)			((a) < (b) ? (a) : (b))
#define MIN3(a, b, c)		((a) < (b) ? MIN(a, c) : MIN(b, c))
#define MAX(a, b)			((a) > (b) ? (a) : (b))
#define MAX3(a, b, c)		((a) > (b) ? MAX(a, c) : MAX(b, c))

void rgb_to_hsv(float *rgb, float *hsv)
{
	float min, max, delta;

	min = MIN3(rgb[0], rgb[1], rgb[2]);
	max = MAX3(rgb[0], rgb[1], rgb[2]);
	delta = max - min;

	if(max == 0) {
		hsv[0] = hsv[1] = hsv[2] = 0;
		return;
	}

	hsv[2] = max;			/* value */
	hsv[1] = delta / max;	/* saturation */

	if(delta == 0.0f) {
		hsv[0] = 0.0f;
	} else if(max == rgb[0]) {
		hsv[0] = (rgb[1] - rgb[2]) / delta;
	} else if(max == rgb[1]) {
		hsv[0] = 2.0f + (rgb[2] - rgb[0]) / delta;
	} else {
		hsv[0] = 4.0f + (rgb[0] - rgb[1]) / delta;
	}
	/*
	hsv[0] /= 6.0f;

	if(hsv[0] < 0.0f) hsv[0] += 1.0f;
	*/
	hsv[0] *= 60.0f;
	if(hsv[0] < 0) hsv[0] += 360;
	hsv[0] /= 360.0f;
}

#define RETRGB(r, g, b) \
	do { \
		rgb[0] = r; \
		rgb[1] = g; \
		rgb[2] = b; \
		return; \
	} while(0)

void hsv_to_rgb(float *hsv, float *rgb)
{
	float sec, frac, o, p, q;
	int hidx;

	if(hsv[1] == 0.0f) {
		rgb[0] = rgb[1] = rgb[2] = hsv[2];	/* value */
	}

	sec = floor(hsv[0] * (360.0f / 60.0f));
	frac = (hsv[0] * (360.0f / 60.0f)) - sec;

	o = hsv[2] * (1.0f - hsv[1]);
	p = hsv[2] * (1.0f - hsv[1] * frac);
	q = hsv[2] * (1.0f - hsv[1] * (1.0f - frac));

	hidx = (int)sec;
	switch(hidx) {
	default:
	case 0: RETRGB(hsv[2], q, o);
	case 1: RETRGB(p, hsv[2], o);
	case 2: RETRGB(o, hsv[2], q);
	case 3: RETRGB(o, p, hsv[2]);
	case 4: RETRGB(q, o, hsv[2]);
	case 5: RETRGB(hsv[2], o, p);
	}
}

void gba_color(struct cmapent *color)
{
	float rgb[3], hsv[3];

	rgb[0] = pow((float)color->r / 255.0f, 2.2);
	rgb[1] = pow((float)color->g / 255.0f, 2.2);
	rgb[2] = pow((float)color->b / 255.0f, 2.2);

	/* saturate colors */
	rgb_to_hsv(rgb, hsv);
	hsv[1] *= 1.2f;
	hsv[2] *= 2.0f;
	if(hsv[1] > 1.0f) hsv[1] = 1.0f;
	if(hsv[2] > 1.0f) hsv[2] = 1.0f;
	hsv_to_rgb(hsv, rgb);

	rgb[0] = pow(rgb[0], 1.0 / 2.6);
	rgb[1] = pow(rgb[1], 1.0 / 2.6);
	rgb[2] = pow(rgb[2], 1.0 / 2.6);

	color->r = (int)(rgb[0] * 255.0f);
	color->g = (int)(rgb[1] * 255.0f);
	color->b = (int)(rgb[2] * 255.0f);
}

void conv_gba_image(struct image *img)
{
	int i;

	if(img->cmap_ncolors) {
		for(i=0; i<img->cmap_ncolors; i++) {
			gba_color(img->cmap + i);
		}
	} else {
		/* TODO */
	}
}

void dump_colormap(struct image *img, int text, FILE *fp)
{
	int i;

	if(text) {
		for(i=0; i<img->cmap_ncolors; i++) {
			fprintf(fp, "%d %d %d\n", img->cmap[i].r, img->cmap[i].g, img->cmap[i].b);
		}
	} else {
		fwrite(img->cmap, sizeof img->cmap[0], 1 << img->bpp, fp);
	}
}

void print_usage(const char *argv0)
{
	printf("Usage: %s [options] <input file>\n", argv0);
	printf("Options:\n");
	printf(" -o <output file>: specify output file (default: stdout)\n");
	printf(" -oc <cmap file>: output colormap to separate file\n");
	printf(" -os <lut file>: generate and output shading LUT\n");
	printf(" -p: dump pixels (default)\n");
	printf(" -P: output in PNG format\n");
	printf(" -c: dump colormap (palette) entries\n");
	printf(" -C <colors>: reduce image down to specified number of colors\n");
	printf(" -s <shade levels>: used in conjunction with -os (default: 8)\n");
	printf(" -i: print image information\n");
	printf(" -t: output as text when possible\n");
	printf(" -n: swap the order of nibbles (for 4bpp)\n");
	printf(" -555: convert to BGR555\n");
	printf(" -h: print usage and exit\n");
}
