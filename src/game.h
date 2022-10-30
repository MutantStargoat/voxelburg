#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>

struct screen {
	char *name;
	int (*start)(void);
	void (*stop)(void);
	void (*frame)(void);
	void (*vblank)(void);
};

extern struct screen *curscr;

int init_screens(void);
int change_screen(struct screen *scr);
struct screen *find_screen(const char *name);

int gba_colors;

#ifndef BUILD_GBA
int32_t view_dtheta, view_dphi, view_zoom;
#endif

#endif	/* GAME_H_ */
