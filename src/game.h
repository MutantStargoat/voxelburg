#ifndef GAME_H_
#define GAME_H_

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

#endif	/* GAME_H_ */
