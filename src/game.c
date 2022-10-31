#include <string.h>
#include "gba.h"
#include "game.h"

struct screen *init_logo_screen(void);
struct screen *init_menu_screen(void);
struct screen *init_game_screen(void);
struct screen *init_controls_screen(void);

struct screen *curscr;

#define MAX_SCR	4
static struct screen *scrlist[MAX_SCR];
static int num_scr;

int init_screens(void)
{
	if(!(scrlist[num_scr++] = init_logo_screen())) {
		return -1;
	}
	if(!(scrlist[num_scr++] = init_menu_screen())) {
		return -1;
	}
	if(!(scrlist[num_scr++] = init_controls_screen())) {
		return -1;
	}
	if(!(scrlist[num_scr++] = init_game_screen())) {
		return -1;
	}
	return 0;
}

int change_screen(struct screen *scr)
{
	if(!scr) return -1;

	mask(INTR_VBLANK);

	if(curscr && curscr->stop) {
		curscr->stop();
	}
	if(scr->start && scr->start() == -1) {
		return -1;
	}
	curscr = scr;

	unmask(INTR_VBLANK);
	return 0;
}

struct screen *find_screen(const char *name)
{
	int i;
	for(i=0; i<num_scr; i++) {
		if(strcmp(scrlist[i]->name, name) == 0) {
			return scrlist[i];
		}
	}
	return 0;
}
