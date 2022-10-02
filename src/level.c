#include "util.h"
#include "debug.h"
#include "level.h"
#include "player.h"
#include "xgl.h"

struct level *init_level(const char *descstr)
{
	const char *str, *line;
	int i, j, ncols = 0, nrows = 0;
	struct level *lvl;
	struct cell *cell;

	str = line = descstr;

	while(*str) {
		if(*str == '\n') {
			if(ncols > 0 && str > line && str - line != ncols) {
				panic(get_pc(), "init_level: inconsistent ncols (%d != %d)\n", str - line, ncols);
			}
			ncols = str - line;
			nrows++;
			while(*++str == '\n');
			line = str;
		} else {
			str++;
		}
	}

	if(!ispow2(ncols)) {
		panic(get_pc(), "init_level: width is not pow2 (%d)\n", ncols);
	}

	lvl = malloc_nf(sizeof *lvl);
	lvl->width = ncols;
	lvl->xmask = ncols - 1;
	lvl->height = nrows;
	lvl->orgx = lvl->width >> 1;
	lvl->orgy = lvl->height >> 1;
	lvl->cells = calloc_nf(ncols * nrows, sizeof *lvl->cells);
	lvl->mobs = 0;
	lvl->items = 0;

	str = descstr;
	cell = lvl->cells;

	for(i=0; i<nrows; i++) {
		for(j=0; j<ncols; j++) {
			cell->x = j;
			cell->y = i;
			if(*str == '#') {
				cell->type = CELL_SOLID;
			} else {
				cell->type = CELL_WALK;
			}
			cell++;
			while(*++str == '\n') str++;
		}
	}

	return lvl;
}

void free_level(struct level *lvl)
{
	void *tmp;

	free(lvl->cells);

	while(lvl->mobs) {
		tmp = lvl->mobs;
		lvl->mobs = lvl->mobs->next;
		free(tmp);
	}
	while(lvl->items) {
		tmp = lvl->items;
		lvl->items = lvl->items->next;
		free(tmp);
	}
}

struct {int dx, dy;} visoffs[8][32] = {
	/* dir 0 */
	{{-4,-4}, {4,-4}, {-3,-4}, {3,-4}, {-2,-4}, {2,-4}, {-3,-3}, {3,-3}, {-1,-4},
	 {1,-4}, {0,-4}, {-2,-3}, {2,-3}, {-1,-3}, {1,-3}, {0,-3}, {-2,-2}, {2,-2},
	 {-1,-2}, {1,-2}, {0,-2}, {-1,-1}, {1,-1}, {0,-1}, {0,0}},
	/* dir 1 */
	{{4,-4}, {3,-4}, {4,-3}, {2,-4}, {4,-2}, {3,-3}, {1,-4}, {4,-1}, {0,-4},
	 {4,0}, {2,-3}, {3,-2}, {1,-3}, {3,-1}, {0,-3}, {3,0}, {2,-2}, {1,-2},
	 {2,-1}, {0,-2}, {2,0}, {1,-1}, {0,-1}, {1,0}, {0,0}},
	/* dir 2 */
	{{4,-4}, {4,4}, {4,-3}, {4,3}, {4,-2}, {4,2}, {3,-3}, {3,3}, {4,-1}, {4,1},
	 {4,0}, {3,-2}, {3,2}, {3,-1}, {3,1}, {3,0}, {2,-2}, {2,2}, {2,-1}, {2,1},
	 {2,0}, {1,-1}, {1,1}, {1,0}, {0,0}},
	/* dir 3 */
	{{4,4}, {4,3}, {3,4}, {4,2}, {2,4}, {3,3}, {4,1}, {1,4}, {4,0}, {0,4},
	 {3,2}, {2,3}, {3,1}, {1,3}, {3,0}, {0,3}, {2,2}, {2,1}, {1,2}, {2,0},
	 {0,2}, {1,1}, {1,0}, {0,1}, {0,0}},
	/* dir 4 */
	{{-4,4}, {4,4}, {-3,4}, {3,4}, {-2,4}, {2,4}, {-3,3}, {3,3}, {-1,4}, {1,4},
	 {0,4}, {-2,3}, {2,3}, {-1,3}, {1,3}, {0,3}, {-2,2}, {2,2}, {-1,2}, {1,2},
	 {0,2}, {-1,1}, {1,1}, {0,1}, {0,0}},
	/* dir 5 */
	{{-4,4}, {-4,3}, {-3,4}, {-4,2}, {-2,4}, {-3,3}, {-4,1}, {-1,4}, {-4,0},
	 {0,4}, {-3,2}, {-2,3}, {-3,1}, {-1,3}, {-3,0}, {0,3}, {-2,2}, {-2,1},
	 {-1,2}, {-2,0}, {0,2}, {-1,1}, {-1,0}, {0,1}, {0,0}},
	/* dir 6 */
	{{-4,-4}, {-4,4}, {-4,-3}, {-4,3}, {-4,-2}, {-4,2}, {-3,-3}, {-3,3}, {-4,-1},
	 {-4,1}, {-4,0}, {-3,-2}, {-3,2}, {-3,-1}, {-3,1}, {-3,0}, {-2,-2}, {-2,2},
	 {-2,-1}, {-2,1}, {-2,0}, {-1,-1}, {-1,1}, {-1,0}, {0,0}},
	/* dir 7 */
	{{-4,-4}, {-3,-4}, {-4,-3}, {-2,-4}, {-4,-2}, {-3,-3}, {-1,-4}, {-4,-1},
	 {0,-4}, {-4,0}, {-2,-3}, {-3,-2}, {-1,-3}, {-3,-1}, {0,-3}, {-3,0}, {-2,-2},
	 {-1,-2}, {-2,-1}, {0,-2}, {-2,0}, {-1,-1}, {0,-1}, {-1,0}, {0,0}}
};

void upd_vis(struct level *lvl, struct player *p)
{
	int dir, idx;
	int x, y;
	struct cell *cptr;
	int32_t theta;

	pos_to_cell(lvl, p->x, p->y, &p->cx, &p->cy);

	lvl->numvis = 0;
	idx = -1;
	theta = X_2PI - p->theta + X_2PI / 16;
	if(theta >= X_2PI) theta -= X_2PI;
	dir = (theta << 3) / X_2PI;	/* p->theta is always [0, 2pi) */
	if(dir < 0 || dir >= 8) {
		panic(get_pc(), "dir: %d\ntheta: %d.%d (%d)\n", dir, p->theta >> 16,
				p->theta & 0xffff, p->theta);
	}
	do {
		idx++;
		x = p->cx + visoffs[dir][idx].dx;
		y = p->cy + visoffs[dir][idx].dy;
		cptr = lvl->cells + y * lvl->width + x;
		lvl->vis[lvl->numvis++] = cptr;
	} while(visoffs[dir][idx].dx | visoffs[dir][idx].dy);
}

void cell_to_pos(struct level *lvl, int cx, int cy, int32_t *px, int32_t *py)
{
	*px = (cx - lvl->orgx) * CELL_SIZE;
	*py = (cy - lvl->orgy) * CELL_SIZE;
}

void pos_to_cell(struct level *lvl, int32_t px, int32_t py, int *cx, int *cy)
{
	*cx = (px + (CELL_SIZE >> 1)) / CELL_SIZE + lvl->orgx;
	*cy = (py + (CELL_SIZE >> 1)) / CELL_SIZE + lvl->orgy;
}
