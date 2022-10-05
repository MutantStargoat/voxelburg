#ifndef LEVEL_H_
#define LEVEL_H_

#include <stdint.h>

/* cell size in 16.16 fixed point */
#define CELL_SIZE	0x20000

enum { MOBS_DEAD, MOBS_IDLE, MOBS_ENGAGE, MOBS_RUN };

struct mob {
	uint8_t type;
	uint8_t x, y;
	uint8_t state;
	struct mob *next;
	struct mob *cellnext;

	void (*update)(void);
};

struct item {
	uint8_t type;
	uint8_t x, y;
	uint8_t pad;
	struct item *next;
	struct item *cellnext;
};

enum { CELL_SOLID, CELL_WALK };

struct cell {
	uint8_t type;
	uint8_t x, y;
	uint8_t rank;
	struct mob *mobs;
	struct item *items;
};

struct level {
	int width, height;
	int orgx, orgy;
	unsigned int xmask;
	int xshift;
	struct cell *cells;

	struct mob *mobs;
	struct item *items;

	/* populated by calc_vis */
	struct cell *vis[128];
	int numvis;
};

struct player;

struct level *init_level(const char *descstr);
void free_level(struct level *lvl);

struct cell *level_cell(struct level *lvl, int cx, int cy);

void upd_vis(struct level *lvl, struct player *p);

void cell_to_pos(struct level *lvl, int cx, int cy, int32_t *px, int32_t *py);
void pos_to_cell(struct level *lvl, int32_t px, int32_t py, int *cx, int *cy);

#endif	/* LEVEL_H_ */
