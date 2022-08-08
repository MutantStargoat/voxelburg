#ifndef PLAYER_H_
#define PLAYER_H_

#include <stdint.h>

struct player {
	int32_t x, y;
	int32_t theta, phi;
};

void player_input(struct player *p, uint16_t bnstate);

#endif	/* PLAYER_H_ */
