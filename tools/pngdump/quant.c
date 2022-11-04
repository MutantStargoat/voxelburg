#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include "image.h"

#define NUM_LEVELS	8

struct octnode;

struct octree {
	struct octnode *root;
	struct octnode *redlist[NUM_LEVELS];
	int redlev;
	int nleaves, maxcol;
};

struct octnode {
	int lvl;
	struct octree *tree;
	int r, g, b, nref;
	int palidx;
	int nsub, leaf;
	struct octnode *sub[8];
	struct octnode *next;
};


static void init_octree(struct octree *tree, int maxcol);
static void destroy_octree(struct octree *tree);

static struct octnode *alloc_node(struct octree *tree, int lvl);
static void free_node(struct octnode *n);
static void free_tree(struct octnode *n);

static void add_color(struct octree *tree, int r, int g, int b, int nref);
static void reduce_colors(struct octree *tree);
static int assign_colors(struct octnode *n, int next, struct cmapent *cmap);
static int lookup_color(struct octree *tree, int r, int g, int b);
static int subidx(int bit, int r, int g, int b);
static void print_tree(struct octnode *n, int lvl);

int quantize_image(struct image *img, int maxcol)
{
	int i, j, cidx;
	unsigned int rgb[3];
	struct octree tree;
	struct image newimg = *img;

	if(maxcol < 2 || maxcol > 256) {
		return -1;
	}

	if(img->bpp > 8) {
		newimg.bpp = maxcol > 16 ? 8 : 4;
		newimg.nchan = 1;
		newimg.scansz = newimg.width * newimg.bpp / 8;
		newimg.pitch = newimg.scansz;
	}

	init_octree(&tree, maxcol);

	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			get_pixel_rgb(img, j, i, rgb);
			add_color(&tree, rgb[0], rgb[1], rgb[2], 1);

			while(tree.nleaves > maxcol) {
				reduce_colors(&tree);
			}
		}
	}

	/* use created octree to generate the palette */
	newimg.cmap_ncolors = assign_colors(tree.root, 0, newimg.cmap);

	/* replace image pixels */
	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			get_pixel_rgb(img, j, i, rgb);
			cidx = lookup_color(&tree, rgb[0], rgb[1], rgb[2]);
			assert(cidx >= 0 && cidx < maxcol);
			put_pixel(&newimg, j, i, cidx);
		}
	}

	*img = newimg;

	destroy_octree(&tree);
	return 0;
}

int gen_shades(struct image *img, int levels, int maxcol, int *shade_lut)
{
	int i, j, cidx, r, g, b;
	unsigned int color[3];
	struct octree tree;
	struct image newimg = *img;

	if(maxcol < 2 || maxcol > 256) {
		return -1;
	}

	init_octree(&tree, maxcol);

	for(i=0; i<img->cmap_ncolors; i++) {
		add_color(&tree, img->cmap[i].r, img->cmap[i].g, img->cmap[i].b, 1024);
	}

	for(i=0; i<img->cmap_ncolors; i++) {
		for(j=0; j<levels - 1; j++) {
			r = img->cmap[i].r * j / (levels - 1);
			g = img->cmap[i].g * j / (levels - 1);
			b = img->cmap[i].b * j / (levels - 1);
			add_color(&tree, r, g, b, 1);

			while(tree.nleaves > maxcol) {
				reduce_colors(&tree);
			}
		}
	}

	newimg.cmap_ncolors = assign_colors(tree.root, 0, newimg.cmap);

	/* replace image pixels */
	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			get_pixel_rgb(img, j, i, color);
			cidx = lookup_color(&tree, color[0], color[1], color[2]);
			put_pixel(&newimg, j, i, cidx);
		}
	}

	/* populate shade_lut based on the new palette, can't generate levels only
	 * for the original colors, because the palette entries will have changed
	 * and moved around.
	 */
	for(i=0; i<newimg.cmap_ncolors; i++) {
		for(j=0; j<levels; j++) {
			r = newimg.cmap[i].r * j / (levels - 1);
			g = newimg.cmap[i].g * j / (levels - 1);
			b = newimg.cmap[i].b * j / (levels - 1);
			*shade_lut++ = lookup_color(&tree, r, g, b);
		}
	}
	for(i=0; i<(maxcol - newimg.cmap_ncolors) * levels; i++) {
		*shade_lut++ = maxcol - 1;
	}

	*img = newimg;

	destroy_octree(&tree);
	return 0;
}

static void init_octree(struct octree *tree, int maxcol)
{
	memset(tree, 0, sizeof *tree);

	tree->redlev = NUM_LEVELS - 1;
	tree->maxcol = maxcol;

	tree->root = alloc_node(tree, 0);
}

static void destroy_octree(struct octree *tree)
{
	free_tree(tree->root);
}

static struct octnode *alloc_node(struct octree *tree, int lvl)
{
	struct octnode *n;

	if(!(n = calloc(1, sizeof *n))) {
		perror("failed to allocate octree node");
		abort();
	}

	n->lvl = lvl;
	n->tree = tree;
	n->palidx = -1;

