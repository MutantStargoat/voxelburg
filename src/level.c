#include "util.h"
#include "debug.h"
#include "level.h"

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
