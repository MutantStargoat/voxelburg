#ifndef LEVEL_H_
#define LEVEL_H_

#include <stdint.h>

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
	uint8_t pad;
	struct mob *mobs;
	struct item *items;
};

struct level {
	int width, height;
	unsigned int xmask;
	struct cell *cells;

	struct mob *mobs;
	struct item *items;
};


struct level *init_level(const char *descstr);
void free_level(struct level *lvl);

#endif	/* LEVEL_H_ */