	if(lvl < tree->redlev) {
		n->next = tree->redlist[lvl];
		tree->redlist[lvl] = n;
	} else {
		n->leaf = 1;
		tree->nleaves++;
	}
	return n;
}

static void free_node(struct octnode *n)
{
	struct octnode *prev, dummy;

	dummy.next = n->tree->redlist[n->lvl];
	prev = &dummy;
	while(prev->next) {
		if(prev->next == n) {
			prev->next = n->next;
			break;
		}
		prev = prev->next;
	}
	n->tree->redlist[n->lvl] = dummy.next;

	if(n->leaf) {
		n->tree->nleaves--;
		assert(n->tree->nleaves >= 0);
	}
	free(n);
}

static void free_tree(struct octnode *n)
{
	int i;

	if(!n) return;

	for(i=0; i<8; i++) {
		free_tree(n->sub[i]);
	}
	free_node(n);
}

static void add_color(struct octree *tree, int r, int g, int b, int nref)
{
	int i, idx, rr, gg, bb;
	struct octnode *n;

	rr = r * nref;
	gg = g * nref;
	bb = b * nref;

	n = tree->root;
	n->r += rr;
	n->g += gg;
	n->b += bb;
	n->nref += nref;

	for(i=0; i<NUM_LEVELS; i++) {
		if(n->leaf) break;

		idx = subidx(i, r, g, b);

		if(!n->sub[idx]) {
			n->sub[idx] = alloc_node(tree, i + 1);
		}
		n->nsub++;
		n = n->sub[idx];

		n->r += rr;
		n->g += gg;
		n->b += bb;
		n->nref += nref;
	}
}

static struct octnode *get_reducible(struct octree *tree)
{
	int best_nref;
	struct octnode dummy, *n, *prev, *best_prev, *best = 0;

	while(tree->redlev >= 0) {
		best_nref = INT_MAX;
		best = 0;
		dummy.next = tree->redlist[tree->redlev];
		prev = &dummy;
		while(prev->next) {
			n = prev->next;
			if(n->nref < best_nref) {
				best = n;
				best_nref = n->nref;
				best_prev = prev;
			}
			prev = prev->next;
		}
		if(best) {
			best_prev->next = best->next;
			tree->redlist[tree->redlev] = dummy.next;
			break;
		}
		tree->redlev--;
	}

	return best;
}

static void reduce_colors(struct octree *tree)
{
	int i;
	struct octnode *n;

	if(!(n = get_reducible(tree))) {
		fprintf(stderr, "warning: no reducible nodes!\n");
		return;
	}
	for(i=0; i<8; i++) {
		if(n->sub[i]) {
			free_node(n->sub[i]);
			n->sub[i] = 0;
		}
	}
	n->leaf = 1;
	tree->nleaves++;
}

static int assign_colors(struct octnode *n, int next, struct cmapent *cmap)
{
	int i;

	if(!n) return next;

	if(n->leaf) {
		assert(next < n->tree->maxcol);
		assert(n->nref);
		cmap[next].r = n->r / n->nref;
		cmap[next].g = n->g / n->nref;
		cmap[next].b = n->b / n->nref;
		n->palidx = next;
		return next + 1;
	}

	for(i=0; i<8; i++) {
		next = assign_colors(n->sub[i], next, cmap);
	}
	return next;
}

static int lookup_color(struct octree *tree, int r, int g, int b)
{
	int i, j, idx;
	struct octnode *n;

	n = tree->root;
	for(i=0; i<NUM_LEVELS; i++) {
		if(n->leaf) {
			assert(n->palidx >= 0);
			return n->palidx;
		}

		idx = subidx(i, r, g, b);
		for(j=0; j<8; j++) {
			if(n->sub[idx]) break;
			idx = (idx + 1) & 7;
		}

		assert(n->sub[idx]);
		n = n->sub[idx];
	}

	fprintf(stderr, "lookup_color(%d, %d, %d) failed!\n", r, g, b);
	abort();
	return -1;
}

static int subidx(int bit, int r, int g, int b)
{
	assert(bit >= 0 && bit < NUM_LEVELS);
	bit = NUM_LEVELS - 1 - bit;
	return ((r >> bit) & 1) | ((g >> (bit - 1)) & 2) | ((b >> (bit - 2)) & 4);
}

static void print_tree(struct octnode *n, int lvl)
{
	int i;
	char ptrbuf[32], *p;

	if(!n) return;

	for(i=0; i<lvl; i++) {
		fputs("|  ", stdout);
	}

	sprintf(ptrbuf, "%p", (void*)n);
	p = ptrbuf + strlen(ptrbuf) - 4;

	if(n->nref) {
		printf("+-(%d) %s: <%d %d %d> #%d", n->lvl, p, n->r / n->nref, n->g / n->nref,
				n->b / n->nref, n->nref);
	} else {
		printf("+-(%d) %s: <- - -> #0", n->lvl, p);
	}
	if(n->palidx >= 0) printf(" [%d]", n->palidx);
	if(n->leaf) printf(" LEAF");
	putchar('\n');

	for(i=0; i<8; i++) {
		print_tree(n->sub[i], lvl + 1);
	}
}

