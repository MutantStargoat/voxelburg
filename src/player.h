#ifndef PLAYER_H_
#define PLAYER_H_

#include <stdint.h>

struct cell;
struct level;

struct player {
	int32_t x, y;
	int32_t theta, phi;
	int cx, cy;
	struct cell *cell;
};

void init_player(struct player *p, struct level *lvl);
void player_input(struct player *p, uint16_t bnstate);

#endif	/* PLAYER_H_ */
